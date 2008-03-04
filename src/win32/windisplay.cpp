// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "win32/display.hpp"
#include "util/errors.hpp"
#include "python/gil.hpp"

// For GET_X_LPARAM, GET_Y_LPARAM
#include <windowsx.h>

#include <cstdlib>
#include <cstring>
#include <cassert>
#include <iostream>

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
using boost::thread;
#include <boost/lexical_cast.hpp>
using boost::lexical_cast;

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

	std::cerr << "VPython ***Win32 Critical Error*** " << file << ":" << line << ": "
		<< func << ": " << msg << ": " << message;
	LocalFree(message);
	std::exit(1);
}

// The first OpenGL Rendering Context, used to share displaylists.
static HGLRC root_glrc = 0;
WNDCLASS display::win32_class;

// A lookup-table for the default widget procedure to use to find the actual
// widget that should handle a particular callback message.
std::map<HWND, display*> display::widgets;
display* display::current = 0;

display::display()
  : Kshift(false), Kctrl(false), Kalt(false),
	widget_handle(0), timer_handle(0), dev_context(0), gl_context(0),
	saved_dc(0), saved_glrc(0),
	last_mousepos_x(0), last_mousepos_y(0), mouselocked( false)
{
}

// TODO: Change mouse movement handling to lock the mouse in place and continue
// to process events.
// This function dispatches incoming messages to the particular message-handler.
LRESULT CALLBACK
display::dispatch_messages( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	{
		python::gil_lock gil;

		display* This = widgets[hwnd];
		if (This == 0)
			return DefWindowProc( hwnd, uMsg, wParam, lParam);

		switch (uMsg) {
			// Handle the cases for the kinds of messages that we are listening for.
			case WM_CLOSE:
				return This->on_close( wParam, lParam);
			case WM_DESTROY:
				return This->on_destroy( wParam, lParam );
			case WM_SIZE:
				return This->on_size( wParam, lParam);
			case WM_MOVE:
				return This->on_move( wParam, lParam);
			case WM_PAINT:
				return This->on_paint( wParam, lParam);
			case WM_MOUSEMOVE:
				// Handle mouse cursor movement.
				return This->on_mousemove( wParam, lParam);
			case WM_SHOWWINDOW:
				return This->on_showwindow( wParam, lParam);
			//case WM_LBUTTONDBLCLK:
			case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN:
				return This->on_buttondown( wParam, lParam);
			case WM_LBUTTONUP: case WM_RBUTTONUP: case WM_MBUTTONUP:
				return This->on_buttonup( wParam, lParam);
    		case WM_KEYUP:
				return This->on_keyUp(uMsg, wParam, lParam);
    		case WM_KEYDOWN:
    			return This->on_keyDown(uMsg, wParam, lParam);
    		case WM_CHAR:
    			return This->on_keyChar(uMsg, wParam, lParam);
			case WM_GETMINMAXINFO:
				return This->on_getminmaxinfo( wParam, lParam);
		}
	}
	return DefWindowProc( hwnd, uMsg, wParam, lParam);
}

VOID CALLBACK
display::timer_callback( HWND hwnd, UINT, UINT_PTR, DWORD)
{
	python::gil_lock gil;
	
	display* This = widgets[hwnd];
	if (0 == This)
		return;
	This->on_paint(0, 0);
}

void
display::register_win32_class()
{
	static bool done = false;
	if (done)
		return;
	else {
		std::memset( &win32_class, 0, sizeof(win32_class));

		win32_class.lpszClassName = "vpython_win32_render_surface";
		win32_class.lpfnWndProc = &dispatch_messages;
		win32_class.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
		win32_class.hInstance = GetModuleHandle(0);
		win32_class.hIcon = LoadIcon( NULL, IDI_APPLICATION );
		win32_class.hCursor = LoadCursor( NULL, IDC_ARROW );
		if (!RegisterClass( &win32_class))
			WIN32_CRITICAL_ERROR("RegisterClass()");
		done = true;
	}
}

