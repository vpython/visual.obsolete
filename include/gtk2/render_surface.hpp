#ifndef VPYTHON_GTK2_RENDER_SURFACE_HPP
#define VPYTHON_GTK2_RENDER_SURFACE_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "display_kernel.hpp"
#include "mouseobject.hpp"

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

	// Track mouse press, release, clicks, drags, and drops.
	struct mousebutton
	{
		bool down;
		bool dragging;
		float last_down_x;
		float last_down_y;
		
		mousebutton() 
			: down(false), dragging(false), 
			last_down_x(-1.0f), last_down_y(-1.0f) {}
		
		// When the button is pressed, call this function with its screen
		// coordinate position
		void press( float x, float y)
		{
			down = true;
			last_down_x = x;
			last_down_y = y;
			dragging = false;
		}
		
		// Returns true when a drag event should be generated, false otherwise
		bool is_dragging() 
		{
			if (down && !dragging) {
				dragging = true;
				return true;
			}
			return false;
		}
		
		// Returns true for drops, false for clicks
		bool release()
		{
			down = false;
			last_down_x = -1;
			last_down_y = -1;
			return dragging;
		}
	} left_button, right_button, middle_button;
	
	mouse_t mouse;
    
	// The length of the Glib::signal_timout, in milliseconds.
	long cycle_time;
	// Used to disconnect the timer when resetting the time.
	sigc::connection timer;
 	
 public:
	render_surface( display_kernel& _core, bool activestereo = false);
	display_kernel& core;
	
	// Signal fired by button down + button up.
	sigc::signal2<void, shared_ptr<renderable>, vector> object_clicked;
	
	// Makes this rendering context active and calls on_gl_free().  This should
	// generally be done only by the last window to shut down.
	void gl_free();
	
	// Returns the mouse object, and updates some of its parameters.
	mouse_t& get_mouse() { return mouse; }
	
	// To be called before the GUI thread exits.
	static void final_cleanup( void);
 
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

class basic_app : public sigc::trackable
{
 private:
	/** Initialize Gtkmm */
	Gtk::Main kit;
	/** Helper class used to initialize Gtk::GL at the right time. */
	struct _init
	{
		_init();
	} init;
	
	/** The parent window. */
	Gtk::Window window;
	/** An image widget for one of the toolbars.  It loads the fullscreen
	 * toggle image, shamelessly ripped from Galeon.
	 */
	Gtk::Image fs_img;
	Gtk::Toolbar tb;
	Gtk::VBox frame;
	display_kernel _core;
	
 private:
	void on_fullscreen_clicked();
	void on_pan_clicked();
	void on_rotate_clicked();
	bool on_delete(GdkEventAny*);
 
 public:
	render_surface scene;
	basic_app( const char* title);
	void run();	
};

} // !namespace cvisual

#endif // !defined VPYTHON_GTK2_RENDER_SURFACE_HPP
