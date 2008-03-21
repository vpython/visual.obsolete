// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "gtk2/render_surface.hpp"
#include "util/errors.hpp"
#include "util/gl_free.hpp"
#include "vpython-config.h"
#include "python/gil.hpp"

#if !(defined(_WIN32) || defined(_MSC_VER))
#include <sys/time.h>
#include <sys/resource.h>
#endif

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
#include <boost/bind.hpp>

#if defined(_WIN32) || defined(_MSC_VER)
	#include <windows.h>
#endif

const long TIMEOUT = 35; // delay before next rendering

namespace cvisual {

namespace {
	Glib::RefPtr<Gdk::GL::Context> share_list;
}

render_surface::render_surface( display_kernel& _core, mouse_manager& _mouse, bool activestereo)
	: 
	cycle_time(TIMEOUT),
	last_time(0),
	core( _core),
	mouse( _mouse )
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

	set_flags( get_flags() | Gtk::CAN_FOCUS);
}

template <class E>
void 
render_surface::mouse_event( E* event, int buttons_toggled ) {
	bool buttons[] = { event->state & GDK_BUTTON1_MASK, event->state & GDK_BUTTON3_MASK };
	bool shiftState[] = { event->state & GDK_SHIFT_MASK, event->state & GDK_CONTROL_MASK, event->state & GDK_MOD1_MASK };
	
	buttons[0] = (buttons[0] != bool( buttons_toggled&1 ));
	buttons[1] = (buttons[1] != bool( buttons_toggled&4 ));

	mouse.report_mouse_state( 2, buttons, event->x, event->y, 3, shiftState, false );

	// xxx Is mouse locking possible with GTK2?
}

bool 
render_surface::on_motion_notify_event( GdkEventMotion* event)
{
	python::gil_lock L;
	mouse_event( event );
	return true;
}

bool 
render_surface::on_enter_notify_event( GdkEventCrossing* event)
{
	python::gil_lock L;
	mouse_event( event );
	return true;
}

bool 
render_surface::on_configure_event( GdkEventConfigure* event)
{
	python::gil_lock L;
	int x,y,w,h;
	get_parent_window()->get_position( x, y );
	get_parent_window()->get_size(w,h);
	core.report_resize( 
		x,y,w,h,
		x+event->x, y+event->y, event->width, event->height );
	return true;
}

void
render_surface::on_realize()
{
	python::gil_lock L;
	Gtk::GL::DrawingArea::on_realize();

	if (!share_list) {
		share_list = get_gl_context();
	}
	assert( share_list);
	
	// Use PRIORITY_DEFAULT_IDLE to ensure that this signal does not get
	// priority over user-initiated GUI events, like mouse clicks and such.
	timer = Glib::signal_timeout().connect( 
		sigc::mem_fun( *this, &render_surface::forward_render_scene),
		cycle_time, Glib::PRIORITY_HIGH_IDLE);
		//cycle_time, Glib::PRIORITY_DEFAULT_IDLE-10);
		//cycle_time, Glib::PRIORITY_HIGH);
}

bool
render_surface::forward_render_scene()
{
	python::gil_lock L;

	// TODO: Use the mouse motion signal hinting to poll its state at the end
	// of a render cycle and pass a single mouse_motion event for it.

// Make sure this rendering thread has high priority:
#if defined(_WIN32) || defined(_MSC_VER)
	//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#else
	int default_prio = getpriority(PRIO_PROCESS, getpid());
	setpriority(PRIO_PROCESS, getpid(), std::max(default_prio -5, -20));
#endif

	double start_time = stopwatch.elapsed();
	double cycle = start_time - last_time;
	last_time = start_time;
	gl_begin();
	bool sat = core.render_scene(); // render the scene
	gl_swap_buffers();
	gl_end();
	double render_time = stopwatch.elapsed() - start_time;

	if (!sat)
		return sat;

	double pick_time = stopwatch.elapsed() - start_time - render_time;
	long irender = int(1000*(render_time+pick_time));
	long icycle = int(1000*cycle);

#if 0
	// cycle_time, one actual cycle, 
	// render_time, pick_time, render+pick, estimate of computation during cycle
	std::cout << cycle_time << " " << icycle << " " << 
		int(1000*render_time) << " " << int(1000*pick_time) << " " <<
		irender << " " << 
		(icycle-irender) << std::endl;
#endif

	/* Scheduling logic for the rendering pulse. This code is intended to make
	 * the rendering loop a good citizen with regard to sharing CPU time with
	 * the Python computational thread. If the time for one render pulse is
	 * large (small) compared to the most recent cycle_time specification, 
	 * we increase (decrease) the timeout, not to go below TIMEOUT ms or 
	 * above 500 ms. 
	 * 
	 * A refinement would be to run rendering full-time if computation has
	 * stopped (e.g. at the end of a program, or when waiting for an event).
	 */
	 
	long old_cycle_time = cycle_time;
	if ((icycle-irender) < (TIMEOUT/2))
		cycle_time += 16;
	else if ((icycle-irender) > TIMEOUT) 
		cycle_time = irender+16;
	if (cycle_time < TIMEOUT) cycle_time = TIMEOUT;
	if (cycle_time > 500) cycle_time = 500;
	if (cycle_time != old_cycle_time) {	
		// Adjust last_time because next cycle starts now:
		last_time += render_time+pick_time;
		timer.disconnect();
		timer = Glib::signal_timeout().connect( 
			sigc::mem_fun( *this, &render_surface::forward_render_scene),
			cycle_time, Glib::PRIORITY_HIGH_IDLE);
			//cycle_time, Glib::PRIORITY_DEFAULT_IDLE-10);
			//cycle_time, Glib::PRIORITY_HIGH);
		
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
	python::gil_lock L;
	
	gl_begin();
	core.render_scene(); // render the scene
	gl_swap_buffers();
	gl_end();

	return true;
}

bool 
render_surface::on_button_press_event( GdkEventButton* event)
{
	python::gil_lock L;
	mouse_event( event, 1 << (event->button-1) );
	return true;
}

bool 
render_surface::on_button_release_event( GdkEventButton* event)
{
	python::gil_lock L;
	mouse_event( event, 1 << (event->button-1) );
	return true;
}

void
render_surface::gl_begin()
{
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
