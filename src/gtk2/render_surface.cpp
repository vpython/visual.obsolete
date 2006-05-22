// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "gtk2/render_surface.hpp"
#include "util/errors.hpp"
#include "util/gl_free.hpp"
#include "vpython-config.h"

#include <gtkmm/gl/init.h>
#include <gdkmm/gl/pixmap.h>
#include <gdkmm/gl/pixmapext.h>

#include <gdkmm/pixmap.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/stock.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/toolbar.h>

#include <gtkmm/toggletoolbutton.h>
#include <gtkmm/radiotoolbutton.h>

#include <algorithm>
#include <iostream>
#include <cassert>

#include <boost/lexical_cast.hpp>

namespace cvisual {

namespace {
	Glib::RefPtr<Gdk::GL::Context> share_list;
}

void
render_surface::final_cleanup( void)
{
	if (share_list) {
		try {
			Glib::RefPtr<Gdk::GL::Config> config = 
				Gdk::GL::Config::create( Gdk::GL::MODE_RGB
					| Gdk::GL::MODE_DOUBLE
					| Gdk::GL::MODE_DEPTH );
			if (!config) {
				VPYTHON_WARNING( "Failed to initialize any OpenGL configuration");
				return;
			}
			
			Glib::RefPtr<Gdk::Pixmap> dummy_target = 
				Gdk::Pixmap::create( Glib::RefPtr<Gdk::Drawable>(), 1, 1, config->get_depth());
			if (!dummy_target) {
				VPYTHON_WARNING( "Failed to create a dummy Gdk::Pixmap");
				return;
			}
			
			Glib::RefPtr<Gdk::GL::Pixmap> target = 
				Gdk::GL::ext( dummy_target).set_gl_capability( config);
			if (!target) {
				VPYTHON_WARNING( "Failed to create a dummy Gdk::GL::Pixmap");
				return;
			}
			
			Glib::RefPtr<Gdk::GL::Context> local_ctx = Gdk::GL::Context::create(
				target, share_list, false);
			if (!local_ctx) {
				VPYTHON_WARNING( "Failed to create a new shared Gdk::GL::Context");
				return;
			}
			
			if (!target->gl_begin( local_ctx)) {
				VPYTHON_WARNING( "Failed to gl_begin() the dummy Gdk::GL::Pixmap");
				return;
			}
			
			clear_gl_error();
			on_gl_free();
			check_gl_error();
			
			target->gl_end();
		} catch (std::exception& err) {
			std::ostringstream msg;
			msg << "Failed releasing GL resources: " << err.what();
			VPYTHON_CRITICAL_ERROR( msg.str());
		}
		share_list.clear();
	}
	else
		VPYTHON_NOTE( "No renderer was made active; nothing to clean up");
}

render_surface::render_surface( display_kernel& _core, bool activestereo)
	: last_mousepos_x(0),
	last_mousepos_y(0),
	cycle_time(30),
	core( _core)
{
	Glib::RefPtr<Gdk::GL::Config> config;

	if (activestereo) {
		config = Gdk::GL::Config::create( 
			Gdk::GL::MODE_RGBA
			| Gdk::GL::MODE_DOUBLE
			| Gdk::GL::MODE_DEPTH
			//| Gdk::GL::MODE_MULTISAMPLE
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
			//| Gdk::GL::MODE_MULTISAMPLE
			);
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
		// | Gdk::ENTER_NOTIFY_MASK //< TODO: Find out why this doesn't work.
		| Gdk::BUTTON2_MOTION_MASK
		| Gdk::BUTTON3_MOTION_MASK);
	set_size_request( 384, 256);
	core.gl_begin.connect( sigc::mem_fun( *this, &render_surface::gl_begin));
	core.gl_end.connect( sigc::mem_fun( *this, &render_surface::gl_end));
	core.gl_swap_buffers.connect( 
		sigc::mem_fun( *this, &render_surface::gl_swap_buffers));
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
	if (!buttondown) {
		core.report_mouse_motion( dx, dy, display_kernel::NONE);
	}
	mouse.set_shift( event->state & GDK_SHIFT_MASK);
	mouse.set_ctrl( event->state & GDK_CONTROL_MASK);
	mouse.set_alt( event->state & GDK_MOD1_MASK);
	last_mousepos_x = static_cast<float>(event->x);
	last_mousepos_y = static_cast<float>(event->y);
	mouse.cam = core.calc_camera();
	
	
	if (left_button.is_dragging())
		mouse.push_event( drag_event( 1, mouse));
	if (middle_button.is_dragging())
		mouse.push_event( drag_event( 2, mouse));
	if (right_button.is_dragging())
		mouse.push_event( drag_event( 3, mouse));
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
		sigc::mem_fun( *this, &render_surface::forward_render_scene),
		cycle_time, Glib::PRIORITY_DEFAULT_IDLE);
}

