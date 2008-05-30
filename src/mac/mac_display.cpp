// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "mac/display.hpp"
#include "util/render_manager.hpp"
#include "util/errors.hpp"
#include "python/gil.hpp"

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
using boost::thread;
#include <boost/lexical_cast.hpp>
using boost::lexical_cast;

namespace cvisual {

template<typename T>
inline T
clamp( T x, T a, T b)
{
	if (b<a) return std::max( b, std::min( x, a));
	else return std::max( a, std::min( x, b));
}

static bool modBit (int mask, int bit)
{
	return (mask & (1 << bit));
}

/**************************** display methods ************************************/
display::display()
  : widget_handle(0), dev_context(0), gl_context(0), window_visible(false)
{
}

void display::paint() {
	if (!window_visible) return;

	gl_begin();

	{
		python::gil_lock gil;
		render_scene();
	}

	gl_end();
}
 
void
display::gl_begin()
{
	if (!wglMakeCurrent( dev_context, gl_context))
		WIN32_CRITICAL_ERROR( "wglMakeCurrent failed");
	current = this;
}

void
display::gl_end()
{
	wglMakeCurrent( NULL, NULL );
	current = 0;
}

void
display::gl_swap_buffers()
{
	if (!window_visible) return;

	gl_begin();
	SwapBuffers( dev_context);
	glFinish();	// Ensure rendering completes here (without the GIL) rather than at the next paint (with the GIL)
	gl_end();
}

void
display::create()
{
	python::gil_lock gil;  // protect statics like widgets, the shared display list context
	
	register_win32_class();

	RECT screen;
	SystemParametersInfo( SPI_GETWORKAREA, 0, &screen, 0);
	int style = -1;
	
	int real_x = static_cast<int>(window_x);
	int real_y = static_cast<int>(window_y);
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
	
	update_size();
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
		SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL );
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

/******************************* Mac-specific stuff ***************************/
/**************** aglFont implementation *******************/


aglFont::aglFont(struct aglContext& _cx, 
		 const char *name, 
		 double size) 
	: cx(_cx), refcount(1)
{
		unsigned char pName[256];
		int ok;
		
		// Font names vary across platforms, so fall back if necessary
		strcpy((char *)(&(pName[1])), name);
		pName[0] = strlen(name);
		fID = FMGetFontFamilyFromName((unsigned char *)pName);
		if (fID <= 0)
			fID = GetAppFont();
		// No size means default
		if (size <= 0)
			size = GetDefFontSize();
		
		fSize = (int)size;
		FetchFontInfo(fID, fSize, 0, &fInfo);
		
		cx.makeCurrent();
		listBase = glGenLists(256);
		ok = aglUseFont(cx.getContext(), fID, 0, fSize, 0, 256, listBase);
		// if (! ok)	um, report something? Unlikely
		cx.makeNotCurrent();
}

aglFont::~aglFont()
{
	fID = -1;
}

void
aglFont::draw(const char *c)
{
	if (fID >= 0) {
		glListBase(listBase);
		glCallLists(strlen(c), GL_UNSIGNED_BYTE, c);
	}
}

double
aglFont::getWidth(const char *c)
{
	int		tw;
	
	if (fID == 0)
		return 0;
	
	// Still using old QuickDraw text measurement because I can't
	// figure out how to use ATSUI
	cx.makeCurrent();
	TextFont(fID);
	TextFace(0);
	TextSize(fSize);
	
	tw = TextWidth(c, 0, strlen(c));
	
	return (double)(tw  * 2) / cx.width();
}

double
aglFont::ascent()
{
	return (double)fInfo.ascent * 2 / cx.height();
}

double
aglFont::descent()
{
	return (double)fInfo.descent * 2 / cx.height();
}

void
aglFont::release()
{
	refcount--;
	if (refcount <= 0) {
		cx.add_pending_glDeleteList( listBase, 256);
		delete(this);
	}
}

glFont*
aglContext::getFont(const char* description, double size)
{
	return new aglFont(*this, description, size);
}


/*************** aglContext implementation *************/


aglContext::aglContext() 
 : window(0),
   ctx(0),
   wx(0), wy(0),
   wwidth(0), wheight(0),
   buttonState(0),
   buttonsChanged(0),
   keyModState(0),
   mouseLocked(false)
{
}

aglContext::~aglContext()
{
}

void aglContext::cleanup()
{
	wx	= -1;
	wy	= -1;
	wwidth	= 0;
	wheight	= 0;
	if (ctx)
		aglDestroyContext(ctx);
	ctx = NULL;
	if (window)
		DisposeWindow(window);
	window = NULL;
}


int
aglContext::getShiftKey()
{
	return modBit(keyModState, shiftKeyBit);
}

int
aglContext::getAltKey()
{
	return modBit(keyModState, optionKeyBit);
}

int
aglContext::getCtrlKey()
{
	// For Mac, treat command key as control also
	return modBit(keyModState, cmdKeyBit) || modBit(keyModState, controlKeyBit);
}

OSStatus
aglContext::vpWindowHandler (EventRef event)
{
	UInt32	kind;
	Rect	bounds;
	
	kind = GetEventKind(event);
	if (kind == kEventWindowBoundsChanged) {
		GetWindowBounds(window, kWindowStructureRgn, &bounds);
		wx = bounds.left;
		wy = bounds.top;
		wwidth  = bounds.right - bounds.left;
		wheight = bounds.bottom - bounds.top;
		// Tell OpenGL about it
		aglUpdateContext(ctx);
	} else if (kind == kEventWindowClosed) {
		// Safest way to destroy a window is by generating ESC key event,
		// which will be passed on and interpreted as close request
		keys.push("escape");
		return noErr;
	}
	return eventNotHandledErr;
}

OSStatus
aglContext::vpKeyboardHandler (EventRef event)
{
	UInt32	kind;
	char	key[2];
	UInt32	code;
	std::string keyStr;
	
	kind = GetEventKind(event);
	if (kind == kEventRawKeyDown || kind == kEventRawKeyRepeat) {
		GetEventParameter(event, kEventParamKeyMacCharCodes,
						  typeChar, NULL,
						  sizeof(char), NULL,
						  &(key[0]));
		key[1] = 0;
		if (isprint(key[0])) {
			// Easy
			keys.push(std::string(key));
		} else {
			keyStr = std::string("");
			GetEventParameter(event, kEventParamKeyModifiers,
							  typeUInt32, NULL,
							  sizeof(keyModState), NULL,
							  &keyModState);
			GetEventParameter(event, kEventParamKeyCode,
							  typeUInt32, NULL,
							  sizeof(code), NULL,
							  &code);
			if (this->getShiftKey())
				keyStr += "shift+";
			if (this->getCtrlKey())
				keyStr += "ctrl+";
			if (this->getAltKey())
				keyStr += "alt+";
			// Carbon doco is still in 1984 with no special keys...sigh
			switch (code) {
				// Apple really do number the F keys like this
				case 122:
					keyStr += "F1";
					break;
				case 120:
					keyStr += "F2";
					break;
				case 99:
					keyStr += "F3";
					break;
				case 118:
					keyStr += "F4";
					break;
				case 96:
					keyStr += "F5";
					break;
				case 97:
					keyStr += "F6";
					break;
				case 98:
					keyStr += "F7";
					break;
				case 100:
					keyStr += "F8";
					break;
				// F9-F12 are reserved by Apple for various purposes
				case 116:
					keyStr += "page up";
					break;
				case 121:
					keyStr += "page down";
					break;
				case 119:
					keyStr += "end";
					break;
				case 115:
					keyStr += "home";
					break;
				case 123:
					keyStr += "left";
					break;
				case 126:
					keyStr += "up";
					break;
				case 124:
					keyStr += "right";
					break;
				case 125:
					keyStr += "down";
					break;	
				case 117:
					keyStr += "delete";
					break;
				case 51:
					keyStr += "backspace";
					break;
				case 48:
					keyStr += "\t";
					break;
				case 36:
					keyStr += "\n";
					break;
				case 53:
					keyStr += "escape";
					break;
				default:
					keyStr += "unknown";
					break;
			}
		}
		keys.push(std::string(keyStr));
		return noErr;
	} else if (kind == kEventRawKeyModifiersChanged) {
		GetEventParameter(event, kEventParamKeyModifiers,
						  typeUInt32, NULL,
						  sizeof(keyModState), NULL,
						  &keyModState);
		return noErr;
	} else
		return eventNotHandledErr;
}

OSStatus
aglContext::vpMouseHandler (EventRef event)
{
	WindowPartCode	part;
	WindowRef		win;
	UInt32			kind;
	Point			pt;
	EventMouseButton btn;
	int				mask;
	
	// First must check if mouse event occurred within our content area
	GetEventParameter(event, kEventParamMouseLocation,
					  typeQDPoint, NULL,
					  sizeof(pt), NULL,
					  &pt);
	part = FindWindow(pt, &win);
	if (win != window || part != inContent)
		// Title bar, close box, ... pass to OS
		return eventNotHandledErr;
	
	// Get window-relative position and any modifier keys
	GetEventParameter(event, kEventParamWindowMouseLocation,
					  typeQDPoint, NULL,
					  sizeof(pt), NULL,
					  &pt);
	GetEventParameter(event, kEventParamKeyModifiers,
					  typeUInt32, NULL,
					  sizeof(keyModState), NULL,
					  &keyModState);
	mousePos.set_x(pt.h);
	mousePos.set_y(pt.v);

	// Mouse button change?
	kind = GetEventKind(event);
	if (kind == kEventMouseUp || kind == kEventMouseDown) {
		GetEventParameter(event, kEventParamMouseButton,
						  typeMouseButton, NULL,
						  sizeof(btn), NULL,
						  &btn);
		// VPython uses the middle and right buttons to adjust the viewpoint but there are
		// still a lot of Macs with one button mice, so translate option/command + button
		// into middle/right button.
		if (btn == kEventMouseButtonPrimary && modBit(keyModState, cmdKeyBit) != 0)
			btn = kEventMouseButtonSecondary;
		if (btn == kEventMouseButtonPrimary && modBit(keyModState, optionKeyBit) != 0)
			btn = kEventMouseButtonTertiary;
		// VPython numbering
		if (btn == kEventMouseButtonPrimary)
			mask = 1 << 0;
		else if (btn == kEventMouseButtonSecondary)
			mask = 1 << 1;
		else if (btn == kEventMouseButtonTertiary)
			mask = 1 << 2;
		// And update ourself
		if (kind == kEventMouseDown) {
			buttonState |= mask;
			buttonsChanged |= mask;
		} else {
			buttonState &= (~mask);
			buttonsChanged |= mask;
		}
	}
	
	return noErr;
}

static OSStatus vpEventHandler (EventHandlerCallRef target, EventRef event, void * data)
{
	UInt32	evtClass;
	aglContext * ctx;
	
	ctx = (aglContext *)data;
	
	evtClass = GetEventClass(event);
	
	switch (evtClass) {
		case kEventClassApplication:
			if (GetEventKind(event) == kEventAppQuit) {
				QuitApplicationEventLoop();
				return noErr;
			}
			break;
		case kEventClassWindow:
			return ctx->vpWindowHandler(event);
			break;
		case kEventClassKeyboard:
			return ctx->vpKeyboardHandler(event);
			break;
		case kEventClassMouse:
			return ctx->vpMouseHandler(event);
			break;
		default:
			break;
	}
	// Default is let OS do it
	return eventNotHandledErr;
}

bool
aglContext::changeWindow(const char* title, int x, int y, int width, int height, int flags)
{
	if (title) {
		SetWindowTitleWithCFString(window, CFStringCreateWithCString(NULL, title, kCFStringEncodingASCII));
	}
	
	if (x >= 0 && y >= 0) {
		wx = x;
		wy = y;
		MoveWindow(window, wx, wy, false);
	}
	if (width > 0 && height > 0) {
		wwidth  = width;
		wheight = height;
		SizeWindow(window, wwidth, wheight, true);
	}
	return true;
}

bool
aglContext::initWindow(const char* title, int x, int y, int width, int height, int flags)
{
	GDHandle	dev;
	OSStatus	err;
	Rect		bounds;
	int			idx;
	AGLPixelFormat	fmt;
	GLint		attrList[] = {
				AGL_RGBA, AGL_DOUBLEBUFFER,
				AGL_DEPTH_SIZE, 32,
				AGL_ALL_RENDERERS, AGL_ACCELERATED,
				AGL_NO_RECOVERY, 
				AGL_NONE,		// Expansion
				AGL_NONE,		// Expansion
				AGL_NONE
				};
	EventHandlerUPP	upp;
	EventHandlerRef	discard;
	EventTypeSpec	handled[] = {
		{ kEventClassApplication, kEventAppQuit },
		{ kEventClassWindow, kEventWindowClosed },
		{ kEventClassWindow, kEventWindowBoundsChanged },
		{ kEventClassKeyboard, kEventRawKeyDown },
		{ kEventClassKeyboard, kEventRawKeyRepeat },
		{ kEventClassKeyboard, kEventRawKeyModifiersChanged },
		{ kEventClassMouse, kEventMouseDown },
		{ kEventClassMouse, kEventMouseUp },
		{ kEventClassMouse, kEventMouseMoved },
		{ kEventClassMouse, kEventMouseDragged },
		{ 0, 0 }
	};
		
	// Window 
	SetRect(&bounds, x, y, width, height);
	err = CreateNewWindow(kDocumentWindowClass, kWindowStandardDocumentAttributes, &bounds, &window);
	if (err != noErr)
		return false;
	this->changeWindow(title, x, y, width, height, flags);
	
	// GL context
	dev = GetMainDevice();
	idx = 0;
	while (attrList[idx] != AGL_NONE) idx ++;
	// Special
	if (flags & glContext::FULLSCREEN) {
		attrList[idx] =	AGL_FULLSCREEN;
		idx ++;
	}
	fmt = aglChoosePixelFormat(&dev, 1, (const GLint *)attrList);
	if (fmt == NULL || aglGetError() != AGL_NO_ERROR)
		return false;
	ctx = aglCreateContext(fmt, NULL);
	if (ctx == NULL)
		return false;
	
	// Fullscreen on Mac
	if (flags & glContext::FULLSCREEN) {
		aglEnable(ctx, AGL_FS_CAPTURE_SINGLE);
		aglSetFullScreen(ctx, 0, 0, 0, 0);
	} else {
		aglSetDrawable(ctx, GetWindowPort(window));
	}
	aglDestroyPixelFormat(fmt);

	// Set up event handling
	InstallStandardEventHandler(GetWindowEventTarget(window));
	upp = NewEventHandlerUPP(vpEventHandler);
	idx = 0;
	while (handled[idx].eventClass != 0 && handled[idx].eventKind != 0) idx ++;
	// Events within window
	InstallEventHandler(GetWindowEventTarget(window),
						upp,
						idx, handled,
						this, &discard);
	// This handles menu bar quit and all events in fullscreen mode
	InstallEventHandler(GetApplicationEventTarget(),
						upp,
						idx, handled,
						this, &discard);
	// Make visible
	if (! (flags & glContext::FULLSCREEN)) {
		ActivateWindow(window, true);
		ShowWindow(window);
	}
	
	return true;
}

bool
aglContext::isOpen()
{
	return window != NULL;
}

void
aglContext::lockMouse()
{
}

void
aglContext::unlockMouse()
{
}

void
aglContext::showMouse()
{
	ShowCursor();
}

void
aglContext::hideMouse()
{
	HideCursor();
}

vector
aglContext::getMousePos()
{
	if (!window)
		return vector(0,0,0);
	
	vector tmp = mousePos;
	tmp.x /= wwidth;
	tmp.y /= wheight;
	
	return tmp;
}

vector
aglContext::getMouseDelta()
{
	// GL units (% of window)
	vector tmp = mousePos - oldMousePos;
	oldMousePos = mousePos;
	
	return tmp;
}

int
aglContext::getMouseButtons()
{
	return buttonState;
}

int
aglContext::getMouseButtonsChanged()
{
	int c = buttonsChanged;
	buttonsChanged = 0;
	return c; 
}

std::string
aglContext::getKeys()
{
	if (!keys.empty()) {
		std::string s = keys.front();
		keys.pop();
		return s;
	} 
	return std::string("");
}

void
aglContext::makeCurrent()
{
	SetPortWindowPort(window);
	aglSetCurrentContext(ctx);
	delete_pending_lists();
}

void
aglContext::makeNotCurrent()
{
	aglSetCurrentContext(NULL);
}

void
aglContext::swapBuffers()
{
	aglSwapBuffers(ctx);
}

int
aglContext::winX()
{
	return wx;
}

int
aglContext::winY()
{
	return wy;
}

int
aglContext::width()
{
	return wwidth;
}

int
aglContext::height()
{
	return wheight;
}

void destroy_context (glContext * cx)
{
	(static_cast<aglContext *>(cx))->cleanup();
}

/******************************* gui_main implementation **********************/

gui_main* gui_main::self = 0;  // Protected by python GIL

boost::signal<void()> gui_main::on_shutdown;

const int CALL_MESSAGE = WM_USER;

gui_main::gui_main()
 : gui_thread(-1)
{
}

LRESULT
gui_main::threadMessage( int code, WPARAM wParam, LPARAM lParam ) {
	if (wParam == PM_REMOVE) {
		MSG& message = *(MSG*)lParam;
		
		if (message.hwnd ==0 && message.message == CALL_MESSAGE ) {
			boost::function<void()>* f = (boost::function<void()>*)message.wParam;
			(*f)();
			delete f;
		}
	}
	return CallNextHookEx( NULL, code, wParam, lParam );
}

#define TIMER_RESOLUTION 5
void __cdecl end_period() { timeEndPeriod(TIMER_RESOLUTION); }

void
gui_main::run()
{
	MSG message;
	
	// Create a message queue and hook thread messages (we can't just check for them in the loop
	// below, becomes Windows runs modal message loops e.g. when resizing a window)
	SetWindowsHookEx(WH_GETMESSAGE, &gui_main::threadMessage, NULL, GetCurrentThreadId());
	PeekMessage(&message, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	
	// Tell the initializing thread
	{
		lock L(init_lock);
		gui_thread = GetCurrentThreadId();
		initialized.notify_all();
	}
	
	timeBeginPeriod(TIMER_RESOLUTION);
	atexit( end_period );
	
	poll();

	// Enter the message loop
	while (GetMessage(&message, 0, 0, 0) > 0) {
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

VOID CALLBACK gui_main::timer_callback( PVOID, BOOLEAN ) {
	// Called in high-priority timer thread when it's time to render
	self->call_in_gui_thread( boost::bind( &gui_main::poll, self ) );
}

void gui_main::poll() {
	// Called in gui thread when it's time to render
	// We don't need the lock here, because displays can't be created or destroyed from Python
	// without a message being processed by the GUI thread.  paint_displays() will pick
	// the lock up as necessary to synchronize access to the actual display contents.

	std::vector<display*> displays;
	for(std::map<HWND, display*>::iterator i = widgets.begin(); i != widgets.end(); ++i )
		if (i->second)
			displays.push_back( i->second );

	int interval = int( 1000. * render_manager::paint_displays( displays ) );
	CreateTimerQueueTimer( &timer_handle, NULL, &timer_callback, NULL, interval, 0, WT_EXECUTEINTIMERTHREAD );
}

} // !namespace cvisual;
