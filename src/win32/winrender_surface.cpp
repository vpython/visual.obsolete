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
		// case WM_CREATE:
			// Handle creation of the window.
		//	return This->on_create( wParam, lParam);
		case WM_SHOWWINDOW:
			return This->on_showwindow( wParam, lParam);
		default:
			return DefWindowProc( hwnd, uMsg, wParam, lParam);
	}
}

VOID CALLBACK
render_surface::timer_callback( HWND hwnd, UINT, UINT_PTR, DWORD)
{
	render_surface* This = widgets[hwnd];
	This->on_paint(0, 0);
}

// TODO: Portions of the routine need to wait until the window is made visible.
LRESULT 
render_surface::on_create( WPARAM, LPARAM)
{
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
	
	// ShowWindow( widget_handle, SW_SHOW);
	// core.report_realize();

	// Initialize the timer routine
	// UINT id = 0;
	// SetTimer( widget_handle, &id, 28, &render_surface::dispatch_messages);
	
	return 0;
}

LRESULT 
render_surface::on_showwindow( WPARAM wParam, LPARAM lParam)
{
	UINT id = 1;
	switch (lParam) {
		case 0:
			// Opening for the first time.
			core.report_realize();
			SetTimer( widget_handle, id, 20, &render_surface::timer_callback);
			break;
		case SW_PARENTCLOSING:
			// Stop rendering when the window is minimized.
			KillTimer( widget_handle, id);
			break;
		case SW_PARENTOPENING:
			// restart rendering when the window is restored.
			SetTimer( widget_handle, id, 20, &render_surface::timer_callback);
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
	// bool left_down = wParam & MK_LBUTTON;
	bool middle_down = wParam & MK_MBUTTON;
	bool right_down = wParam & MK_RBUTTON;
	float mouse_x = LOWORD(lParam);
	float mouse_y = HIWORD(lParam);
	
	float dx = mouse_x - last_mousepos_x;
	float dy = mouse_y - last_mousepos_y;
	if (middle_down) {
		core.report_mouse_motion( dx, dy, display_kernel::MIDDLE);
	}
	if (right_down) {
		core.report_mouse_motion( dx, dy, display_kernel::RIGHT);
	}
	last_mousepos_x = mouse_x;
	last_mousepos_y = mouse_y;
	return 0;
}

LRESULT  
render_surface::on_size( WPARAM, LPARAM)
{
	RECT dims;
	GetClientRect( widget_handle, &dims);
	window_width = dims.right - dims.left;
	window_height = dims.bottom - dims.top;
	core.report_resize( dims.right - dims.left, dims.bottom - dims.top);
	return 0;
}

LRESULT  
render_surface::on_paint( WPARAM, LPARAM)
{
	core.render_scene();
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
	// DestroyWindow( widget_handle);
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

render_surface::render_surface()
	: last_mousepos_x(0), last_mousepos_y(0), window_width(0), window_height(0),
	widget_handle(0), timer_handle(0), dev_context(0), gl_context(0)
{
	// Initialize data
	// Initialize the win32_class
	render_surface::register_win32_class();
	
	// Connect callbacks from the display_kernel to this object.  These will not
	// be called back from the core until report_realize is called.
	core.gl_begin.connect( SigC::slot( *this, &render_surface::gl_begin));
	core.gl_end.connect( SigC::slot( *this, &render_surface::gl_end));
	core.gl_swap_buffers.connect( 
		SigC::slot( *this, &render_surface::gl_swap_buffers));
}

void
render_surface::create_window( HWND parent)
{
	widget_handle = CreateWindow(
		win32_class.lpszClassName,
		0, // No window title for this surface.
		WS_CHILD,
		CW_USEDEFAULT, // x
		CW_USEDEFAULT, // y
		CW_USEDEFAULT, // width
		CW_USEDEFAULT, // height
		parent,
		0, // A unique index to identify this widget by the parent
		GetModuleHandle(0),
		0 // No data passed to the WM_CREATE function.
	);
	widgets[widget_handle] = this;
	on_create(0,0);
}

render_surface::~render_surface()
{
}

/******************************* basic_app implementation ****************/

// The WindProc function for this class mostly just dispatches the message to
// a particular message handler, rather than performing all of that logic here.
LRESULT CALLBACK 
basic_app::dispatch_messages( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	basic_app* This = windows[hwnd];
	if (This == 0)
		return DefWindowProc( hwnd, uMsg, wParam, lParam);

	switch (uMsg) {
		case WM_COMMAND:
			// A Command from a particular button.
			return This->on_command( wParam, lParam);
		case WM_GETMINMAXINFO:
			return This->on_getminmaxinfo( wParam, lParam);
		case WM_SIZE:
			return This->on_size( wParam, lParam);
		// case WM_CREATE:
		// 	return This->on_create( wParam, lParam);
		case WM_CLOSE:
			DestroyWindow( This->window_handle);
			return 0;
		case WM_DESTROY:
			windows.erase( hwnd);
			PostQuitMessage(0);
			return 0;
		default:
			return DefWindowProc( hwnd, uMsg, wParam, lParam);
	}	
}

// Called in response to WM_RESIZE.
LRESULT
basic_app::on_size( WPARAM, LPARAM)
{
	RECT n_dims;
	GetClientRect( window_handle, &n_dims);
	
	// Resize the child toolbar
	SendMessage( toolbar, TB_AUTOSIZE, 0, 0);
	
	// Get the new size of the child toolbar
	SIZE tb_size = {0,0};
	SendMessage( toolbar, TB_GETMAXSIZE, 0, (LPARAM)&tb_size);
	
	// Set scene to be the new size.
	MoveWindow( 
		scene.widget_handle, 
		0, tb_size.cy, 
		n_dims.right, n_dims.bottom - tb_size.cy,
		TRUE
	);
	return 0;
}

// The string table for the toolbar.
namespace {
enum str_idx {
	PAN,
	ROTATE_ZOOM,
	FULLSCREEN,
	QUIT,
	FULLSCREEN_TT,
	QUIT_TT,
	SEPARATOR
};

const char* toolbar_strs[] = {
	"Pan",
	"Rotate/Zoom",
	"Fullscreen",
	"Quit",
	"Toggle fullscreen mode on/off",
	"Exit this VPython program"
};
} // !namespace (unnamed)

// Called only once in response to WM_CREATE.  This function allocates size and
// creates its children.
LRESULT
basic_app::on_create( WPARAM, LPARAM)
{
	TBBUTTON buttons[5];
	// The Quit button.
	buttons[0].iBitmap = tb_imlist_idx[0];
	buttons[0].idCommand = QUIT;
	buttons[0].fsState = TBSTATE_ENABLED;
	buttons[0].fsStyle = TBSTYLE_AUTOSIZE | TBSTYLE_BUTTON;
	buttons[0].dwData = 0;
	buttons[0].iString = (int)toolbar_strs[QUIT];
	
	// The fullscreen toggle button
	buttons[1].iBitmap = tb_imlist_idx[1];
	buttons[1].idCommand = FULLSCREEN;
	buttons[1].fsState = TBSTATE_ENABLED;
	buttons[1].fsStyle = TBSTYLE_AUTOSIZE | TBSTYLE_CHECK;
	buttons[1].dwData = 0;
	buttons[1].iString = (int)toolbar_strs[FULLSCREEN];
	
	// A separator
	// TODO: Figure out why this looks wrong.
	buttons[2].iBitmap = 5;
	buttons[2].idCommand = SEPARATOR;
	buttons[2].fsState = TBSTATE_ENABLED;
	buttons[2].fsStyle = TBSTYLE_AUTOSIZE | TBSTYLE_SEP;
	buttons[2].dwData = 0;
	buttons[2].iString = 0;

	// Rotate/zoom button
	buttons[3].iBitmap = tb_imlist_idx[2];
	buttons[3].idCommand = ROTATE_ZOOM;
	buttons[3].fsState = TBSTATE_ENABLED | TBSTATE_PRESSED;
	buttons[3].fsStyle = TBSTYLE_AUTOSIZE | TBSTYLE_CHECKGROUP;
	buttons[3].dwData = 0;
	buttons[3].iString = (int)toolbar_strs[ROTATE_ZOOM];

	// Pan button
	buttons[4].iBitmap = tb_imlist_idx[3];
	buttons[4].idCommand = PAN;
	buttons[4].fsState = TBSTATE_ENABLED;
	buttons[4].fsStyle = TBSTYLE_AUTOSIZE | TBSTYLE_CHECKGROUP;
	buttons[4].dwData = 0;
	buttons[4].iString = (int)toolbar_strs[PAN];
	
	toolbar = CreateWindow(
		TOOLBARCLASSNAME,
		0,
		WS_CHILD | CCS_ADJUSTABLE | TBSTYLE_LIST, 0, 0, 0, 0, 
		window_handle, 
		0, 
		GetModuleHandle(0), 
        0
	);
	// Report the size of the TBBUTTON struct to the toolbar
	SendMessage( toolbar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);
	// Add the image list to the toolbar.
	SendMessage( toolbar, TB_SETIMAGELIST, 0, (LPARAM)tb_imlist);
	// Add the buttons to the toolbar.
	SendMessage( toolbar, TB_ADDBUTTONS, (WPARAM)5, (LPARAM) buttons);
	
	// Create the render_surface widget
	scene.create_window( window_handle);
	// Size the widget tree
	this->on_size( 0, 0);
	
	return 0;
}

LRESULT
basic_app::on_command( WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam)) {
		case QUIT:
			DestroyWindow(window_handle);
			break;
		case ROTATE_ZOOM:
			scene.core.mouse_mode = display_kernel::ZOOM_ROTATE;
			break;
		case FULLSCREEN:
			// Toggle the window fullscreen, somehow.
			break;
		case PAN:
			scene.core.mouse_mode = display_kernel::PAN;
		default:
			return DefWindowProc( window_handle, WM_COMMAND, wParam, lParam);
	}
	return 0;	
};

