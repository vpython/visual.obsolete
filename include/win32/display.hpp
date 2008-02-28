#ifndef VPYTHON_WIN32_DISPLAY_HPP
#define VPYTHON_WIN32_DISPLAY_HPP

#include "display_kernel.hpp"
#include "mouseobject.hpp"
#include "util/thread.hpp"
#include <map>
#include <string>

namespace cvisual {

class display : public display_kernel
{
 private:
	friend class font;
	static VOID CALLBACK
		timer_callback( HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
	static LRESULT CALLBACK
		dispatch_messages( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static display* current;

	// Properties of the main window.
	float x;
	float y;
	bool exit; ///< True when Visual should shutdown on window close.
	bool visible; ///< True when the user has requested this window to be visible.
	bool fullscreen; ///< True when the display is in fullscreen mode.
	bool show_toolbar; ///< True when toolbar is displayed (pan, etc).
	std::string title;

	// For the benefit of win32::bitmap_font.
	float window_width;
	float window_height;

 	HWND widget_handle;
 	UINT_PTR timer_handle;
 	HDC dev_context;
 	HGLRC gl_context;

 	HDC saved_dc;
 	HGLRC saved_glrc;

	// A callback function to handle the render pulse.

	static void register_win32_class();
	static WNDCLASS win32_class;
	static std::map<HWND, display*> widgets;

	// Procedures used to process messages.
	LRESULT on_showwindow( WPARAM, LPARAM);
	LRESULT on_mousemove( WPARAM, LPARAM);
	LRESULT on_paint( WPARAM, LPARAM);
	LRESULT on_close( WPARAM, LPARAM);
	LRESULT on_buttondown( WPARAM, LPARAM);
	LRESULT on_buttonup( WPARAM, LPARAM);
	LRESULT on_getminmaxinfo( WPARAM, LPARAM);
	LRESULT on_keyUp( UINT, WPARAM, LPARAM);
	LRESULT on_keyDown( UINT, WPARAM, LPARAM);
	LRESULT on_keyChar( UINT, WPARAM, LPARAM);
	LRESULT on_size( WPARAM, LPARAM);
	LRESULT on_move( WPARAM, LPARAM);
	
	//boolean variables keeping track of Shift, Ctrl and Alt
	bool Kshift;
	bool Kalt;
	bool Kctrl;

	// Callbacks provided to the display_kernel object.
	void on_gl_begin();
	void on_gl_end();
	void on_gl_swap_buffers();

	mousebutton left_button, right_button, middle_button;
	mouse_t mouse;
	float last_mousepos_x;
 	float last_mousepos_y;
 	bool mouselocked;

 	bool active; ///< True when the display is actually visible


	// The interface for reading keyboard presses from this display in Python.
	atomic_queue<std::string> keys;

 public:
	display();
	virtual ~display();

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
 
	bool is_showing_toolbar();
	void set_show_toolbar( bool);
	
	static int get_titlebar_height();
	static int get_toolbar_height();

	virtual void add_renderable( shared_ptr<renderable>);
	virtual void add_renderable_screen( shared_ptr<renderable>);

	// Tells the application where it can find its data.  Win32 doesn't
	// use this information;
	static void set_dataroot( std::string) {};

	atomic_queue<std::string>* get_kb();
	mouse_t* get_mouse();
	
	EXTENSION_FUNCTION getProcAddress( const char* name );

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
	static boost::signal<void()> on_shutdown;
};

} // !namespace cvisual

#endif /*VPYTHON_WIN32_DISPLAY_HPP*/
