// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "gtk2/render_surface.hpp"
#include "util/errors.hpp"
#include "util/gl_free.hpp"
#include "vpython-config.h"

#include <gtkmm/gl/init.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/stock.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/toolbar.h>
#ifdef VPYTHON_USE_GTKMM_24
# include <gtkmm/toggletoolbutton.h>
# include <gtkmm/radiotoolbutton.h>
#endif

#include <algorithm>
#include <iostream>

#include <boost/lexical_cast.hpp>

namespace cvisual {

namespace {
	Glib::RefPtr<Gdk::GL::Context> share_list;
}

render_surface::render_surface( display_kernel& _core, bool activestereo)
	: last_mousepos_x(0),
	last_mousepos_y(0),
	last_mouseclick_x(0),
	last_mouseclick_y(0),
	left_button_down(false),
	middle_button_down(false),
	right_button_down(false),
	dragging(false),
	cycle_time(30),
	core( _core)
{
	Glib::RefPtr<Gdk::GL::Config> config;

	if (activestereo) {
		config = Gdk::GL::Config::create( 
			Gdk::GL::MODE_RGBA
			| Gdk::GL::MODE_DOUBLE
			| Gdk::GL::MODE_DEPTH
			| Gdk::GL::MODE_MULTISAMPLE
			| Gdk::GL::MODE_STEREO );
		if (!config) {
			// Try again without the multisample extension.
			config = Gdk::GL::Config::create( Gdk::GL::MODE_RGB
				| Gdk::GL::MODE_DOUBLE
				| Gdk::GL::MODE_DEPTH
				| Gdk::GL::MODE_STEREO );
			if (!config) {
				VPYTHON_WARNING("'active' stereo requested, but not available."
					"  Falling back to: 'nostereo'.");
			}
		}
	}
	else if (!activestereo || !config) {	
		config = Gdk::GL::Config::create( 
			Gdk::GL::MODE_RGBA
			| Gdk::GL::MODE_DOUBLE
			| Gdk::GL::MODE_DEPTH
			| Gdk::GL::MODE_MULTISAMPLE );
		if (!config) {
			// Try again without the multisample extension.
			config = Gdk::GL::Config::create( Gdk::GL::MODE_RGB
				| Gdk::GL::MODE_DOUBLE
				| Gdk::GL::MODE_DEPTH );
			if (!config) {
				VPYTHON_CRITICAL_ERROR("failed to initialize any OpenGL configuration, Aborting.");
				std::exit(1);
			}
		}
	}
	if (share_list) {
		this->set_gl_capability(config, share_list, true);
	}
	else {
		set_gl_capability( config);
	}
	
	add_events( Gdk::EXPOSURE_MASK
		| Gdk::BUTTON_PRESS_MASK
		| Gdk::BUTTON_RELEASE_MASK
		| Gdk::POINTER_MOTION_MASK 
		| Gdk::ENTER_NOTIFY_MASK 
		| Gdk::BUTTON2_MOTION_MASK
		| Gdk::BUTTON3_MOTION_MASK);
	set_size_request( 384, 256);
	core.gl_begin.connect( SigC::slot( *this, &render_surface::gl_begin));
	core.gl_end.connect( SigC::slot( *this, &render_surface::gl_end));
	core.gl_swap_buffers.connect( 
		SigC::slot( *this, &render_surface::gl_swap_buffers));
	set_flags( get_flags() | Gtk::CAN_FOCUS);
}

void
render_surface::gl_free()
{
	gl_begin();
	try {
		clear_gl_error();
		on_gl_free();
		check_gl_error();
	}
	catch (gl_error& error) {
		VPYTHON_CRITICAL_ERROR( "Caught OpenGL error during shutdown: " + std::string(error.what()));
		std::cerr << "Continuing with the shutdown." << std::endl;
	}
	gl_end();
}

bool 
render_surface::on_motion_notify_event( GdkEventMotion* event)
{
	// Direct the core to perform rendering ops.
	
	// The vertical and horizontal changes of the mouse position since the last
	// call.
	float dy = static_cast<float>( event->y) - last_mousepos_y;
	float dx = static_cast<float>( event->x) - last_mousepos_x;
	bool buttondown = false;
	
	if (event->state & GDK_BUTTON2_MASK) {
		core.report_mouse_motion( dx, dy, display_kernel::MIDDLE);
		buttondown = true;
	}
	if (event->state & GDK_BUTTON3_MASK) {
		core.report_mouse_motion( dx, dy, display_kernel::RIGHT);
		buttondown = true;
	}
	else if (!buttondown) {
		core.report_mouse_motion( dx, dy, display_kernel::NONE);
	}
	mouse.set_shift( event->state & GDK_SHIFT_MASK);
	mouse.set_ctrl( event->state & GDK_CONTROL_MASK);
	mouse.set_alt( event->state & GDK_MOD1_MASK);
	last_mousepos_x = static_cast<float>(event->x);
	last_mousepos_y = static_cast<float>(event->y);
	mouse.cam = core.calc_camera();
	return true;
}

bool 
render_surface::on_enter_notify_event( GdkEventCrossing* event)
{
	last_mousepos_x = event->x;
	last_mousepos_y = event->y;
	return true;
}

bool 
render_surface::on_configure_event( GdkEventConfigure* event)
{
	core.report_resize( 
		static_cast<float>(event->width), 
		static_cast<float>(event->height));
	return true;
}

void
render_surface::on_realize()
{
	Gtk::GL::DrawingArea::on_realize();
	core.report_realize();
	if (!share_list) {
		share_list = get_gl_context();
	}
	assert( share_list);
	
	// Use PRIORITY_DEFAULT_IDLE to ensure that this signal does not get
	// priority over user-initiated GUI events, like mouse clicks and such.
	timer = Glib::signal_timeout().connect( 
		SigC::slot( *this, &render_surface::forward_render_scene),
		cycle_time, Glib::PRIORITY_DEFAULT_IDLE);
}

bool
render_surface::forward_render_scene()
{
	Glib::Timer time;
	bool sat = core.render_scene();
	double elapsed = time.elapsed();
	
	if (!sat)
		return sat;
	
	/* Scheduling logic for the rendering pulse.  This code is intended to make
	 * the rendering loop a better citizen with regard to sharing CPU time with
	 * the Python working thread.  If the time for one render pulse is
	 * greater than the timeout value, we raise the timeout by 5 ms to give the
	 * Python thread some more CPU time.  If it is more than 5 ms less than the
	 * timeout value, than the timeout is reduced by 5 ms, not to go below 30 ms.
	 */
	if (elapsed > double(cycle_time)/1000) {
		timer.disconnect();
		// Try to give the user process some minimal execution time.
		cycle_time += 5;
		timer = Glib::signal_timeout().connect( 
			SigC::slot( *this, &render_surface::forward_render_scene),
			cycle_time, Glib::PRIORITY_DEFAULT_IDLE);
		
		VPYTHON_NOTE( 
			std::string("Changed rendering cycle time to ") 
			+ boost::lexical_cast<std::string>(cycle_time) + "ms.");
		return false;
	}
	if (elapsed < double(cycle_time-5)/1000 && cycle_time > 30) {
		timer.disconnect();
		// Try to give the user process some minimal execution time.
		cycle_time -= 5;
		timer = Glib::signal_timeout().connect( 
			SigC::slot( *this, &render_surface::forward_render_scene),
			cycle_time, Glib::PRIORITY_DEFAULT_IDLE);
		
		VPYTHON_NOTE( 
			std::string("Changed rendering cycle time to ") 
			+ boost::lexical_cast<std::string>(cycle_time) + "ms.");
		return false;
		
	}
	return sat;
}

bool
render_surface::on_expose_event( GdkEventExpose*)
{
	core.render_scene();
	return true;
}

bool 
render_surface::on_button_press_event( GdkEventButton* event)
{
	last_mouseclick_x = event->x;
	last_mouseclick_y = event->y;
	
	
	return true;
}

bool 
render_surface::on_button_release_event( GdkEventButton* event)
{
	// Only handles the signal if:
	// 	- something has connected to object_picked.
	//  - the left mouse button is the source.
	//  - there are less than 4 pixels of mouse drag in the event.
	if (!object_clicked.empty()
		&& event->button == 1
		&& fabs(last_mouseclick_x - event->x) < 4
		&& fabs(last_mouseclick_y - event->y) < 4) {
		shared_ptr<renderable> pick 
			= core.pick( event->x, event->y);
		if (pick)
			object_clicked( pick);
	}
	return true;
}

void
render_surface::gl_begin()
{
	// TODO: Use the mouse motion signal hinting to poll its state at the end
	// of a render cycle and pass a single mouse_motion event for it.
	assert( get_gl_window()->gl_begin(get_gl_context()));
}

void
render_surface::gl_end()
{
	get_gl_window()->gl_end();
}

void
render_surface::gl_swap_buffers()
{
	get_gl_window()->swap_buffers();
}

basic_app::_init::_init()
{
	Gtk::GL::init(NULL, NULL);
}

basic_app::basic_app( const char* title)
	: kit( NULL, NULL), 
	fs_img( Gdk::Pixbuf::create_from_file( 
			VPYTHON_PREFIX "/data/galeon-fullscreen.png")),
	scene( _core)
{
	_core.illuminate_default();
	
	window.set_title( title);
	window.set_icon_from_file( VPYTHON_PREFIX "/data/logo_t.gif");
#ifndef VPYTHON_USE_GTKMM_24
	using namespace Gtk::Toolbar_Helpers;
	// Quit button	
	tb.tools().push_back( StockElem( 
		Gtk::Stock::QUIT,  
		SigC::slot( &Gtk::Main::quit),
		"Exit this program"));
	// Fullscreen toggle button.
	tb.tools().push_back( ToggleElem( 
		"Fullscreen",
		fs_img,
		SigC::slot( *this, &basic_app::on_fullscreen_clicked),
		"Toggle fullscreen on/off"));
	// The mouse control button group.
	Gtk::RadioButton::Group mouse_ctl;
	tb.tools().push_back( Space());
	tb.tools().push_back( RadioElem(
		mouse_ctl,
		"Rotate/Zoom",
		SigC::slot( *this, &basic_app::on_rotate_clicked)));
	tb.tools().push_back( RadioElem(
		mouse_ctl,
		"Pan",
		SigC::slot( *this, &basic_app::on_pan_clicked)));
#else
	Gtk::ToolButton* button = 0;
	button = Gtk::manage( new Gtk::ToolButton( Gtk::Stock::QUIT));
	button->signal_clicked().connect( sigc::ptr_fun( &Gtk::Main::quit));
	tb.append( *button);
	button = Gtk::manage( new Gtk::ToggleToolButton( fs_img, "Fullscreen"));
	button->signal_clicked().connect( 
		sigc::mem_fun( *this, &basic_app::on_fullscreen_clicked));
	tb.append( *button);
	Gtk::RadioButtonGroup mouse_ctl;
	button = Gtk::manage( new Gtk::RadioToolButton( mouse_ctl, "Rotate/Zoom"));
	button->signal_clicked().connect(
		sigc::mem_fun( *this, &basic_app::on_rotate_clicked));
	tb.append( *button);
	button = Gtk::manage( new Gtk::RadioToolButton( mouse_ctl, "Pan"));
	button->signal_clicked().connect(
		sigc::mem_fun( *this, &basic_app::on_pan_clicked));
	tb.append( *button);
	
#endif
	// Compose the frame and scen widgets.
	frame.pack_start( tb, Gtk::PACK_SHRINK, 0);
	frame.pack_start( scene);
	window.add( frame);
}

void
basic_app::run()
{
	scene.signal_delete_event().connect( 
		SigC::slot( *this, &basic_app::on_delete));
	window.show_all();
	kit.run(window);
}

void
basic_app::on_fullscreen_clicked()
{
	static bool fullscreen = false;
	if (fullscreen) {
		window.unfullscreen();
		fullscreen = false;
	}
	else {
		window.fullscreen();
		fullscreen = true;
	}
}

void
basic_app::on_pan_clicked()
{
	scene.core.mouse_mode = display_kernel::PAN;
}

void
basic_app::on_rotate_clicked()
{
	scene.core.mouse_mode = display_kernel::ZOOM_ROTATE;
}

bool
basic_app::on_delete( GdkEventAny*)
{
	scene.gl_free();
	return true;
}

} // !namespace cvisual