LRESULT
display::on_showwindow( WPARAM wParam, LPARAM lParam)
{
	switch (lParam) {
		case 0:
			// Opening for the first time.
			SetTimer( widget_handle, 1, 30, &timer_callback);
			break;
		case SW_PARENTCLOSING:
			// Stop rendering when the window is minimized.
			KillTimer( widget_handle, 1);
			break;
		case SW_PARENTOPENING:
			// restart rendering when the window is restored.
			SetTimer( widget_handle, 1, 30, &timer_callback);
			break;
		default:
			return DefWindowProc( widget_handle, WM_SHOWWINDOW, wParam, lParam);
	}
	return 0;
}

LRESULT
display::on_size( WPARAM, LPARAM)
{
	RECT dims;
	// The following calls report the fact that the widget area has been
	// repainted to the windowing system.
	GetClientRect( widget_handle, &dims);
	report_resize( x, y, (float) dims.right, (float) dims.bottom );
	return 0;
}

LRESULT
display::on_move( WPARAM, LPARAM lParam)
{
	report_resize( LOWORD( lParam ), HIWORD( lParam ), window_width, window_height );
	return 0;
}

LRESULT
display::on_getminmaxinfo( WPARAM, LPARAM lParam)
{
	MINMAXINFO* info = (MINMAXINFO*)lParam;
	// Prevents making the window too small.
	info->ptMinTrackSize.x = 70;
	info->ptMinTrackSize.y = 70;
	return 0;
}

LRESULT
display::on_paint( WPARAM, LPARAM)
{
	if (window_width < 1 || window_height < 1)
		return 0;

	gl_begin();
	bool sat = render_scene();
	gl_swap_buffers();
	gl_end();
	if (!sat) return 1;  // xxx
	
	gl_begin();
	boost::tie( mouse.pick, mouse.pickpos, mouse.position) =
		pick( last_mousepos_x, last_mousepos_y);
	gl_end();
	
	// It's very important for the render thread not to starve Python.  Since
	// it holds the gil_lock so much longer, it tends to monopolize it if it
	// doesn't sleep between renders.  However, note that this code does a poor
	// job of making the frame rate smooth if rendering takes varying time.  A
	// better implementation will need to use the multimedia timers for better 
	// precision.
	int next_render_interval = 30;
	SetTimer( widget_handle, 1, next_render_interval, &timer_callback);
	
	RECT dims;
	// The following calls report the fact that the widget area has been
	// repainted to the windowing system.
	GetClientRect( widget_handle, &dims);
	ValidateRect( widget_handle, &dims);
	return 0;
}

LRESULT
display::on_close( WPARAM, LPARAM)
{
	// Happens only when the user closes the window
	VPYTHON_NOTE( "Closing a window from the GUI");
	DestroyWindow(widget_handle);
	if (exit)
		PostQuitMessage(0);
	return 0;
}

LRESULT
display::on_destroy(WPARAM wParam,LPARAM lParam)
{
	// Happens after on_close, and also when a window is destroyed programmatically
	// (e.g. scene.visible = 0)
	report_closed();
	UINT id = 1;
	KillTimer( widget_handle, id);
	// We can only free the OpenGL context if it isn't the one we are using for display list sharing
	// xxx Display list sharing is the suck
	if ( gl_context != root_glrc )
		wglDeleteContext( gl_context );
	ReleaseDC( widget_handle, dev_context);
	widgets.erase( widget_handle );
	return DefWindowProc( widget_handle, WM_DESTROY, wParam, lParam );
}

void
display::gl_begin()
{
	saved_dc = wglGetCurrentDC();
	saved_glrc = wglGetCurrentContext();
	if (!wglMakeCurrent( dev_context, gl_context))
		WIN32_CRITICAL_ERROR( "wglMakeCurrent failed");
	current = this;
}

void
display::gl_end()
{
	wglMakeCurrent( saved_dc, saved_glrc);
	saved_dc = 0;
	saved_glrc = 0;
	current = 0;
}

void
display::gl_swap_buffers()
{
	SwapBuffers( dev_context);
}

