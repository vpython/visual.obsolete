#ifndef VPYTHON_GTK2_DISPLAY_HPP
#define VPYTHON_GTK2_DISPLAY_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "display_kernel.hpp"

#include <boost/scoped_ptr.hpp>

#include <gtkmm/gl/drawingarea.h>
#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <gtkmm/image.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/box.h>
#include <glibmm.h>

namespace cvisual {
using boost::scoped_ptr;

// TODO: Implement userzoom, userspin, show, hide, project??, keyboard, and
// mouse events.
class display : public display_kernel,
	public SigC::Object
{
 private:
	scoped_ptr<Gtk::GL::DrawingArea> area;
	scoped_ptr<Gtk::Window> window;
	SigC::Connection timer;

	mutex mtx;
	float last_mousepos_x;
	float last_mousepos_y;
	bool active; // True when the display is actually visible
 
	// Properties of the main window.
	float x;
	float y;
	float width;
	float height;
	bool exit;
	bool visible; // True when the user has requested this window to be visible.
	std::string title;
 
	// The 'selected' display.
	static shared_ptr<display> selected;
	static Glib::RefPtr<Gdk::GL::Context> share_list;
	
 public:
	display();
	~display();

	void set_x( float x);
	float get_x();
 
	void set_y( float y);
	float get_y();
 
	void set_width( float w);
	float get_width();
 
	void set_height( float h);
	float get_height();
	
	void set_visible( bool v);
	bool get_visible();

	void set_title( std::string n_title);
	std::string get_title();

	virtual void add_renderable( shared_ptr<renderable>);
	virtual void add_renderable_screen( shared_ptr<renderable>);
	static void set_selected( shared_ptr<display> d);
	static shared_ptr<display> get_selected();
	
	// Called by the gtk2_main class below when this window needs to create
	// or destroy itself.
	void create();
	void destroy();
	
 private:
	// Signal handlers for the various widgets.
	void on_fullscreen_clicked();
	void on_pan_clicked();
	void on_rotate_clicked();
	bool on_window_delete(GdkEventAny*);
	void on_quit_clicked();
 
 	// Low-level signal handlers for the underlying Gtk::GL::DrawingArea.
	// Called when the user moves the mouse across the screen.
	bool on_motion_notify( GdkEventMotion* event);
	// Called when the window is resized.
	bool on_configure( GdkEventConfigure* event);
	// Called when the mouse enters the scene.
	bool on_enter_notify( GdkEventCrossing* event);
	// Called when the window is established for the first time.
 	void on_realize();
	// Called when the window has been exposed from the back, and should be 
	// painted again.
	bool on_expose( GdkEventExpose*);
	// Called when one of the mouse buttons has been pressed.
	// bool on_button_press( GdkEventButton*);
	// bool on_button_release( GdkEventButton*);
	// bool on_area_delete( GdkEventAny*);

	// Functions to be used as callbacks for connections via SigC::slots.
	void gl_begin();
	void gl_end();
	void gl_swap_buffers();
};

// A singlton.  This class provides all of the abstraction from the Gtk::Main
// object.
class gtk2_main : public SigC::Object
{
 private:
	Gtk::Main kit;
	
	Glib::Dispatcher signal_add_display;
	void add_display_impl();
	
	Glib::Dispatcher signal_remove_display;
	void remove_display_impl();
	
	Glib::Dispatcher signal_shutdown;
	void shutdown_impl();
	
	// Static storage initialized by the caller (itself) to be called with
	// the appropriate functions to add itself.
	mutex call_lock;
	condition call_complete;
	display* caller;
	bool returned;
	bool waiting_allclosed;
	
	std::list<display*> displays;
	
	gtk2_main();
	void run();
	static gtk2_main* self;
	static void thread_proc(void);
	static void init_thread(void);

 public:
	// (Nonblocking) Returns true if all of the active displays have been
	// shut down
	static bool allclosed();
	// Waits until all displays have been closed by the user.
	static void waitclosed();
	// Force all displays to close and exit the Gtk event loop.
	static void shutdown();
	
	// Called by a display to make it visible, or invisible.
	static void add_display( display*);
	static void remove_display( display*);
	
	// Called by a display from within the Gtk loop when closed by the user.
	static void report_window_delete( display*);
	static void quit();
};

} // !namespace cvisual

#endif // !defined VPYTHON_GTK2_DISPLAY_HPP