bool
render_surface::forward_render_scene()
{
	Glib::Timer time;
	bool sat = core.render_scene();
	if (!sat)
		return sat;
	// TODO: Figure out an optimzation to avoid performing this extra pick
	// rendering cycle when the user isn't looking at scene.mouse[.{pick,pickpos,pos}]
	boost::tie( mouse.pick, mouse.pickpos, mouse.position) = 
		core.pick( last_mousepos_x, last_mousepos_y);

#if 1
	double elapsed = time.elapsed();
	/* Scheduling logic for the rendering pulse.  This code is intended to make
	 * the rendering loop a better citizen with regard to sharing CPU time with
	 * the Python working thread.  If the time for one render pulse is
	 * greater than the timeout value, we raise the timeout by 5 ms to give the
	 * Python thread some more CPU time.  If it is more than 5 ms less than the
	 * timeout value, than the timeout is reduced by 5 ms, not to go below 30 ms.
	 */
	if (elapsed > double(cycle_time + 5)/1000) {
		timer.disconnect();
		// Try to give the user process some minimal execution time.
		cycle_time = int(elapsed * 1000) + 5;
		timer = Glib::signal_timeout().connect( 
			sigc::mem_fun( *this, &render_surface::forward_render_scene),
			cycle_time, Glib::PRIORITY_DEFAULT_IDLE);
		
		VPYTHON_NOTE( 
			std::string("Changed rendering cycle time to ") 
			+ boost::lexical_cast<std::string>(cycle_time) + "ms.");
		return false;
	}
	if (elapsed < double(cycle_time-5)/1000 && cycle_time > 30) {
		timer.disconnect();
		// Try to give the user process some minimal execution time.
		cycle_time = int(elapsed * 1000) + 5;
		timer = Glib::signal_timeout().connect( 
			sigc::mem_fun( *this, &render_surface::forward_render_scene),
			cycle_time, Glib::PRIORITY_DEFAULT_IDLE);
		
		VPYTHON_NOTE( 
			std::string("Changed rendering cycle time to ") 
			+ boost::lexical_cast<std::string>(cycle_time) + "ms.");
		return false;
		
	}
#endif
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
	if (event->type == GDK_BUTTON_RELEASE)
		// Ignore erronious condition
		return true;
	if (event->button > 3)
		// Ignore extra buttons (such as scroll wheel and forward/back)
		return true;
	mouse.set_shift( event->state & GDK_SHIFT_MASK);
	mouse.set_ctrl( event->state & GDK_CONTROL_MASK);
	mouse.set_alt( event->state & GDK_MOD1_MASK);
	switch (event->button) {
		case 1: // Left
			left_button.press( event->x, event->y);
			break;
		case 2: // Middle
			middle_button.press( event->x, event->y);
			break;
		case 3: // Right
			right_button.press( event->x, event->y);
			break;
		default:
			// Captured above
			break;
	}
	// Generate a press event.
	mouse.push_event( press_event( event->button, mouse));
	return true;
}

bool 
render_surface::on_button_release_event( GdkEventButton* event)
{
	if (!object_clicked.empty() && mouse.pick
			&& event->button == 1) {
		object_clicked( mouse.pick, mouse.pickpos);
	}
	
	if (event->type != GDK_BUTTON_RELEASE)
		// Ignore erronious condition
		return true;
	if (event->button > 3)
		// Ignore extra buttons (such as scroll wheel and forward/back)
		return true;
	mouse.set_shift( event->state & GDK_SHIFT_MASK);
	mouse.set_ctrl( event->state & GDK_CONTROL_MASK);
	mouse.set_alt( event->state & GDK_MOD1_MASK);
	bool drop = false;
	switch (event->button) {
		case 1:
			drop = left_button.release();
			break;
		case 2:
			drop = middle_button.release();
			break;
		case 3:
			drop = right_button.release();
			break;
		default:
			// Captured above
			break;
	}
	if (drop)
		mouse.push_event( drop_event( event->button, mouse));
	else
		mouse.push_event( click_event( event->button, mouse));
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

#if 0
mouse_t&
render_surface::get_mouse()
{
    watching_mouse = true;
    // Set a condition variable to wait
}
#endif

////////////////////////////////////////////////////////////////////////////////
#if 0
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
	
	// Compose the frame and scen widgets.
	frame.pack_start( tb, Gtk::PACK_SHRINK, 0);
	frame.pack_start( scene);
	window.add( frame);
}

void
basic_app::run()
{
	scene.signal_delete_event().connect( 
		sigc::mem_fun( *this, &basic_app::on_delete));
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
#endif

} // !namespace cvisual