void
display::create()
{
	python::gil_lock gil;  // protect statics like widgets, the shared display list context
	
	register_win32_class();

	RECT screen;
	SystemParametersInfo( SPI_GETWORKAREA, 0, &screen, 0);
	int style = -1;
	int real_x = static_cast<int>(x);
	int real_y = static_cast<int>(y);
	int real_width = static_cast<int>(window_width);
	int real_height = static_cast<int>(window_height);

	if (fullscreen) {
		real_x = screen.left;
		real_y = screen.top;
		real_width = screen.right - real_x;
		real_height = screen.bottom - real_y;
		style = WS_OVERLAPPED | WS_POPUP | WS_MAXIMIZE | WS_VISIBLE;
	}
	else if (real_x < 0 && real_y < 0 || real_x > screen.right || real_y > screen.bottom) {
		real_x = CW_USEDEFAULT;
		real_y = CW_USEDEFAULT;
	}
	else if (real_x < screen.left) {
		real_x = screen.left;
	}
	else if (real_y < screen.top) {
		real_y = screen.top;
	}

	if (real_x + real_width > screen.right)
		real_width = screen.right - real_x;
	if (real_y + real_height > screen.bottom)
		real_height = screen.bottom - real_y;

	if (!fullscreen)
		style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

	widget_handle = CreateWindow(
		win32_class.lpszClassName,
		title.c_str(),
		style,
		real_x, real_y,
		real_width, real_height,
		0,
		0, // A unique index to identify this widget by the parent
		GetModuleHandle(0),
		0 // No data passed to the WM_CREATE function.
	);
	widgets[widget_handle] = this;

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
		0,                               // no opacity buffer
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
display::destroy()
{
	DestroyWindow( widget_handle);
}

display::~display()
{
}

void
display::activate(bool active) {
	if (active) {
		VPYTHON_NOTE( "Opening a window from Python.");
		gui_main::call_in_gui_thread( boost::bind( &display::create, this ) );
	} else {
		VPYTHON_NOTE( "Closing a window from Python.");
		gui_main::call_in_gui_thread( boost::bind( &display::destroy, this ) );
	}
}

display::EXTENSION_FUNCTION
display::getProcAddress(const char* name) {
	return (EXTENSION_FUNCTION)::wglGetProcAddress( name );
}

LRESULT
display::on_mousemove( WPARAM wParam, LPARAM lParam)
{
	// TODO: Modify this implementation to make the mouse disappear and lock
	// it to a particular position during mouse right-clicks.
	bool left_down = wParam & MK_LBUTTON;
	bool middle_down = wParam & MK_MBUTTON;
	bool right_down = wParam & MK_RBUTTON;
	if (left_down && right_down) {
		middle_down = true;
		right_down = false;
		left_down = false;
	}
	bool buttondown = left_down || middle_down || right_down;
	float mouse_x = LOWORD(lParam);
	float mouse_y = HIWORD(lParam);

	float dx = mouse_x - last_mousepos_x;
	float dy = mouse_y - last_mousepos_y;
	// bool mouselocked = false;
	if (middle_down) {
		report_mouse_motion( dx, dy, display_kernel::MIDDLE);
		// mouselocked = true;
	}
	if (right_down) {
		report_mouse_motion( dx, dy, display_kernel::RIGHT);
		// mouselocked = true;
	}
	if (!buttondown)
		report_mouse_motion( dx, dy, display_kernel::NONE);

	mouse.set_shift( wParam & MK_SHIFT);
	mouse.set_ctrl( wParam & MK_CONTROL);
	mouse.set_alt( GetKeyState( VK_MENU) < 0);
	// if (mouselocked) {
	//	SetCursorPos( static_cast<int>(last_mousepos_x), static_cast<int>(last_mousepos_y));
	// }
	// else {
	last_mousepos_x = mouse_x;
	last_mousepos_y = mouse_y;
	// }
	mouse.cam = calc_camera();

	if (left_button.is_dragging())
		mouse.push_event( drag_event( 1, mouse));
	if (!zoom_is_allowed() && middle_button.is_dragging())
		mouse.push_event( drag_event( 2, mouse));
	if (!spin_is_allowed() && right_button.is_dragging())
		mouse.push_event( drag_event( 3, mouse));

	return 0;
}

LRESULT
display::on_buttondown( WPARAM wParam, LPARAM lParam)
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
	else if (wParam & MK_MBUTTON && middle_button.press( x, y)) {
		button_id = 2;
	}
	else if (wParam & MK_RBUTTON && right_button.press( x, y)) {
		button_id = 3;
	}

	if (button_id)
		mouse.push_event( press_event( button_id, mouse));
	return 0;
}

