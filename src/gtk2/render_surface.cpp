// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "gtk2/render_surface.hpp"
#include "util/errors.hpp"
#include "util/gl_free.hpp"
#include "vpython-config.h"

// The following are part of the gtkglextmm package:
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
#include <sstream>
#include <iostream>
#include <cassert>

#include <boost/lexical_cast.hpp>

namespace cvisual {

namespace {
	Glib::RefPtr<Gdk::GL::Context> share_list;
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

bool 
render_surface::on_motion_notify_event( GdkEventMotion* event)
{
	// Direct the core to perform rendering ops.
#ifdef G_OS_WIN32
	// Fake middle mouse events on Win32
	if (event->state & GDK_BUTTON3_MASK && event->state & GDK_BUTTON1_MASK) {
		event->state |= GDK_BUTTON2_MASK;
		event->state &= ~(GDK_BUTTON3_MASK | GDK_BUTTON1_MASK);
	}
#endif
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
	if (!core.zoom_is_allowed() && middle_button.is_dragging())
		mouse.push_event( drag_event( 2, mouse));
	if (!core.spin_is_allowed() && right_button.is_dragging())
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
	// double scene_elapsed = time.elapsed();
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
	 
	// std::cout << scene_elapsed << " " << (elapsed-scene_elapsed) << " " << cycle_time << std::endl;
	if (elapsed > double(cycle_time + 5)/1000) {
		timer.disconnect();
		// Try to give the user process some minimal execution time.
		cycle_time += 5;
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
		// Can render again sooner than current cycle_time.
		cycle_time -= 5;
		if (cycle_time < 30) cycle_time = 30;
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
	bool clickok = false;
	switch (event->button) {
		case 1:
			clickok = true;
			drop = left_button.release().second;
			break;
		case 2:
			if (!core.zoom_is_allowed())
				clickok = true;
				drop = middle_button.release().second;
			break;
		case 3:
			if (!core.spin_is_allowed())
				clickok = true;
				drop = right_button.release().second;
			break;
		default:
			// Captured above
			break;
	}
	if (drop)
		mouse.push_event( drop_event( event->button, mouse));
	else if (clickok)
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

} // !namespace cvisual
