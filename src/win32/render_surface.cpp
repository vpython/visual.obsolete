
#include "win32/render_surface.hpp"
#include "util/errors.hpp"

#include <cstdlib>

/**************************** Utilities ************************************/
// Extracts and decodes a Win32 error message.
static void
decode_win32_error()
{
	DWORD code = GetLastError();
	char* message = 0;
	FormatMessage( 
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		0, // Ignored parameter
		code, // Error code to be decoded.
		0, // Use the default language
		&message, // The output buffer to fill the message
		1, // Allocate at least one byte in order to free something.
		0); // No additional arguments.
	
	VPYTHON_CRITICAL_ERROR(message);
	LocalFree(message);
	std::exit(1);
}

/**************** render_surface implementation  *****************/

// A lookup-table for the default window procedure to use to find the actual
// window that should handle a particular callback message.
std::map<HWND, render_surface*> render_surface::widgets;

void
render_surface::register_win32_class()
{
	static bool done = false;
	if (done)
		return;
	else {
		memset( &win32_class, 0, sizeof(win32_class));
		// TODO: Add extended styles WS_VISIBLE WS_CHILD
		win32_class.lpszClassName = "vpython_win32_render_surface";
		win32_class.lpfnWndProc = &dispatch_messages;
		win32_class.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
		win32_class.hInstance = GetModuleHandle(0);
		win32_class.hIcon = LoadIcon( NULL, IDI_APPLICATION );
		win32_class.hCursor = LoadCursor( NULL, IDC_ARROW );
		if (!RegisterClass( &win32_class))
			decode_win32_error();
		done = true;
	}
}

// This function dispatches incoming messages to the particular message-handler.
LRESULT CALLBACK 
render_surface::dispatch_messages( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	render_surface* This = widgets[hwnd];
	switch (uMsg) {
		// Handle the cases for the kinds of messages that we are listening for.
		case WM_DESTROY:
			return This->on_destroy( wParam, lParam);
		case WM_SIZE:
			return This->on_size( wParam, lParam);
		case WM_PAINT:
			// Cause the window to be painted.
			return This->on_paint( wParam, lParam);
		case WM_MOUSEMOVE:
			// Handle mouse cursor movement.
			return This->on_mousemove( wParam, lParam);
		case WM_CREATE:
			// Handle creation of the window.
			return This->on_create( wParam, lParam);
		case WM_ACTIVATE:
			// When the window is min/maximized.
		default:
			return DefWindowProc( hwnd, uMsg, wParam, lParam);
	}
}

VOID CALLBACK
render_surface::timer_callback( HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	render_surface* This = widgets[hwnd];
	This->do_paint();
}

LRESULT 
render_surface::on_create( WPARAM wParam, LPARAM lParam)
{
	// The first OpenGL Rendering Context, used to share displaylists.
	static HGLRC root_glrc = 0;
	
	dev_context = GetDC(widget_handle);
	
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
	if (!root_glrc) 
		root_glrc = gl_context;
	else
		wglShareLists( root_glrc, gl_context);
	
	ShowWindow( widget_handle, SW_SHOW);
	core.report_realize();

	// Initialize the timer routine
	UINT id = 0;
	SetTimer( widget_handle, &id, 28, &render_surface::dispatch_messages);
	return 0;
}

LRESULT  
render_surface::on_mousemove( WPARAM wParam, LPARAM lParam)
{
	bool left_down = wParam & MK_LBUTTON;
	bool middle_down = wParam & MK_MBUTTON;
	bool right_down = wParam & MK_RBUTTON;
	float mouse_x = LOWORD(lParam);
	float mouse_y = HIGHWORD(lParam);
	
	float dx = mouse_x - last_mousepos_x;
	float dy = mouse_y - last_mousepos_y;
	if (middle_down) {
		core.report_mouse_motion( dx, dy, render_core::MIDDLE);
	}
	if (right_down) {
		core.report_mouse_motion( dx, dy, render_core::RIGHT);
	}
	last_mousepos_x = mouse_x;
	last_mousepos_y = mouse_y;
	return 0;
}

LRESULT  
render_surface::on_size( WPARAM wParam, LPARAM lParam)
{
	RECT dims;
	GetClientRect( window_handle, &dims);
	core.report_resize( dims.right - dims.left, dims.bottom - dims.top);
	return 0;
}

LRESULT  
render_surface::on_paint( WPARAM wParam, LPARAM lParam)
{
	core.render_scene();
	return 0;
}

LRESULT  
render_surface::on_destroy( WPARAM wParam, LPARAM lParam)
{
	// Perform cleanup actions.
	UINT id = 0;
	KillTimer( widget_handle, &id);
	wglDeleteContext( gl_context);
	ReleaseDC( dev_context);
	widgets.remove( widget_handle);
	DestroyWindow( widget_handle);
	return 0;
}

WNDCLASS render_surface::win32_class;

// Callbacks provided to the render_core object.
void 
render_surface::gl_begin()
{
	wglMakeCurrent( dev_context, gl_context);
}

void 
render_surface::gl_end()
{
}

void 
render_surface::swap_buffers()
{
	SwapBuffers( dev_context);
}

render_surface::render_surface()
{
	// Initialize data
	// Initialize the win32_class
	register_win32_class();
}

