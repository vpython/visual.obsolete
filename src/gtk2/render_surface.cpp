#include "gtk2/render_surface.hpp"
#include "util/errors.hpp"
#include "vpython-config.h"

#include <gtkmm/gl/init.h>
#include <algorithm>
#include <iostream>

namespace {
	Glib::RefPtr<Gdk::GL::Context> share_list;
}

render_surface::render_surface()
{
	Glib::RefPtr<Gdk::GL::Config> config = Gdk::GL::Config::create( 
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
			VPYTHON_CRITICAL_ERROR("failed to initialize an OpenGL configuration.");
			std::exit(1);
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
	// core.stereo_mode = render_core::REDCYAN_STEREO;
}

void 
render_surface::add_renderable( shared_ptr<renderable> obj)
{
	if (obj->color.alpha == 1.0)
		core.layer_world.push_back( obj);
	else
		core.layer_world_transparent.push_back( obj);
}

void 
render_surface::add_renderable_screen( shared_ptr<renderable> obj)
{
	core.layer_screen.push_back( obj);
}
	
void 
render_surface::remove_renderable( shared_ptr<renderable> obj)
{
	// Choice of removal algorithms:  For containers that support thier own
	// removal methods (list, set), use the member function.  Else, use 
	// std::remove.
	if (obj->color.alpha != 1.0) {
		core.layer_world.remove( obj);
	}
	else
		std::remove( core.layer_world_transparent.begin(), 
			core.layer_world_transparent.end(), obj);
}
	
void 
render_surface::remove_renderable_screen( shared_ptr<renderable> obj)
{
	core.layer_screen.remove(obj);
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
		core.report_mouse_motion( dx, dy, render_core::MIDDLE);
		buttondown = true;
	}
	if (event->state & GDK_BUTTON3_MASK) {
		core.report_mouse_motion( dx, dy, render_core::RIGHT);
		buttondown = true;
	}
	else if (!buttondown) {
		core.report_mouse_motion( dx, dy, render_core::NONE);
	}
	last_mousepos_x = static_cast<float>(event->x);
	last_mousepos_y = static_cast<float>(event->y);
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
		share_list = get_gl_context()->get_share_list();
		// TODO: Fix this, it isn't working.  Find out why.
	}
	// assert( share_list);
	
	Glib::signal_timeout().connect( 
		SigC::slot( *this, &render_surface::forward_render_scene),
		28);
}

bool
render_surface::forward_render_scene()
{
	return core.render_scene();
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
	if (event->button == 1) {
		last_mouseclick_x = event->x;
		last_mouseclick_y = event->y;
	}
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
		shared_ptr<renderable> pick = core.pick( event->x, event->y);
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
	: kit( NULL, NULL)
	, fs_img( Gdk::Pixbuf::create_from_file( 
			VPYTHON_PREFIX "/data/galeon-fullscreen.png"))
{
	using namespace Gtk::Toolbar_Helpers;

	window.set_title( title);
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
	// Compose the frame and scen widgets.
	frame.pack_start( tb, Gtk::PACK_SHRINK, 0);
	frame.pack_start( scene);
	window.add( frame);
}

void
basic_app::run()
{
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
	scene.core.mouse_mode = render_core::PAN;
}

void
basic_app::on_rotate_clicked()
{
	scene.core.mouse_mode = render_core::ZOOM_ROTATE;
}
