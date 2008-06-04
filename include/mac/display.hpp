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
using boost::scoped_ptr;

class aglContext 
{
 private:
    // This is a list of glDeleteLists calls that should be made the next time
    // the context is made active.
    mutex list_lock;
	std::vector<std::pair<int, int> > pending_glDeleteLists;

 protected:
	// Implementors of this class should call this function in their implementation
	// of makeCurrent();
 	void delete_pending_lists();

 public:
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
	
	// Do not delete contexts directly, as there can be
	// nasty interactions with the event loop thread.
	// Use the destroy_context function instead.
	aglContext();
	~aglContext();
	
	void cleanup();

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

	OSStatus aglContext::vpWindowHandler (EventRef event);
	OSStatus aglContext::vpMouseHandler (EventRef event);
	OSStatus aglContext::vpKeyboardHandler (EventRef event);
	
	int winX();
	int winY();
	int width();
	int height();

	inline std::string lastError() { return error_message; }
    
	//glFont* getFont(const char* description, double size);

	AGLContext getContext () { return ctx; }
	
	void destroy_context (aglContext * cx);
	
	void add_pending_glDeleteList(int base, int howmany);
	
 private:
	WindowRef	window;
	AGLContext	ctx;
	int wx, wy, wwidth, wheight;
	std::string error_message;

	int buttonState, buttonsChanged;
	UInt32 keyModState;
	vector mousePos, oldMousePos;
	bool mouseLocked;
	std::queue<std::string> keys;
	
};

// A singleton.  This class provides all of the abstraction from the Gtk::Main
// object, in addition to providing asynchronous communication channels between
// threads.
class gui_main
{
 private:	
	// Components of the startup sequence.
	static void init_thread(void);

	gui_main();	//< This is the only nonstatic member function that doesn't run in the gui thread!
	void run();
	void poll();

	static gui_main* self;
	
	//DWORD gui_thread;
	mutex init_lock;
	condition initialized;

 	//HANDLE timer_handle;
	
	//static LRESULT CALLBACK threadMessage( int, WPARAM, LPARAM );

	//static VOID CALLBACK timer_callback( PVOID, BOOLEAN );
	
 public:
	// Calls the given function in the GUI thread.
	static void call_in_gui_thread( const boost::function< void() >& f );
	
	// This signal is invoked when the user closes the program (closes a display
	// with display.exit = True).
	// wrap_display_kernel() connects a signal handler that forces Python to
	// exit.
	static boost::signal<void()> on_shutdown;
};

class display : public display_kernel
{	
public:
	display();
	virtual ~display();
	
	// Called by the gui_main class below (or render_manager as its agent)
	void create();
	void destroy();
	void paint(); // { area->paint(); }
	void swap(); // { area->swap(); } or win: { gl_swap_buffers(); }

	// Tells the application where it can find its data.
	static void set_dataroot( const std::wstring& dataroot);
	
	// Implements key display_kernel virtual methods
	virtual void activate(bool);
	EXTENSION_FUNCTION getProcAddress( const char* name );
	
 private:
	//static int get_titlebar_height();
	//static int get_toolbar_height();

/*
	// Signal handlers for the various widgets.
	void on_fullscreen_clicked();
	void on_pan_clicked();
	void on_rotate_clicked();
	bool on_window_delete(      );
	void on_quit_clicked();
	void on_zoom_clicked();
	bool on_key_pressed(        );
*/
	
	// Functions to manipulate the OpenGL context
	void gl_begin();
	void gl_end();
	void gl_swap_buffers();

 private:
	//scoped_ptr<render_surface> area;
	//Gtk::Window* window;
	 bool window_visible;
	 aglContext agl_context;
	 static display* current;
};

} // !namespace cvisual

#endif /*VPYTHON_MAC_DISPLAY_HPP*/
