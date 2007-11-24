#ifndef VPYTHON_GTK2_RENDER_SURFACE_HPP
#define VPYTHON_GTK2_RENDER_SURFACE_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "display_kernel.hpp"
#include "mouseobject.hpp"

// Part of the gtkglextmm package:
#include <gtkmm/gl/drawingarea.h>

#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <gtkmm/image.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/box.h>

namespace cvisual {

class render_surface : public Gtk::GL::DrawingArea
{
 private:
	// Note that the upper-left corner of the window is the origin.
	float last_mousepos_x;
	float last_mousepos_y;

	mousebutton left_button, right_button, middle_button;
	mouse_t mouse;
    
	// The length of the Glib::signal_timout, in milliseconds.
	long cycle_time;
	// Used to disconnect the timer when resetting the time.
	sigc::connection timer;
 	
 public:
	render_surface( display_kernel& _core, bool activestereo = false);
	display_kernel& core;
	
	// Returns the mouse object, and updates some of its parameters.
	mouse_t& get_mouse() { return mouse; }
 
 protected:
	// Low-level signal handlers
	// Called when the user moves the mouse across the screen.
	virtual bool on_motion_notify_event( GdkEventMotion* event);
	// Called when the window is resized.
	virtual bool on_configure_event( GdkEventConfigure* event);
	// Called when the mouse enters the scene.
	virtual bool on_enter_notify_event( GdkEventCrossing* event);
	// Called when the window is established for the first time.
 	virtual void on_realize();
	virtual bool on_expose_event( GdkEventExpose*);
	virtual bool on_button_press_event( GdkEventButton*);
	virtual bool on_button_release_event( GdkEventButton*);
 
 private:
	// Functions to be used as callbacks for connections via SigC::slots.
	void gl_begin();
	void gl_end();
	void gl_swap_buffers();
	bool forward_render_scene();
};

} // !namespace cvisual

#endif // !defined VPYTHON_GTK2_RENDER_SURFACE_HPP
