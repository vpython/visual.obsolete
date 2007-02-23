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
#include "mouseobject.hpp"
#include "util/thread.hpp"
#include <map>
#include <string>
//#include <sigc++/object.h>

extern "C" {
VOID CALLBACK
render_surface_timer_callback( HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

// A callback function to dispatch normal messages from the system.
LRESULT CALLBACK
render_surface_dispatch_messages( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
} // !extern "C"


namespace cvisual {

// NOTE: This implementation might require Windows 98 (For the timer callback).
class font;

class render_surface : public display_kernel
{
 private:
	friend class font;
	friend VOID CALLBACK
		::render_surface_timer_callback( HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
	friend LRESULT CALLBACK
		::render_surface_dispatch_messages( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
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

 	HDC saved_dc;
 	HGLRC saved_glrc;

	// A callback function to handle the render pulse.

	static void register_win32_class();
	static WNDCLASS win32_class;
	static std::map<HWND, render_surface*> widgets;

	// Procedures used to process messages.
	LRESULT on_showwindow( WPARAM, LPARAM);
	LRESULT on_mousemove( WPARAM, LPARAM);
	LRESULT on_paint( WPARAM, LPARAM);
	LRESULT on_close( WPARAM, LPARAM);
	LRESULT on_buttondown( WPARAM, LPARAM);
	LRESULT on_buttonup( WPARAM, LPARAM);
	LRESULT on_getminmaxinfo( WPARAM, LPARAM);
	LRESULT on_keypress( UINT, WPARAM, LPARAM);
	LRESULT on_size( WPARAM, LPARAM);
	LRESULT on_move( WPARAM, LPARAM);

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
	render_surface();
	virtual ~render_surface();

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
	static void set_dataroot( std::string) {};

	atomic_queue<std::string>* get_kb();
	mouse_t* get_mouse();

	// The 'selected' display.
 private:
	static shared_ptr<render_surface> selected;
 public:
	static void set_selected( shared_ptr<render_surface> d);
	static shared_ptr<render_surface> get_selected();
};
} // !namespace cvisual

#endif // !defined VPYTHON_WIN32_RENDER_SURFACE_HPP
