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
display* display::current = 0;

display::display()
  : window_visible(false)
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
	agl_context.makeCurrent();
	current = this;
}

void
display::gl_end()
{
	agl_context.makeNotCurrent();
	current = 0;
}

void
display::gl_swap_buffers()
{
	if (!window_visible) return;

	gl_begin();
	agl_context.swapBuffers();
	glFinish();	// Ensure rendering completes here (without the GIL) rather than at the next paint (with the GIL)
	gl_end();
}

void
display::create()
{
	python::gil_lock gil;  // protect statics like widgets, the shared display list context
	// Just to get going, set flags to 0:
	if (!agl_context.initWindow(title, window_x, window_y, window_width, window_height, 0)) {
		std::ostringstream msg;
		msg << "Could not create the window!\n";
		VPYTHON_CRITICAL_ERROR( msg.str());
		std::exit(1);
	}
}

void
display::destroy()
{
	agl_context.cleanup();
}

display::~display()
{
}

void
display::activate(bool active) {
	if (active) {
		VPYTHON_NOTE( "Opening a window from Python.");
		//SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL );
		gui_main::call_in_gui_thread( boost::bind( &display::create, this ) );
	} else {
		VPYTHON_NOTE( "Closing a window from Python.");
		gui_main::call_in_gui_thread( boost::bind( &display::destroy, this ) );
	}
}

display::EXTENSION_FUNCTION
display::getProcAddress(const char* name) {
	//return (EXTENSION_FUNCTION)::wglGetProcAddress( name ); // what instead of wglGetProcAddress?
}

/**************** Mac-specific stuff ***********************/

/*
Deprecated:
In aglFont::aglFont,
   FMGetFontFamilyFromName, GetAppFont, GetDefFontSize, FetchFontInfo, aglUseFont

In aglFont::getWidth,
   TextFont, TextFace, TextSize, TextWidth

In aglContext::initWindow,
   GetMainDevice, aglSetDrawable
*/

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

/*
aglFont*
aglContext::getFont(const char* description, double size)
{
	return new aglFont(*this, description, size);
}
*/

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
aglContext::changeWindow(std::string title, int x, int y, int width, int height, int flags)
{
	/*
	if (title) {
		SetWindowTitleWithCFString(window, CFStringCreateWithCString(NULL, title, kCFStringEncodingASCII));
	}
	*/
	
	if (x >= 0 && y >= 0) {
		wx = x;
		wy = y;
		MoveWindowStructure(window, wx, wy);
	}
	if (width > 0 && height > 0) {
		wwidth  = width;
		wheight = height;
		SizeWindow(window, wwidth, wheight, true);
	}
	return true;
}

bool
aglContext::initWindow(std::string title, int x, int y, int width, int height, int flags)
{
	GDHandle	dev;
	OSStatus	err;
	Rect		bounds;
	int			minY, idx;
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
	
	// Don't let window hide itself under the menu bar. Margin should be more than
	// the menu bar itself because sometimes the position gets set first and then
	// the title bar extended above that point.
	minY = GetMBarHeight() * 2;
	if (y < minY && ! (flags & aglContext::FULLSCREEN))
		y = minY;
	// Window 
	SetRect(&bounds, x, y, x + width, y + height);
	err = CreateNewWindow(kDocumentWindowClass, kWindowStandardDocumentAttributes, &bounds, &window);
	if (err != noErr)
		return false;
	this->changeWindow(title, x, y, width, height, flags);
	
	// GL context
	dev = GetMainDevice();
	idx = 0;
	while (attrList[idx] != AGL_NONE) idx ++;
	// Special
	if (flags & aglContext::FULLSCREEN) {
		attrList[idx] =	AGL_FULLSCREEN;
		idx ++;
	}
	if (flags & aglContext::QB_STEREO) {
		attrList[idx] = AGL_STEREO;
		idx ++;
	}
	fmt = aglChoosePixelFormat(&dev, 1, (const GLint *)attrList);
	if ((fmt == NULL || aglGetError() != AGL_NO_ERROR) && (flags & aglContext::QB_STEREO)) {
		// Try without stereo
		idx --;
		attrList[idx] = AGL_NONE;
		fmt = aglChoosePixelFormat(&dev, 1, (const GLint *)attrList);
	}

	if (fmt == NULL || aglGetError() != AGL_NO_ERROR)
		return false;
	ctx = aglCreateContext(fmt, NULL);
	if (ctx == NULL)
		return false;
	
	// Fullscreen on Mac
	if (flags & aglContext::FULLSCREEN) {
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
	while (handled[idx].eventClass != 0 || handled[idx].eventKind != 0) idx ++;
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
	if (! (flags & aglContext::FULLSCREEN)) {
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

/*
void destroy_context (glContext * cx)
{
	(static_cast<aglContext *>(cx))->cleanup();
}
*/


} // !namespace cvisual;