void
render_surface::complete_init( HWND parent)
{
	CreateWindow(
		win32_class.lpszClassName,
		0, // No window title for this surface.
		CW_USEDEFAULT, // x TODO: Implement window resizing for this child.
		CW_USEDEFAULT, // y
		CW_USEDEFAULT, // width
		CW_USEDEFAULT, // height
		parent,
		1, // A unique index for to identify this widget by the parent
		GetModuleHandle(0),
		0
	); // No data passed to the WM_CREATE function.
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
	switch (uMsg) {
		case WM_COMMAND:
			// A Command from a particular button.
			return This->on_command( wpara, lParam);
		case WM_SIZE:
			return This->on_size( wParam, lParam);
		case WM_CREATE:
			return This->on_create( wParam, lParam);
		case WM_CLOSE:
			DestroyWindow( window_handle);
			return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		default:
			return DefWindowProc( hwnd, uMsg, wParam, lParam);
	}	
}

// Called in response to WM_RESIZE.
LRESULT
basic_app::on_size( WPARAM wParam, LPARAM lParam)
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
enum str_idx {
	PAN,
	ROTATE_ZOOM,
	FULLSCREEN,
	QUIT,
	FULLSCREEN_TT,
	QUIT_TT,
	SEPARATOR
};
const char** toolbar_strs = {
	"Pan",
	"Rotate/Zoom",
	"Fullscreen",
	"Quit",
	"Toggle fullscreen mode on/off",
	"Exit this VPython program"
};

// Called only once in response to WM_CREATE.  This function allocates size and
// creates its children.
LRESULT
basic_app::on_create( WPARAM wParam, LPARAM lParam)
{
	// TODO: Figure out something better than I_IMAGENONE, since that requires 
	// WinME.  Each of these are just going to have to get bitmaps, somehow.
	TBBUTTON buttons[5];
	// The Quit button.
	buttons[0].iBitmap = I_IMAGENONE;
	buttons[0].idCommand = QUIT;
	buttons[0].fsState = TBSTATE_ENABLED;
	buttons[0].fsStyle = TBSTYLE_AUTOSIZE | TBSTYLE_BUTTON;
	buttons[0].dwData = 0;
	buttons[0].iString = toolbar_strs[QUIT];
	
	buttons[1].iBitmap = I_IMAGENONE;
	buttons[1].idCommand = FULLSCREEN;
	buttons[1].fsState = TBSTATE_ENABLED;
	buttons[1].fsStyle = TBSTYLE_AUTOSIZE | TBSTYLE_CHECK;
	buttons[1].dwData = 0;
	buttons[1].iString = toolbar_strs[FULLSCREEN];
	
	buttons[2].iBitmap = I_IMAGENONE;
	buttons[2].idCommand = SEPARATOR;
	buttons[2].fsState = TBSTATE_ENABLED;
	buttons[2].fsStyle = TBSTYLE_AUTOSIZE | TBSTYLE_SEP;
	buttons[2].dwData = 0;
	buttons[2].iString = 0;

	buttons[3].iBitmap = I_IMAGENONE;
	buttons[3].idCommand = ROTATE_ZOOM;
	buttons[3].fsState = TBSTATE_ENABLED;
	buttons[3].fsStyle = TBSTYLE_AUTOSIZE | TBSTYLE_CHECKGROUP;
	buttons[3].dwData = 0;
	buttons[3].iString = toolbar_strs[ROTATE_ZOOM];

	buttons[4].iBitmap = I_IMAGENONE;
	buttons[4].idCommand = PAN;
	buttons[4].fsState = TBSTATE_ENABLED;
	buttons[4].fsStyle = TBSTYLE_AUTOSIZE | TBSTYLE_CHECKGROUP;
	buttons[4].dwData = 0;
	buttons[4].iString = toolbar_strs[PAN];
	
	toolbar = CreateWindowEx(
		0,
		TOOLBARCLASSNAME,
		0,
		WS_CHILD | CCS_ADJUSTABLE, 0, 0, 0, 0, 
		window_handle, 
		(HMENU) ID_TOOLBAR, 
		GetModuleHandle(0), 
        0
	);
	SendMessage(toolbar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);
	// Add the buttons to the toolbar widget.
	
	// Create the render_surface widget
	scene.complete_init();
	return 0;
	
}

LRESULT
basic_app::on_command( WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wparam)) {
		case QUIT:
			DestroyWindow(window_handle);
			break;
		case ROTATE_ZOOM:
			scene.core.mouse_mode = render_core::ZOOM_ROTATE;
			break;
		case FULLSCREEN:
			// Toggle the window fullscreen, somehow.
			break;
		case PAN:
			scene.core.mouse_mode = render_core::PAN;
		default:
			return DefWindowProc( window_handle, wParam, lParam);
	}
	return 0;	
};

void
basic_app::register_win32_class()
{
	static bool done = false;
	if (done)
		return;
	else {
		INITCOMMONCONTROLSEX comctl_init = {
			sizeof (INITCOMMONCONTROLSEX),
			ICC_BAR_CLASSES
		};
		InitCommonControlsEx( &comctl_init);
		
		// Register the top-level window class information to Windows.
		done = true;
	}
}

WNDCLASS* basic_app::win32_class = 0;

basic_app::basic_app()
{
	// Register (if needed) the window class for this structure.
	// Create a hidden top-level window
	// Compose and initialize the toolbar
	
}

basic_app::~basic_app()
{
}

void
basic_app::run()
{
	// Make the top-level window visible.
	ShowWindow( window_handle, SW_SHOW);
	
	// Enter the message loop
	MSG message;
	BOOL msg_err = 0;
	while (msg_err = GetMessage( &message, 0, 0, 0) != 0) {
		if (msg_err == -1)
			decode_win32_error();
		TranslateMessage( &message);
		DispatchMessage( &message);
	}
}
