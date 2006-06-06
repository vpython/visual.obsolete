// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "win32/render_surface.hpp"
#include "util/errors.hpp"
#include "vpython-config.h"

#include <sigc++/sigc++.h>

#include <cstdlib>
#include <cstring>
#include <cassert>
#include <iostream>

namespace cvisual {

/**************************** Utilities ************************************/
// Extracts and decodes a Win32 error message.
void
win32_write_critical( 
	std::string file, int line, std::string func, std::string msg)
{
	DWORD code = GetLastError();
	char* message = 0;
	FormatMessage( 
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		0, // Ignored parameter
		code, // Error code to be decoded.
		0, // Use the default language
		(char*)&message, // The output buffer to fill the message
		512, // Allocate at least one byte in order to free something below.
		0); // No additional arguments.
	
	std::cerr << "VPython ***Critical Error*** " << file << ":" << line << ": "
		<< func << ": " << msg << ": " << message;
	LocalFree(message);
	std::exit(1);
}

/**************** render_surface implementation  *****************/

// A lookup-table for the default widget procedure to use to find the actual
// widget that should handle a particular callback message.
std::map<HWND, render_surface*> render_surface::widgets;
render_surface* render_surface::current = 0;

void
render_surface::register_win32_class()
{
	static bool done = false;
	if (done)
		return;
	else {
		std::memset( &win32_class, 0, sizeof(win32_class));
		
		win32_class.lpszClassName = "vpython_win32_render_surface";
		win32_class.lpfnWndProc = &render_surface::dispatch_messages;
		win32_class.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
		win32_class.hInstance = GetModuleHandle(0);
		win32_class.hIcon = LoadIcon( NULL, IDI_APPLICATION );
		win32_class.hCursor = LoadCursor( NULL, IDC_ARROW );
		if (!RegisterClass( &win32_class))
			WIN32_CRITICAL_ERROR("RegisterClass()");
		done = true;
	}
}

// TODO: Change mouse movement handling to lock the mouse in place and continue
// to process events.
// This function dispatches incoming messages to the particular message-handler.
LRESULT CALLBACK 
render_surface::dispatch_messages( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	render_surface* This = widgets[hwnd];
	if (This == 0)
		return DefWindowProc( hwnd, uMsg, wParam, lParam);

	switch (uMsg) {
		// Handle the cases for the kinds of messages that we are listening for.
		case WM_DESTROY:
			return This->on_destroy( wParam, lParam);
		case WM_SIZE:
			return This->on_size( wParam, lParam);
		case WM_PAINT:
			return This->on_paint( wParam, lParam);
		case WM_MOUSEMOVE:
			// Handle mouse cursor movement.
			return This->on_mousemove( wParam, lParam);
		case WM_SHOWWINDOW:
			return This->on_showwindow( wParam, lParam);
		case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN:
			return This->on_buttondown( wParam, lParam);
		case WM_LBUTTONUP: case WM_RBUTTONUP: case WM_MBUTTONUP:
			return This->onbuttonup( wParam, lParam);
		case WM_WINDOWPOSCHANGED:
			return This->on_windowmove( wParam, lParam);
		default:
			return DefWindowProc( hwnd, uMsg, wParam, lParam);
	}
}

VOID CALLBACK
render_surface::timer_callback( HWND hwnd, UINT, UINT_PTR, DWORD)
{
	render_surface* This = widgets[hwnd];
	if (0 == This)
		return;
	This->on_paint(0, 0);
}

LRESULT 
render_surface::on_showwindow( WPARAM wParam, LPARAM lParam)
{
	UINT id = 1;
	switch (lParam) {
		case 0:
			// Opening for the first time.
			core.report_realize();
			SetTimer( widget_handle, id, 30, &render_surface::timer_callback);
			break;
		case SW_PARENTCLOSING:
			// Stop rendering when the window is minimized.
			KillTimer( widget_handle, id);
			break;
		case SW_PARENTOPENING:
			// restart rendering when the window is restored.
			SetTimer( widget_handle, id, 30, &render_surface::timer_callback);
			break;
		default:
			return DefWindowProc( widget_handle, WM_SHOWWINDOW, wParam, lParam);
	}
	return 0;
}

LRESULT  
render_surface::on_mousemove( WPARAM wParam, LPARAM lParam)
{
	// TODO: Modify this implementation to make the mouse dissappear and lock
	// it to a particular position during mouse right-clicks.
	bool left_down = wParam & MK_LBUTTON;
	bool middle_down = wParam & MK_MBUTTON;
	bool right_down = wParam & MK_RBUTTON;
	if (left_down && right_down)
		middle_down = true;
	bool buttondown = left_down || middle_down || right_down;
	float mouse_x = LOWORD(lParam);
	float mouse_y = HIWORD(lParam);
	
	float dx = mouse_x - last_mousepos_x;
	float dy = mouse_y - last_mousepos_y;
	bool mouselocked = false;
	if (middle_down) {
		core.report_mouse_motion( dx, dy, display_kernel::MIDDLE);
		mouselocked = true;
	}
	if (right_down) {
		core.report_mouse_motion( dx, dy, display_kernel::RIGHT);
		mouselocked = true;
	}
	if (!buttondown)
		core.report_mouse_motion( dx, dy, display_kernel::NONE);
	
	mouse.set_shift( wParam & MK_SHIFT);
	mouse.set_ctrl( wParam & MK_CONTROL);
	mouse.set_alt( GetKeyState( VK_MENU) < 0);
	if (mouselocked) {
		SetCursorPos( last_mousepos_x, last_mousepos_y);
	}
	else {
		last_mousepos_x = mouse_x;
		last_mousepos_y = mouse_y;
	}
	mouse.cam = core.calc_camera();
	
	if (left_button.is_dragging())
		mouse.push_event( drag_event( 1, mouse));
	if (middle_button.is_dragging())
		mouse.push_event( drag_event( 2, mouse));
	if (right_button.is_dragging())
		mouse.push_event( drag_event( 3, mouse));
	
	return 0;
}

LRESULT  
render_surface::on_size( WPARAM, LPARAM lParam)
{
	window_width = LOWORD(lParam);
	window_height = HIWORD(lParam);
	core.report_resize( window_width, window_height);
	return 0;
}

LRESULT  
render_surface::on_paint( WPARAM, LPARAM)
{
	bool sat = core.render_scene();
	if (!sat)
		return 1;
	
	boost::tie( mouse.pick, mouse.pickpos, mouse.position) = 
		core.pick( last_mousepos_x, last_mousepos_y);
	// TODO: Add timer cycle time munging (or at least consider it)
	RECT dims;
	// The following calls report the fact that the widget area has been 
	// repainted to the windowing system.
	GetClientRect( widget_handle, &dims);
	ValidateRect( widget_handle, &dims);
	return 0;
}

LRESULT  
render_surface::on_destroy( WPARAM, LPARAM)
{
	// Perform cleanup actions.
	UINT id = 1;
	KillTimer( widget_handle, id);
	wglDeleteContext( gl_context);
	ReleaseDC( widget_handle, dev_context);
	widgets.erase( widget_handle);
	if (exit) {
		PostQuitMessage( 0);
	}
	return 0;
}

LRESULT
render_surface::on_buttondown( WPARAM wParam, LPARAM lParam)
{
	mouse.set_shift( wParam & MK_SHIFT);
	mouse.set_ctrl( wParam & MK_CONTROL);
	mouse.set_alt( GetKeyState( VK_MENU) < 0);
	
	float x = (float)GET_X_LPARAM(lParam);
	float y = (float)GET_Y_LPARAM(lParam);
	
	int button_id = 0;
	if (wParam & MK_LBUTTON && left_button.press(x, y)) {
		button_id = 1;
	}
	if (wParam & MK_RBUTTON && right_button.press( x, y)) {
		button_id = 3;
	}
	if (wParam & MK_MBUTTON && middle_button.press( x, y)) {
		button_id = 2;
	}
	
	if (button_id)
		mouse.push_event( press_event( button_id, mouse));
	return 0;
}

LRESULT
render_surface::on_buttonup( WPARAM wParam, LPARAM lParam)
{
	mouse.set_shift( wParam & MK_SHIFT);
	mouse.set_ctrl( wParam & MK_CONTROL);
	mouse.set_alt( GetKeyState( VK_MENU) < 0);
	
	float x = (float)GET_X_LPARAM(lParam);
	float y = (float)GET_Y_LPARAM(lParam);
	
	int button_id = 0;
	std::pair<bool, bool> unique_drop( false, false);
	#define unique unique_drop.first
	#define drop unique_drop.second
	
	if (wParam & MK_LBUTTON) {
		unique_drop = left_button.release();
		if (unique) {
			button_id = 1;
			goto found;
		}
	}
	if (wParam & MK_RBUTTON) {
		unique_drop = right_button.release();
		if (unique) {
			button_id = 3;
			goto found;
		}
	}
	if (wParam & MK_MBUTTON) {
		unique_drop = middle_button.release();
		if (unique) {
			button_id = 2;
			goto found;
		}
	}
found:
	if (button_id)
		if (drop)
			mouse.push_event( drop_event( button_id, mouse));
		else
			mouse.push_event( click_event( event->button, mouse));
	return 0;
	#undef unique
	#undef drop
}

LRESULT
render_surface::on_windowmove( WPARAM wParam, LPARAM lParam)
{
	WINDOWPOS* pos = (WINDOWPOS*)lParam;
	x = pos->x;
	y = pos->y;
	window_width = pos->cx;
	window_height = pos->cy;
	return 0;
}

WNDCLASS render_surface::win32_class;

// Callbacks provided to the display_kernel object.
void 
render_surface::gl_begin()
{
	wglMakeCurrent( dev_context, gl_context);
	current = this;
}

void 
render_surface::gl_end()
{
	wglMakeCurrent( 0, 0);
	current = 0;
}

void 
render_surface::gl_swap_buffers()
{
	SwapBuffers( dev_context);
}

void
render_surface::gl_free()
{
	VPYTHON_NOTE( "Releasing GL resources");
	gl_begin();
	try {
		clear_gl_error();
		on_gl_free();
		check_gl_error();
	}
	catch (gl_error& error) {
		VPYTHON_CRITICAL_ERROR( "Caught OpenGL error during shutdown: " + std::string(error.what()));
		std::cerr << "Continuing with the shutdown." << std::endl;
	}
	gl_end();
}

render_surface::render_surface()
	: x(-1), y(-1),
	exit(true), visible(true), fullscreen(false), title( "VPython"),
	last_mousepos_x(0), last_mousepos_y(0), mouselocked( false), active(false),
	window_width(-1), window_height(-1),
	widget_handle(0), timer_handle(0), dev_context(0), gl_context(0)
{
	// Initialize data
	// Initialize the win32_class
	render_surface::register_win32_class();
	
	// Connect callbacks from the display_kernel to this object.  These will not
	// be called back from the core until report_realize is called.
	core.gl_begin.connect( sigc::mem_fun( *this, &render_surface::gl_begin));
	core.gl_end.connect( sigc::mem_fun( *this, &render_surface::gl_end));
	core.gl_swap_buffers.connect( 
		sigc::mem_fun( *this, &render_surface::gl_swap_buffers));
}

void
render_surface::create()
{
	register_win32_class();
	
	RECT screen;
	SystemParametersInfo( SPI_GETWORKAREA, 0, &screen, 0);
	int style = -1;
	if (fullscreen) {
		x = screen.left;
		y = screen.top;
		window_width = screen.right - x;
		window_height = screen.bottom - y;
		style = WS_OVERLAPPED | WS_POPUP | WS_MAXIMIZE | WS_VISIBLE;
	}
	else
		style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	
	widget_handle = CreateWindow(
		win32_class.lpszClassName,
		title.c_str(),
		style,
		x > 0 ? x : CW_USEDEFAULT,
		y > 0 ? y : CW_USEDEFAULT,
		window_width > 0 ? window_width : CW_USEDEFAULT,
		window_height > 0 ? window_height : CW_USEDEFAULT,
		0,
		0, // A unique index to identify this widget by the parent
		GetModuleHandle(0),
		0 // No data passed to the WM_CREATE function.
	);
	widgets[widget_handle] = this;

	// The first OpenGL Rendering Context, used to share displaylists.
	static HGLRC root_glrc = 0;
	
	dev_context = GetDC(widget_handle);
	if (!dev_context)
		WIN32_CRITICAL_ERROR( "GetDC()");
	
	PIXELFORMATDESCRIPTOR pfd = { 
		sizeof(PIXELFORMATDESCRIPTOR),   // size of this pfd 
		1,                               // version number 
		PFD_DRAW_TO_WINDOW |             // output to screen (not an image) 
		PFD_SUPPORT_OPENGL |             // support OpenGL 
		PFD_DOUBLEBUFFER,                // double buffered
		PFD_TYPE_RGBA,                   // RGBA type 
		24,                              // 24-bit color depth 
		0, 0, 0, 0, 0, 0,                // color bits ignored 
		0,                               // no alpha buffer 
		0,                               // shift bit ignored 
		0,                               // no accumulation buffer 
		0, 0, 0, 0,                      // accum bits ignored 
		32,                              // 32-bit z-buffer 
		0,                               // no stencil buffer 
		0,                               // no auxiliary buffer 
		PFD_MAIN_PLANE,                  // main layer 
		0,                               // reserved 
		0, 0, 0                          // layer masks ignored 
	};
	
	int pixelformat = ChoosePixelFormat( dev_context, &pfd);
	
	DescribePixelFormat( dev_context, pixelformat, sizeof(pfd), &pfd);
	SetPixelFormat( dev_context, pixelformat, &pfd);
	
	gl_context = wglCreateContext( dev_context);
	if (!gl_context)
		WIN32_CRITICAL_ERROR("wglCreateContext()");
	
	if (!root_glrc) 
		root_glrc = gl_context;
	else
		wglShareLists( root_glrc, gl_context);
	
	ShowWindow( widget_handle, SW_SHOW);
}

void
render_surface::destroy()
{
	DestroyWindow( widget_handle);
	hwnd = 0;
	active = false;
}

render_surface::~render_surface()
{
}

void 
render_surface::set_x( float n_x)
{
	lock L(mtx);
	if (active) {
		throw std::runtime_error( "Cannot change parameters of an active window");
	}
	else
		x = n_x;
}
float 
render_surface::get_x()
{
	return x;
}
 
void 
render_surface::set_y( float n_y)
{
	lock L(mtx);
	if (active) {
		throw std::runtime_error( "Cannot change parameters of an active window");
	}
	else
		y = n_y;
}
float 
render_surface::get_y()
{
	return y;
}
 
void 
render_surface::set_width( float w)
{
	lock L(mtx);
	if (active) {
		throw std::runtime_error( "Cannot change parameters of an active window");
	}
	else
		window_width = w;
}
float 
render_surface::get_width()
{
	return window_width;
}
 
void 
render_surface::set_height( float h)
{
	lock L(mtx);
	if (active) {
		throw std::runtime_error( "Cannot change parameters of an active window");
	}
	else
		window_height = h;;
}
float 
render_surface::get_height()
{
	return window_height;
}
	
void 
render_surface::set_visible( bool v)
{
	if (vis && !active) {
		VPYTHON_NOTE( "Opening a window from Python.");
		gui_main::add_display( this);
	}
	else if (!vis && active) {
		VPYTHON_NOTE( "Closing a window from Python.");
		gui_main::remove_display(this);
	}
	visible = vis;
}
bool 
render_surface::get_visible()
{
	if (!active)
		return false;
	return visible;
}

void 
render_surface::set_title( std::string n_title)
{
	lock L(mtx);
	if (active) {
		throw std::runtime_error( "Cannot change parameters of an active window");
	}
	else
		title = n_title;
}
std::string 
render_surface::get_title()
{
	return title;
}
 
bool 
render_surface::is_fullscreen()
{
	return fullscreen;
}
void 
render_surface::set_fullscreen( bool fs)
{
	lock L(mtx);
	if (active) {
		throw std::runtime_error( "Cannot change parameters of an active window");
	}
	else
		fullscreen = fs;
}

void
render_surface::add_renderable( shared_ptr<renderable> obj)
{
	display_kernel::add_renderable( obj);
	if (!active && visible) {
		gui_main::add_display(this);
	}
}

void
render_surface::add_renderable_screen( shared_ptr<renderable> obj)
{
	display_kernel::add_renderable( obj);
	if (!active && visible)
		gui_main::add_display(this);
}

mouse_t*
render_surface::get_mouse()
{
	if (!visible)
		visible = true;
	if (!active)
		gui_main::add_display( this);
	
	return &mouse;
}

atomic_queue<std::string>*
render_surface::get_kb()
{
	if (!visible)
		visible = true;
	if (!active)
		gui_main::add_display( this);
	
	return &keys;
}

void 
render_surface::set_selected( shared_ptr<display> d)
{
	selected = d;
}

shared_ptr<display>
render_surface::get_selected()
{
	return selected;
}

/******************************* gui_main implementatin **********************/

gui_main* gui_main::self = 0;
mutex* gui_main::init_lock = 0;
condition* gui_main::init_signal = 0;
sigc::signal0<void> gui_main::on_shutdown;


gui_main::gui_main()
	: idThread(GetCurrentThreadId()), 
	caller( 0), returned( false), waiting_allclosed(false), 
	thread_exited(false), shutting_down( false)
{
	// Force the create of a message queue
	MSG message;
	PeekMessage( &message, NULL, WM_USER, WM_USER, PM_NOREMOVE)
}

void 
gui_main::signal_add_display()
{
	PostThreadMessage( idThread, VPYTHON_ADD_DISPLAY, 0, 0);
}

void 
gui_main::signal_remove_display()
{
	PostThreadMessage( idThread, VPYTHON_REMOVE_DISPLAY, 0, 0);
}

void 
gui_main::signal_shutdown()
{
	PostThreadMessage( idThread, VPYTHON_SHUTDOWN, 0, 0);
}

void
gui_main::run()
{
	// Enter the message loop
	MSG message;
	while (true) {
		while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
			if (message.message == WM_QUIT)
				goto quit;
			
			if (message.hwnd ==0) { // One of the threaded callbacks
				switch (message.message) {
					case VPYTHON_ADD_DISPLAY:
						add_display_impl();
						break;
					case VPYTHON_REMOVE_DISPLAY:
						remove_display_impl();
						break;
					case VPYTHON_SHUTDOWN:
						shutdown_impl();
						break;
					default:
						break; // nothing...
				}
				continue;
			}
			// Destined for the primary window procedure above
			TranslateMessage( &message);
			DispatchMessage( &message);
		}
		if (!WaitMessage())
			WIN32_CRITICAL_ERROR( "WaitMessage()");
	}
	
quit:
	lock L(call_lock);
	if (waiting_allclosed) {
		returned = true;
		call_complete.notify_all();
	}
	thread_exited = true;
}