LRESULT 
basic_app::on_getminmaxinfo( WPARAM, LPARAM lParam)
{
	// TODO: Make this configurable.
	// TODO: Do not allow shrinking below the size of the toolbar.
	MINMAXINFO* info = (MINMAXINFO*)lParam;
	info->ptMinTrackSize.x = 384;
	info->ptMinTrackSize.y = 256;
	return 0;
}


void
basic_app::register_win32_class()
{
	static bool done = false;
	if (done)
		return;
	else {
		InitCommonControls();
		
		// Regsiter the top-level window class.
		std::memset( &win32_class, 0, sizeof(win32_class));
		
		win32_class.lpszClassName = "vpython_win32_basic_app";
		win32_class.lpfnWndProc = &basic_app::dispatch_messages;
		win32_class.hInstance = GetModuleHandle(0);
		win32_class.hIcon = LoadIcon( NULL, IDI_APPLICATION );
		win32_class.hCursor = LoadCursor( NULL, IDC_ARROW );
		if (!RegisterClass( &win32_class))
			WIN32_CRITICAL_ERROR("RegisterClass()");
		done = true;
	}
}

WNDCLASS basic_app::win32_class;
std::map<HWND, basic_app*> basic_app::windows;

/* Startup sequence:
 * Register the Win32 class type for render_surface in render_surface::render_surface()
 * Load images and fill an image list in basic_app::basic_app.
 * Register the Win32 class type for the main window.
 * Create the Main Window (invisible) with CreateWindow()
 * When recieving the WM_CREATE message:
 *   Create the toolbar and initialize the toolbar widgets in on_create()
 *   Create the render_surface object, and allocates its size with MoveWindow()
 * When render_surface recieves WM_CREATE:
 *   Gets the device context for the widget.
 *   Picks a pixel format that is suitable.
 *   Adds OpenGL capability to the widget.
 *   Shares the displaylist context.
 * 
 * Stage 2:  When the GL scene has been composed, the test program calls 
 * basic_app::run():
 * Shows the main window.
 * Shows the toolbar.
 * Shows the render_surface:
 *   render_surface recieves SW_SHOWWINDOW and it:
 *      Reports realization to the display_kernel.
 *      Renders the first window.
 *      Starts the timer for further rendering.
 * Enters the message pumping loop.  Program control is exclusively 
 *   message-based at this point.
 */
 
