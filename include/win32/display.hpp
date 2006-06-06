#ifndef VPYTHON_WIN32_DISPLAY_HPP
#define VPYTHON_WIN32_DISPLAY_HPP

#include "display_kernel.hpp"
#include "win32/render_surface.hpp"

namespace cvisual {

class display : public display_kernel
{
 private:
// TODO: Does this actually have to have a new window class type?
// Yes: To handle keyboard events, at least.
	HWND window_handle;
	static LRESULT CALLBACK 
	dispatch_messages( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	static void register_win32_class();
	static WNDCLASS win32_class;
	static std::map<HWND, basic_app*> windows;
	
	// Message handlers
	LRESULT on_size( WPARAM, LPARAM);
	LRESULT on_create( WPARAM, LPARAM);
	LRESULT on_command( WPARAM, LPARAM);
	LRESULT on_getminmaxinfo( WPARAM, LPARAM);
	
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
	
	virtual void add_renderable( shared_ptr<renderable>);
	virtual void add_renderable_screen( shared_ptr<renderable>);
	
	static void set_selected( shared_ptr<display> d);
	static shared_ptr<display> get_selected();
	
	static void set_dataroot( Glib::ustring dataroot);
	
	atomic_queue<std::string>& get_kb();
	mouse_t* get_mouse();
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
 	
 	void add_display_impl();
	void remove_display_impl();
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
	static sigc::signal0<void> on_shutdown;
};

} // !namespace cvisual

#endif /*VPYTHON_WIN32_DISPLAY_HPP*/