void 
gui_main::thread_proc(void)
{
	assert( init_lock);
	assert( init_signal);
	assert( !self);
	{
		lock L(*init_lock);
		self = new gui_main();
		init_signal->notify_all();
	}
	self->run();
	VPYTHON_NOTE( "Terminating GUI thread.");
	gui_main::on_shutdown();
}

void
gui_main::init_thread(void)
{
	if (!init_lock) {
		init_lock = new mutex;
		init_signal = new condition;
		VPYTHON_NOTE( "Starting GUI thread.");
		lock L(*init_lock);
		thread gtk( &gui_main::thread_proc);
		while (!self)
			init_signal->wait(L);
	}
}

// TODO: This may not be safe.  In fact, it might be easier to get notification
// of shutdown via a truly joinable GUI thread.
bool
gui_main::allclosed()
{
	if (!self)
		return true;
	lock L(self->call_lock);
	return self->displays.empty();
}

void
gui_main::waitclosed()
{
	if (!self)
		return;
	lock L(self->call_lock);
	if (self->thread_exited)
		return;
	self->waiting_allclosed = true;
	self->returned = false;
	while (!self->returned) {
		self->call_complete.wait(L);
	}
}

void
gui_main::add_display( display* d)
{
	init_thread();
	lock L(self->call_lock);
	if (self->shutting_down) {
		return;
	}
	VPYTHON_NOTE( std::string("Adding new display object at address ") 
		+ lexical_cast<std::string>(d));
	self->caller = d;
	self->returned = false;
	self->signal_add_display();
	while (!self->returned)
		self->call_complete.py_wait(L);
	self->caller = 0;
}

