#ifndef VPYTHON_WIN32_DISPLAY_HPP
#define VPYTHON_WIN32_DISPLAY_HPP

#include "display_kernel.hpp"
#include "win32/render_surface.hpp"

namespace cvisual {

typedef render_surface display;

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
	static boost::signal<void()> on_shutdown;
};

} // !namespace cvisual

#endif /*VPYTHON_WIN32_DISPLAY_HPP*/
