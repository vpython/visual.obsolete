// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "win32/display.hpp"
#include "util/errors.hpp"

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

// TODO: Change mouse movement handling to lock the mouse in place and continue
// to process events.
// This function dispatches incoming messages to the particular message-handler.
LRESULT CALLBACK
display::dispatch_messages( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	using namespace cvisual;
	display* This = display::widgets[hwnd];
	if (This == 0)
		return DefWindowProc( hwnd, uMsg, wParam, lParam);

	switch (uMsg) {
		// Handle the cases for the kinds of messages that we are listening for.
		case WM_CLOSE:
			return This->on_close( wParam, lParam);
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
		default:
			return DefWindowProc( hwnd, uMsg, wParam, lParam);
	}
}

VOID CALLBACK
display::timer_callback( HWND hwnd, UINT, UINT_PTR, DWORD)
{
	using namespace cvisual;
	display* This = display::widgets[hwnd];
	if (0 == This)
		return;
	This->on_paint(0, 0);
}

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

/**************** display implementation  *****************/

// A lookup-table for the default widget procedure to use to find the actual
// widget that should handle a particular callback message.
std::map<HWND, display*> display::widgets;
display* display::current = 0;
shared_ptr<display> display::selected;

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
	UINT id = 1;
	switch (lParam) {
		case 0:
			// Opening for the first time.
			report_realize();
			SetTimer( widget_handle, id, 30, &timer_callback);
			break;
		case SW_PARENTCLOSING:
			// Stop rendering when the window is minimized.
			KillTimer( widget_handle, id);
			break;
		case SW_PARENTOPENING:
			// restart rendering when the window is restored.
			SetTimer( widget_handle, id, 30, &timer_callback);
			break;
		default:
			return DefWindowProc( widget_handle, WM_SHOWWINDOW, wParam, lParam);
	}
	return 0;
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
display::on_size( WPARAM, LPARAM)
{
	RECT dims;
	// The following calls report the fact that the widget area has been
	// repainted to the windowing system.
	GetClientRect( widget_handle, &dims);
	if (dims.right != window_width || dims.bottom != window_height) {
		window_width = (float) dims.right;
		window_height = (float) dims.bottom;
		report_resize( window_width, window_height);
	}
	return 0;
}

LRESULT
display::on_move( WPARAM, LPARAM lParam)
{
	x = LOWORD( lParam);
	y = HIWORD( lParam);
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

	bool sat = render_scene();
	if (!sat)
		return 1;

	boost::tie( mouse.pick, mouse.pickpos, mouse.position) =
		pick( last_mousepos_x, last_mousepos_y);
	// TODO: Add timer cycle time munging (or at least consider it)
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
	VPYTHON_NOTE( "Closing a window from the GUI");
	UINT id = 1;
	KillTimer( widget_handle, id);
	if (exit) {
		//gl_free();
	}
	gui_main::report_window_delete(this);
	wglDeleteContext( gl_context);
	ReleaseDC( widget_handle, dev_context);
	widgets.erase( widget_handle);
	DestroyWindow(widget_handle);
	if (exit) {
		PostQuitMessage(0);
	}
	active = false;
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
			gui_main::report_window_delete(this);
			if (exit)
				gui_main::quit();
			return 0;
		}
		keys.push( std::string(kStr));
	}
	
	return 0;
}

WNDCLASS display::win32_class;

// Callbacks provided to the display_kernel object.
void
display::on_gl_begin()
{
	saved_dc = wglGetCurrentDC();
	saved_glrc = wglGetCurrentContext();
	if (!wglMakeCurrent( dev_context, gl_context))
		WIN32_CRITICAL_ERROR( "wglMakeCurrent failed");
	current = this;
}

void
display::on_gl_end()
{
	wglMakeCurrent( saved_dc, saved_glrc);
	saved_dc = 0;
	saved_glrc = 0;
	current = 0;
}

void
display::on_gl_swap_buffers()
{
	SwapBuffers( dev_context);
}

display::display()
	: x(-1), y(-1),
	exit(true), visible(true), fullscreen(false), title( "VPython"),
	Kshift(false), Kctrl(false), Kalt(false),
	window_width(430), window_height(430),
	widget_handle(0), timer_handle(0), dev_context(0), gl_context(0),
	saved_dc(0), saved_glrc(0),
	last_mousepos_x(0), last_mousepos_y(0), mouselocked( false), active(false)
{
	// Connect callbacks from the display_kernel to this object.  These will not
	// be called back from the core until report_realize is called.
	gl_begin.connect(
		boost::bind(&display::on_gl_begin, this));
	gl_end.connect(
		boost::bind(&display::on_gl_end, this));
	gl_swap_buffers.connect(
		boost::bind(&display::on_gl_swap_buffers, this));
}

void
display::create()
{
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
	active = true;
	visible = true;
	ShowWindow( widget_handle, SW_SHOW);
}

void
display::destroy()
{
	CloseWindow( widget_handle);
	// widget_handle = 0;
}

display::~display()
{
}