LRESULT
display::on_buttonup( WPARAM wParam, LPARAM lParam)
{
	mouse.set_shift( wParam & MK_SHIFT);
	mouse.set_ctrl( wParam & MK_CONTROL);
	mouse.set_alt( GetKeyState( VK_MENU) < 0);

	// float x = (float)GET_X_LPARAM(lParam);
	// float y = (float)GET_Y_LPARAM(lParam);

	int button_id = 0;
	std::pair<bool, bool> unique_drop( false, false);
	#define unique unique_drop.first
	#define drop unique_drop.second

	if (!(wParam & ~MK_LBUTTON)) {
		unique_drop = left_button.release();
		if (unique) {
			button_id = 1;
			goto found;
		}
	}
	if (!(wParam & ~MK_MBUTTON) && !zoom_is_allowed()) {
		unique_drop = middle_button.release();
		if (unique) {
			button_id = 2;
			goto found;
		}
	}
	if (!(wParam & ~MK_RBUTTON) && !spin_is_allowed()) {
		unique_drop = right_button.release();
		if (unique) {
			button_id = 3;
			goto found;
		}
	}
found:
	if (button_id)
		if (drop)
			mouse.push_event( drop_event( button_id, mouse));
		else {
			mouse.push_event( click_event( button_id, mouse));
		}
	return 0;
	#undef unique
	#undef drop
}

LRESULT
display::on_keyUp(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Kshift = (GetKeyState(VK_SHIFT) < 0) ||
		(GetKeyState(VK_CAPITAL) & 1);
	Kalt = GetKeyState(VK_MENU) < 0;
	Kctrl = GetKeyState(VK_CONTROL) < 0;

	return 0;
}

LRESULT
display::on_keyDown(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Note that this algorithm will proably fail if the user is using anything 
	// other than a US keyboard.
	char *kNameP;
	char kStr[60],fStr[4];
	
	Kshift = (GetKeyState(VK_SHIFT) < 0) ||
		(GetKeyState(VK_CAPITAL) & 1);
	Kalt = GetKeyState(VK_MENU) < 0;
	Kctrl = GetKeyState(VK_CONTROL) < 0;
	kStr[0] = 0;
	kNameP = NULL;
	
	switch (wParam) {
		
	case VK_F1:
	case VK_F2:
	case VK_F3:
	case VK_F4:
	case VK_F5:
	case VK_F6:
	case VK_F7:
	case VK_F8:
	case VK_F9:
	case VK_F10:
	case VK_F11:
	case VK_F12:
		sprintf(fStr,"f%d",wParam-VK_F1+1);
		kNameP = fStr;
		break;
		
	case VK_PRIOR:
		kNameP = "page up";
		break;
		
	case VK_NEXT:
		kNameP = "page down";
		break;
		
	case VK_END:
		kNameP = "end";
		break;
		
	case VK_HOME:
		kNameP = "home";
		break;
		
	case VK_LEFT:
		kNameP = "left";
		break;
		
	case VK_UP:
		kNameP = "up";
		break;
		
	case VK_RIGHT:
		kNameP = "right";
		break;
		
	case VK_DOWN:
		kNameP = "down";
		break;
		
	case VK_SNAPSHOT:
		kNameP = "print screen";
		break;
		
	case VK_INSERT:
		kNameP = "insert";
		break;
		
	case VK_DELETE:
		kNameP = "delete";
		break;
		
	case VK_NUMLOCK:
		kNameP = "numlock";
		break;
		
	case VK_SCROLL:
		kNameP = "scrlock";
		break;
		
	} // wParam
	
	if (kNameP) {
		if (Kctrl) strcat(kStr,"ctrl+");
		if (Kalt) strcat(kStr,"alt+");
		if (Kshift) strcat(kStr,"shift+");
		strcat(kStr,kNameP);
		keys.push( std::string(kStr));
	}
	
	return 0;
}

