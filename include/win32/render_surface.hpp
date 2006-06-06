#ifndef VPYTHON_WIN32_RENDER_SURFACE_HPP
#define VPYTHON_WIN32_RENDER_SURFACE_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#define _WIN32_IE 0x400
#include <commctrl.h>

#include "display_kernel.hpp"
#include "util/thread.hpp"
#include <map>
#include <string>
#include <sigc++/object.h>

namespace cvisual {

// NOTE: This implementation might require Windows 98 (For the timer callback).
class bitmap_font;

class render_surface : public display_kernel
{
 private:
	friend class bitmap_font;
	static render_surface* current;
	
	// Properties of the main window.
	float x;
	float y;
	bool exit; ///< True when Visual should shutdown on window close.
	bool visible; ///< True when the user has requested this window to be visible.
	bool fullscreen; ///< True when the display is in fullscreen mode.
	std::string title;
	
	// For the benefit of win32::bitmap_font.
	float window_width;
	float window_height;
	
 	HWND widget_handle;
 	UINT_PTR timer_handle;
 	HDC dev_context;
 	HGLRC gl_context;
 	
 	// A callback function to dispatch normal messages from the system.
	static LRESULT CALLBACK 
	dispatch_messages( HWND hwnd, UINT uMsg, WPARAM wParam,	LPARAM lParam);
	
	// A callback function to handle the render pulse.
	static VOID CALLBACK
	timer_callback( HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
	
	static void register_win32_class();
	static WNDCLASS win32_class;
	static std::map<HWND, render_surface*> widgets;
	
	// Procedures used to process messages.
	LRESULT on_showwindow( WPARAM, LPARAM);
	LRESULT on_mousemove( WPARAM, LPARAM);
	LRESULT on_size( WPARAM, LPARAM);
	LRESULT on_paint( WPARAM, LPARAM);
	LRESULT on_destroy( WPARAM, LPARAM);
	LRESULT on_buttondown( WPARAM, LPARAM);
	LRESULT on_buttonup( WPARAM, LPARAM);
	// LRESULT on_getminmaxinfo( WPARAM, LPARAM);
	// LRESULT on_keypress( UINT, WPARAM, LPARAM);
	LRESULT on_windowmove( WPARAM, LPARAM);
	
	// Callbacks provided to the display_kernel object.
	void gl_begin();
	void gl_end();
	void gl_swap_buffers();
	
	mousebutton left_button, right_button, middle_button;
	mouse_t mouse;
	float last_mousepos_x;
 	float last_mousepos_y;
 	bool mouselocked;
 	
 	bool active; ///< True when the display is actually visible
 	
	
	// The interface for reading keyboard presses from this display in Python.
	atomic_queue<std::string> keys;
	
 public:
	render_surface();
	virtual ~render_surface();

	// Makes this rendering context active and calls on_gl_free().  This should
	// generally be done only by the last window to shut down, however it is
	// harmless to call it more than once during the shutdown.  Attempting to 
	// render after this is called will probably not work...
	void gl_free( void);
	
	// Called by the gui_main class below when this window needs to create
	// or destroy itself.
	void create();
	void destroy();
	
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
	
	virtual void add_renderable( shared_ptr<renderable>);
	virtual void add_renderable_screen( shared_ptr<renderable>);
	
	// Tells the application where it can find its data.  Win32 doesn't
	// use this information;
	static void set_dataroot( Glib::ustring) {};
	
	atomic_queue<std::string>* get_kb();
	mouse_t* get_mouse();
	
	// The 'selected' display.
 private:
	static shared_ptr<display> selected;
 public:	
	static void set_selected( shared_ptr<display> d);
	static shared_ptr<display> get_selected();

};

class gui_main
{
 private:
	// Message types for the three signals
 	enum msg_id {
 		VPYTHON_ADD_DISPLAY = WM_USER,
 		VPYTHON_REMOVE_DISPLAY,
 		VPYTHON_SHUTDOWN
 	};
 	
 	// Signal functions, called from the Python thread
 	void signal_add_display();
 	void signal_remove_display();
 	void signal_shutdown();
 	
 	// And their implementations, executed in the Win32 thread
 	void add_display_impl();
	void remove_display_impl();
	void shutdown_impl();
	
	// Storage used for communication, initialized by the caller, filled by the
	// callee.  Some of them are marked volitile to inhibit optimizations that
	// could prevent a read operation from observing the change in state.
 	DWORD idThread;
	mutex call_lock;
	condition call_complete;
	render_surface* caller;
	volatile bool returned;
	volatile bool waiting_allclosed;
	volatile bool thread_exited;
	volatile bool shutting_down;
	
	std::list<render_surface*> displays;
	
	// Componants of the startup sequence.
	gui_main();
	void run();
	static gui_main* self; //< Always initialized by the thread after it starts
	// up.
	// init_{signal,lock} are always initialized by the Python thread.
	static mutex* volatile init_lock;
	static condition* volatile init_signal;
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
	static sigc::signal0<void> on_shutdown;
};

} // !namespace cvisual

#endif // !defined VPYTHON_WIN32_RENDER_SURFACE_HPP