void
display::set_x( float n_x)
{
	lock L(mtx);
	if (active) {
		throw std::runtime_error( "Cannot change parameters of an active window");
	}
	else
		x = n_x;
}
float
display::get_x()
{
	return x;
}

void
display::set_y( float n_y)
{
	lock L(mtx);
	if (active) {
		throw std::runtime_error( "Cannot change parameters of an active window");
	}
	else
		y = n_y;
}
float
display::get_y()
{
	return y;
}

void
display::set_width( float w)
{
	lock L(mtx);
	if (active) {
		throw std::runtime_error( "Cannot change parameters of an active window");
	}
	else
		window_width = w;
}
float
display::get_width()
{
	return window_width;
}

void
display::set_height( float h)
{
	lock L(mtx);
	if (active) {
		throw std::runtime_error( "Cannot change parameters of an active window");
	}
	else
		window_height = h;;
}
float
display::get_height()
{
	return window_height;
}

void
display::set_visible( bool vis)
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
display::get_visible()
{
	if (!active)
		return false;
	return visible;
}

void
display::set_title( std::string n_title)
{
	lock L(mtx);
	if (active) {
		throw std::runtime_error( "Cannot change parameters of an active window");
	}
	else
		title = n_title;
}
std::string
display::get_title()
{
	return title;
}

bool
display::is_fullscreen()
{
	return fullscreen;
}
void
display::set_fullscreen( bool fs)
{
	lock L(mtx);
	if (active) {
		throw std::runtime_error( "Cannot change parameters of an active window");
	}
	else
		fullscreen = fs;
}

int
display::get_titlebar_height()
{
#if !(defined(_WIN32) || defined(_MSC_VER))
	return 23;
#else
	return 25; // Ubuntu Linux; unknown what situation is on Mac
#endif
}

int
display::get_toolbar_height()
{
	return 37;
}

bool
display::is_showing_toolbar()
{
	return show_toolbar;
}

void
display::set_show_toolbar( bool fs)
{
	if (active)
		throw std::runtime_error( 
			"Cannot change the window's state after initialization.");
	show_toolbar = fs;
}

void
display::add_renderable( shared_ptr<renderable> obj)
{
	display_kernel::add_renderable( obj);
	if (!active && visible) {
		gui_main::add_display(this);
	}
}

void
display::add_renderable_screen( shared_ptr<renderable> obj)
{
	display_kernel::add_renderable( obj);
	if (!active && visible)
		gui_main::add_display(this);
}

mouse_t*
display::get_mouse()
{
	if (!visible)
		visible = true;
	if (!active)
		gui_main::add_display( this);

	return &mouse;
}

atomic_queue<std::string>*
display::get_kb()
{
	if (!visible)
		visible = true;
	if (!active)
		gui_main::add_display( this);

	return &keys;
}

void
display::set_selected( shared_ptr<display> d)
{
	selected = d;
}

shared_ptr<display>
display::get_selected()
{
	return selected;
}

display::EXTENSION_FUNCTION
display::getProcAddress(const char* name) {
	return (EXTENSION_FUNCTION)::wglGetProcAddress( name );
}

/******************************* gui_main implementatin **********************/

gui_main* gui_main::self = 0;
mutex* volatile gui_main::init_lock = 0;
condition* volatile gui_main::init_signal = 0;
boost::signal<void()> gui_main::on_shutdown;


gui_main::gui_main()
	: idThread(GetCurrentThreadId()),
	caller( 0), returned( false), waiting_allclosed(false),
	thread_exited(false), shutting_down( false)
{
	// Force the create of a message queue
	MSG message;
	PeekMessage( &message, NULL, WM_USER, WM_USER, PM_NOREMOVE);
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
			if (message.message == WM_QUIT) {
				VPYTHON_NOTE( "WM_QUIT recieved");
				goto quit;
			}

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
			//if(!self->shutting_down)
			//if(!(self->shutting_down && (message.message == WM_PAINT)))
			{
				// Destined for the primary window procedure above
				TranslateMessage( &message);
				DispatchMessage( &message);
			}
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
	
	// Brandmeyer suggestion: (http://msdn2.microsoft.com/en-us/library/ms685100.aspx)
	/*
	if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST)) {
		VPYTHON_WARNING("Could not raise the rendering thread priority");
	}
	else
	{
		VPYTHON_NOTE("Raised the rendering thread priority to THREAD_PRIORITY_HIGHEST");
	}
	*/
	
	// Try opposite priority:
	/*
	if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST)) {
		VPYTHON_WARNING("Could not lower the rendering thread priority");
	}
	else
	{
		VPYTHON_NOTE("Lowered the rendering thread priority to THREAD_PRIORITY_LOWEST");
	}
	*/
	
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
		thread gui( &gui_main::thread_proc);
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
	self->shutting_down = true;
	self->signal_shutdown();
//	while (!self->returned)
//		self->call_complete.py_wait(L);
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
		//gui_main::shutdown();
	}
	if (display_empty){
		if  (self->waiting_allclosed)
			gui_main::quit();
		else
			gui_main::shutdown();
	}//*/
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