LRESULT
display::on_keyChar(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Note that this algorithm will proably fail if the user is using anything 
	// other than a US keyboard.
	int fShift,fAlt,fCtrl;
	char *kNameP;
	char kStr[60],wStr[2];
	
	if ((wParam >= 32) && (wParam <= 126))
	{
		char kk[2];
	    kk[0] = wParam;
	    kk[1] = 0;
	    keys.push( std::string(kk)); 
	    return 0;
	}
	
	fShift = (GetKeyState(VK_SHIFT) < 0) ||
		(GetKeyState(VK_CAPITAL) & 1);
	fAlt = GetKeyState(VK_MENU) < 0;
	fCtrl = GetKeyState(VK_CONTROL) < 0;
	kStr[0] = 0;
	kNameP = NULL;
	
	if (!fCtrl && wParam == VK_RETURN)
		kNameP = "\n";
	else if (!fCtrl && wParam == VK_ESCAPE)
		kNameP = "escape";
	else if (!fCtrl && wParam == VK_BACK)
		kNameP = "backspace";
	else if (!fCtrl && wParam == VK_TAB)
		kNameP = "\t";
	else if ((wParam > 0) && (wParam <= 26)) {
		wStr[0] = wParam-1+'a';
		wStr[1] = 0;
		kNameP = wStr;
	} else if (wParam == 27)
		kNameP = "[";
	else if (wParam == 28)
		kNameP = "\\";
	else if (wParam == 29)
		kNameP = "]";
	else if (wParam == 30)
		kNameP = "^";
	else if (wParam == 31)
		kNameP = "_";
	
	if (kNameP) {
		if (fCtrl) strcat(kStr,"ctrl+");
		if (fAlt) strcat(kStr,"alt+");
		if (fShift) strcat(kStr,"shift+");
		strcat(kStr,kNameP);			
		if (strcmp(kStr,"escape") == 0)
		{
			// Allow the user to delete a fullscreen window this way
			destroy();
			if (exit)
				PostQuitMessage(0);
			return 0;
		}
		keys.push( std::string(kStr));
	}
	
	return 0;
}

/******************************* gui_main implementatin **********************/

gui_main* gui_main::self = 0;  // Protected by python GIL

boost::signal<void()> gui_main::on_shutdown;

const int CALL_MESSAGE = WM_USER;

gui_main::gui_main()
 : gui_thread(-1)
{
}

void
gui_main::run()
{
	MSG message;

	// Create a message queue
	PeekMessage(&message, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	
	// Tell the initializing thread
	{
		lock L(init_lock);
		gui_thread = GetCurrentThreadId();
		initialized.notify_all();
	}

	// Enter the message loop
	while (GetMessage(&message, 0, 0, 0) > 0) {
		if (message.hwnd ==0 && message.message == CALL_MESSAGE ) {
			boost::function<void()>* f = (boost::function<void()>*)message.wParam;
			(*f)();
			delete f;
			continue;
		}

		// Destined for the primary window procedure above
		TranslateMessage( &message);
		DispatchMessage( &message);
	}

	// We normally exit the message queue because PostQuitMessage() has been called
	VPYTHON_NOTE( "WM_QUIT (or message queue error) received");
	on_shutdown(); // Tries to kill Python
}

void
gui_main::init_thread(void)
{
	if (!self) {
		// We are holding the Python GIL through this process, including the wait!
		// We can't let go because a different Python thread could come along and mess us up (e.g.
		//   think that we are initialized and go on to call PostThreadMessage without a valid idThread)
		self = new gui_main;
		thread gui( boost::bind( &gui_main::run, self ) );
		lock L( self->init_lock );
		while (self->gui_thread == -1)
			self->initialized.wait( L );
	}
}

void
gui_main::call_in_gui_thread( const boost::function< void() >& f )
{
	init_thread();
	PostThreadMessage( self->gui_thread, CALL_MESSAGE, (WPARAM)(new boost::function<void()>(f)), 0);
}

} // !namespace cvisual;
