#ifndef VPYTHON_WIN32_RENDER_SURFACE_HPP
#define VPYTHON_WIN32_RENDER_SURFACE_HPP

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#define _WIN32_IE 0x400
#include <commctrl.h>

#include "render_core.hpp"
#include <map>
#include <string>
#include <sigc++/object.h>

// NOTE: This implementation might require Windows 98 (For the timer callback).

class basic_app;
class bitmap_font;

class render_surface : public SigC::Object
{
 private:
	friend class basic_app;
	friend class bitmap_font;

 	float last_mousepos_x;
 	float last_mousepos_y;
	
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
	LRESULT on_create( WPARAM, LPARAM);
	LRESULT on_showwindow( WPARAM, LPARAM);
	LRESULT on_mousemove( WPARAM, LPARAM);
	LRESULT on_size( WPARAM, LPARAM);
	LRESULT on_paint( WPARAM, LPARAM);
	LRESULT on_destroy( WPARAM, LPARAM);
	
	// Callbacks provided to the render_core object.
	void gl_begin();
	void gl_end();
	void gl_swap_buffers();
	
	void create_window( HWND);
	
 public:
	render_surface();
	~render_surface();
	render_core core;
	
	void add_renderable( shared_ptr<renderable>);
	void add_renderable_screen( shared_ptr<renderable>);
	void remove_renderable( shared_ptr<renderable>);
	void remove_renderable_screen( shared_ptr<renderable>);

	// Signal fired by button down + button up
	SigC::Signal1<void, shared_ptr<renderable> > object_clicked;
	static render_surface* current;
};

class basic_app
{
 private:
	// Handles to various Windows resources
 	HWND window_handle;
 	HWND toolbar;
 	HANDLE tb_image[4];
 	HIMAGELIST tb_imlist;
 	int tb_imlist_idx[4];
 	
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
	
	std::string title;
	
 public:
	basic_app( std::string title = std::string("VPython rendering core 2"));
	~basic_app();
	render_surface scene;
 
	// Shows the main window and enters the message loop, returning when done.
	void run();
};



#endif // !defined VPYTHON_WIN32_RENDER_SURFACE_HPP
