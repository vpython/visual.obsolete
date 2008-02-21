#ifndef VPYTHON_GTK2_DISPLAY_HPP
#define VPYTHON_GTK2_DISPLAY_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "display_kernel.hpp"
#include "gtk2/render_surface.hpp"
#include "util/atomic_queue.hpp"
#include "mouseobject.hpp"

#include <boost/scoped_ptr.hpp>


#include <gtkmm/gl/drawingarea.h>
#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <gtkmm/image.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/box.h>
#include <libglademm/xml.h>

namespace cvisual {
using boost::scoped_ptr;

// TODO: Implement userzoom, userspin, show, hide, keyboard, uniform, and
// mouse events.
class display : public display_kernel,
	public sigc::trackable
{
 private:
	scoped_ptr<render_surface> area;
	Glib::RefPtr<Gnome::Glade::Xml> glade_file;
	Gtk::Window* window;
	sigc::connection timer;

	mutex mtx;
	bool active; ///< True when the display is actually visible
 
	// Properties of the main window.
	float x;
	float y;
	float width;
	float height;
	bool exit; ///< True when Visual should shutdown on exit.
	bool visible; ///< True when the user has requested this window to be visible.
	bool fullscreen; ///< True when the display is in fullscreen mode.
	bool show_toolbar; ///< True when toolbar is displayed (pan, etc).
	std::string title;
 
	// The 'selected' display.
	static shared_ptr<display> selected;
		
	// The interface for reading keyboard presses from this display in Python.
	atomic_queue<std::string> keys;
	
 public:
	display();
	virtual ~display();

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
 
	bool is_fullscreen();
	void set_fullscreen( bool);
 
	bool is_showing_toolbar();
	void set_show_toolbar( bool);

	virtual void add_renderable( shared_ptr<renderable>);
	virtual void add_renderable_screen( shared_ptr<renderable>);
	
	static void set_selected( shared_ptr<display> d);
	static shared_ptr<display> get_selected();
	
	static int get_titlebar_height();
	static int get_toolbar_height();
	
	static void set_dataroot( Glib::ustring dataroot);
	
	// Called by the gui_main class below when this window needs to create
	// or destroy itself.
	void create();
	void destroy();
	
	atomic_queue<std::string>& get_kb();
	mouse_t* get_mouse();
	
 private:
	// Signal handlers for the various widgets.
	void on_fullscreen_clicked();
	void on_pan_clicked();
	void on_rotate_clicked();
	bool on_window_delete( GdkEventAny*);
	void on_quit_clicked();
	void on_zoom_clicked();
	bool on_key_pressed( GdkEventKey*);
};

// A singleton.  This class provides all of the abstraction from the Gtk::Main
// object, in addition to providing asynchronous communication channels between
// threads.
class gui_main : public sigc::trackable
{
 private:
	Gtk::Main kit;
	
	Glib::Dispatcher signal_add_display;
	void add_display_impl();
	
	Glib::Dispatcher signal_remove_display;
	void remove_display_impl();
	
	Glib::Dispatcher signal_shutdown;
	void shutdown_impl();
	
	// Storage used for communication, initialized by the caller, filled by the
	// callee.  Some of them are marked volitile to inhibit optimizations that
	// could prevent a read operation from observing the change in state.
	mutex call_lock;
	condition call_complete;
	display* caller;
	volatile bool returned;
	volatile bool waiting_allclosed;
	volatile bool thread_exited;
	volatile bool shutting_down;
	
	std::list<display*> displays;
	
	// Componants of the startup sequence.
	gui_main();
	void run();
	static gui_main* self; //< Always initialized by the thread after it starts
	// up.
	// init_{signal,lock} are always initialized by the Python thread.
	static mutex* init_lock;
	static condition* init_signal;
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
	// This signal is invoked when the gui thread exits on shutdown.
	// wrap_display_kernel() connects a signal handler that forces Python to
	// exit upon shutdown of the render loop.
	static boost::signal<void()> on_shutdown;
};

} // !namespace cvisual

#endif // !defined VPYTHON_GTK2_DISPLAY_HPP