void
gui_main::add_display_impl()
{
	lock L(call_lock);
	caller->create();
	displays.push_back(caller);
	returned = true;
	call_complete.notify_all();
}

void
gui_main::remove_display( display* d)
{
	assert( self);
	VPYTHON_NOTE( std::string("Removing existing display object at address ") 
		+ lexical_cast<std::string>(d));
	
	lock L(self->call_lock);
	self->caller = d;
	self->returned = false;
	self->signal_remove_display();
	while (!self->returned)
		self->call_complete.py_wait(L);
	self->caller = 0;
}

void
gui_main::remove_display_impl()
{
	lock L(call_lock);
	caller->destroy();
	displays.remove( caller);
	returned = true;
	call_complete.notify_all();
}

void
gui_main::shutdown()
{
	if (!self)
		return;
	lock L(self->call_lock);
	VPYTHON_NOTE( "Initiating shutdown from Python.");
	if (self->thread_exited)
		return;
	self->returned = false;
	self->signal_shutdown();
	while (!self->returned)
		self->call_complete.py_wait(L);
}

void
gui_main::shutdown_impl()
{
	lock L(call_lock);
	shutting_down = true;
	for (std::list<display*>::iterator i = displays.begin(); i != displays.end(); ++i) {
		(*i)->destroy();
	}
	self->returned = true;
	call_complete.notify_all();
	PostQuitMessage( 0);
}

void
gui_main::report_window_delete( display* window)
{
	assert( self != 0);
	bool display_empty = false;
	{
		lock L(self->call_lock);
		self->displays.remove( window);
		display_empty = self->displays.empty();
	}
	if (display_empty && self->waiting_allclosed)
		gui_main::quit();
}

void
gui_main::quit()
{
	assert( self != 0);
	lock L(self->call_lock);
	self->shutting_down = true;
	for (std::list<display*>::iterator i = self->displays.begin(); 
			i != self->displays.end(); ++i) {
		(*i)->destroy();
	}
	self->displays.clear();
	PostQuitMessage( 0);
}

} // !namespace cvisual;