/* Shutdown sequence:
 * When initiated by user pressing the window close icon on the window bar,
 *   basic_app directly calls DestroyWindow() on the parent, which recursively
 *   sends WM_DESTROY to the toolbar and render_surface.
 * render_surface recieves WM_DESTROY and:
 *    KillTimer()'s the timer.
 *    Deletes the GL rendering context.
 *    Destroys the widget-specific device context.
 *    Remove's its handle from the lookup table.
 * basic_app recieves WM_DESTROY and:
 *    Remove's its handle from the lookup table.
 *    Posts WM_QUIT to the message loop.
 * WM_QUIT causes basic_app::run() to recieve 0 from GetMessage(), and returns
 *    program control to main(), which returns 0 to the shell.
 */
basic_app::basic_app( std::string _title)
	: title( _title)
{ 
	// Load the images for the toolbar from files.
	tb_image[0] = LoadImage( 
		0, VPYTHON_PREFIX "/data/no.bmp", IMAGE_BITMAP, 32, 32, LR_LOADFROMFILE);
	tb_image[1] = LoadImage( 
		0, VPYTHON_PREFIX "/data/galeon-fullscreen.bmp", IMAGE_BITMAP, 32, 32, 
		LR_LOADFROMFILE);
	tb_image[2] = LoadImage( 
		0, VPYTHON_PREFIX "/data/no.bmp", IMAGE_BITMAP, 32, 32, LR_LOADFROMFILE);
	tb_image[3] = LoadImage( 
		0, VPYTHON_PREFIX "/data/no.bmp", IMAGE_BITMAP, 32, 32, LR_LOADFROMFILE);
	for (int i = 0; i < 4; ++i) {
		if (tb_image[i] == 0)
			WIN32_CRITICAL_ERROR("LoadImage()");
	}
	tb_imlist = ImageList_Create( 32, 32, ILC_COLOR24, 4, 0);
	for (int i = 0; i < 4; ++i) {
		tb_imlist_idx[i] = ImageList_Add( tb_imlist, (HBITMAP)tb_image[i], 0);
	}
	
	basic_app::register_win32_class();
	
	// Create a hidden top-level window
	window_handle = CreateWindow(
		win32_class.lpszClassName,
		title.c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, // x
		CW_USEDEFAULT, // y
		384, // width
		256+32, // height
		0,
		0,
		GetModuleHandle(0),
		0 // No data passed to the WM_CREATE function.
	);
	windows[window_handle] = this;
	on_create(0,0);
}

basic_app::~basic_app()
{
	ImageList_Destroy( tb_imlist);
	for (int i = 0; i < 4; ++i) {
		DeleteObject( tb_image[i]);
	}
}

void
basic_app::run()
{
	// Make the top-level window visible.
	ShowWindow( window_handle, SW_SHOW);
	ShowWindow( toolbar, SW_SHOW);
	ShowWindow( scene.widget_handle, SW_SHOW);
	
	// Enter the message loop
	MSG message;
	while (true) {
		while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
			if (message.message == WM_QUIT)
				return;
			TranslateMessage( &message);
			DispatchMessage( &message);
		}
		if (!WaitMessage())
			WIN32_CRITICAL_ERROR( "WaitMessage()");
	}
}

} // !namespace cvisual;
