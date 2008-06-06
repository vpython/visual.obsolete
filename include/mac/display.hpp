#ifndef VPYTHON_MAC_DISPLAY_HPP
#define VPYTHON_MAC_DISPLAY_HPP

#include <map>
#include <string>

#include "display_kernel.hpp"
#include "util/atomic_queue.hpp"
#include "mouseobject.hpp"

#include <boost/scoped_ptr.hpp>

#include <Carbon/Carbon.h>
#include <AGL/agl.h>

namespace cvisual {

class display : public display_kernel
{
 public:
	display();
	virtual ~display();

	// Called by the gui_main class below (or render_manager as its agent)
	void create();
	void destroy();
	void paint();
	void swap() { gl_swap_buffers(); }
	
	void lockMouse();
	void unlockMouse();
	void showMouse();
	void hideMouse();
	int  getMouseButtons();
	int  getMouseButtonsChanged();
	vector  getMouseDelta();
	vector  getMousePos();
	std::string getKeys();
	int getShiftKey();
	int getAltKey();
	int getCtrlKey();
	
	// Functions to manipulate the OpenGL context
	void gl_begin();
	void gl_end();
	void gl_swap_buffers();

	// Tells the application where it can find its data.
	// Win32 doesn't use this information.
	static void set_dataroot( const std::wstring& ) {};

	// Implements key display_kernel virtual methods
	virtual void activate( bool active );
	virtual EXTENSION_FUNCTION getProcAddress( const char* name );
	
 private:
	friend class font;
	static display* current;
	
	bool initWindow( std::string title, int x, int y, int width, int height, int flags );
	bool changeWindow( std::string title, int x, int y, int width, int height, int flags );
	bool isOpen();	
	
	enum {
		DEFAULT    = 0,
		FULLSCREEN = 0x1,
		QB_STEREO  = 0x2
	} WindowFlags;
  
	void makeCurrent();
	void makeNotCurrent();
	void swapBuffers();

	OSStatus vpWindowHandler (EventRef event);
	OSStatus vpMouseHandler (EventRef event);
	OSStatus vpKeyboardHandler (EventRef event);

 	bool window_visible;
	
	void update_size();

    // This is a list of glDeleteLists calls that should be made the next time
    // the context is made active.
    mutex list_lock;
	std::vector<std::pair<int, int> > pending_glDeleteLists;

	inline std::string lastError() { return error_message; }
    
	AGLContext getContext () { return gl_context; }
	
	void destroy_context (aglContext * cx);
	
	void add_pending_glDeleteList(int base, int howmany);
	
	WindowRef	window;
	AGLContext	gl_context;
	std::string error_message;

	int buttonState, buttonsChanged;
	UInt32 keyModState;
	vector mousePos, oldMousePos;
	bool mouseLocked;
	std::queue<std::string> keys;

 protected:
	// Implementors of this class should call this function in their implementation
	// of makeCurrent();
 	void delete_pending_lists();

};

/***************** gui_main implementation ********************/

class gui_main
{
 private:	
	// Components of the startup sequence.
	static void init_thread(void);

	gui_main();	//< This is the only nonstatic member function that doesn't run in the gui thread!
	void run();
	void poll();

	static gui_main* self;
	
	void _event_callback( double seconds, bool (*callback)(void*), void *data);

	template <class T>
	inline void event_callback( double seconds, bool (*callback)(T*), T* data) 
	{
		_event_callback( seconds, (bool(*)(void*))callback, (void*)data);
	}
	
	typedef struct {
		bool (*func)(void * data);
		void * funcData;
		double delay;
		EventLoopTimerUPP upp;
	} VPCallback;

 public:
	 static void* event_loop (void * arg);
	 static void doCallback (EventLoopTimerRef timer, void * data);
	 void _event_callback( double seconds, bool (*callback)(void*), void *data)
	 void start_event_loop();
	 static bool doQuit(void * arg);
	 void stop_event_loop();
	 void init_thread(void);
	 
	 // Calls the given function in the GUI thread.
	 static void call_in_gui_thread( const boost::function< void() >& f );
	 void timer_callback(void, bool);
	 void poll();
	
	 void init_platform(); // make app into a GUI app if necessary
	
	 // This signal is invoked when the user closes the program (closes a display
	 // with display.exit = True).
	 // wrap_display_kernel() connects a signal handler that forces Python to
	 // exit.
	 static boost::signal<void()> on_shutdown;
};

} // !namespace cvisual

#endif /*VPYTHON_MAC_DISPLAY_HPP*/
