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

static bool modBit (int mask, int bit)
{
	return (mask & (1 << bit));
}

static std::set<display*> widgets;

void 
init_platform() // called just once
{
	ProcessSerialNumber psn;
	CFDictionaryRef		app;
	const void *		backKey;
	const void *		background;
	int					err;
	
	VPYTHON_NOTE( "Start of init_platform.");
	//std::cout << "init_platform\n";
	// If we were invoked from python rather than pythonw, change into a GUI app
	GetCurrentProcess(&psn);
	app = ProcessInformationCopyDictionary(&psn, kProcessDictionaryIncludeAllInformationMask);
	backKey = CFStringCreateWithCString(NULL, "LSBackgroundOnly", kCFStringEncodingASCII);
	background = CFDictionaryGetValue(app, backKey);
	if (background == kCFBooleanTrue) {
		TransformProcessType(&psn, kProcessTransformToForegroundApplication);
	}
	SetFrontProcess(&psn);
	VPYTHON_NOTE( "End of init_platform.");
}

/*
Deprecated Carbon elements (which means doubly deprecated, since Carbon itself has little support):
In aglFont::aglFont in mac_font_renderer.cpp,
   FMGetFontFamilyFromName, GetAppFont, GetDefFontSize, FetchFontInfo, aglUseFont

In aglFont::getWidth in mac_font_renderer.cpp,
   TextFont, TextFace, TextSize, TextWidth

In display::initWindow,
   GetMainDevice, aglSetDrawable
*/

/**************************** display methods ************************************/
display* display::current = 0;

display::display()
 : window_visible(false),
   window(0),
   gl_context(0),
   buttonState(0),
   buttonsChanged(0),
   keyModState(0),
   mouseLocked(false)
{
	VPYTHON_NOTE( "Initialize display.");
}

display::~display()
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
	SetPortWindowPort(window);
	aglSetCurrentContext(gl_context);
	delete_pending_lists();
	current = this;
}

void
display::gl_end()
{
	aglSetCurrentContext(NULL);
	current = 0;
}

void
display::gl_swap_buffers()
{
	if (!window_visible) return;

	gl_begin();
	aglSwapBuffers(gl_context);
	glFinish();	// Ensure rendering completes here (without the GIL) rather than at the next paint (with the GIL)
	gl_end();
}

void
display::create()
{
	VPYTHON_NOTE( "Start create.");
	python::gil_lock gil;  // protect statics like widgets, the shared display list context
	widgets.insert(this);
	// TODO: Just to get going, set flags to 0:
	if (!initWindow(title, window_x, window_y, window_width, window_height, 0)) {
		std::ostringstream msg;
		msg << "Could not create the window!\n";
		VPYTHON_CRITICAL_ERROR( msg.str());
		std::exit(1);
	}
	VPYTHON_NOTE( "End create.");
}

/*
// Original comment:
 	// Do not delete contexts directly, as there can be
	// nasty interactions with the event loop thread.
	// Use the destroy_context function instead.
void destroy_context (glContext * cx)
{
	(static_cast<aglContext *>(cx))->cleanup();
}
*/

void
display::destroy() // was aglContext::cleanup()
{
	VPYTHON_NOTE( "Start destroy()");
	window_x	= -1;
	window_y	= -1;
	window_width	= 0;
	window_height	= 0;
	if (gl_context)
		aglDestroyContext(gl_context);
	gl_context = NULL;
	if (window)
		DisposeWindow(window);
	window = NULL;
	python::gil_lock gil;
	widgets.erase(this);
	VPYTHON_NOTE( "End destroy()");
}

void
display::activate(bool active) {
	VPYTHON_NOTE( "Start activate.");
	if (active) {
		VPYTHON_NOTE( "Opening a window from Python.");
		//SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL );
		gui_main::call_in_gui_thread( boost::bind( &display::create, this ) );
	} else {
		VPYTHON_NOTE( "Closing a window from Python.");
		gui_main::call_in_gui_thread( boost::bind( &display::destroy, this ) );
	}
	VPYTHON_NOTE( "End activate.");
}

display::EXTENSION_FUNCTION
display::getProcAddress(const char* name) {
	// TODO: What instead of wglGetProcAddress?
	//return (EXTENSION_FUNCTION)::wglGetProcAddress( name );
}

void
display::makeCurrent()
{
	SetPortWindowPort(window);
	aglSetCurrentContext(gl_context);
	delete_pending_lists();
}

void
display::makeNotCurrent()
{
	aglSetCurrentContext(NULL);
}

void
display::add_pending_glDeleteList(int base, int howmany)
{
	// TODO: Cache::write_lock L(list_lock);
	pending_glDeleteLists.push_back( std::make_pair(base, howmany));	
}

void 
display::delete_pending_lists()
{
	// TODO: Cache::write_lock L( list_lock);
	for (std::vector<std::pair<int, int> >::iterator i = pending_glDeleteLists.begin();
		i != pending_glDeleteLists.end(); ++i) {
		glDeleteLists(i->first, i->second);	
	}
	pending_glDeleteLists.clear();
}

int
display::getShiftKey()
{
	return modBit(keyModState, shiftKeyBit);
}

int
display::getAltKey()
{
	return modBit(keyModState, optionKeyBit);
}

int
display::getCtrlKey()
{
	// For Mac, treat command key as control also
	return modBit(keyModState, cmdKeyBit) || modBit(keyModState, controlKeyBit);
}

void
display::update_size()
{
	Rect	drawing, bounds; // region to draw in, bounds of window
	
	GetWindowBounds(window, kWindowContentRgn, &drawing);
	GetWindowBounds(window, kWindowStructureRgn, &bounds);
	report_resize(	drawing.left, drawing.top, drawing.right-drawing.left, drawing.bottom-drawing.top, 
			bounds.left, bounds.top, bounds.right-bounds.left, bounds.bottom-bounds.top );
}

OSStatus
display::vpWindowHandler (EventHandlerCallRef target, EventRef event)
{
	UInt32	kind;
	
	kind = GetEventKind(event);
	if (kind == kEventWindowBoundsChanged) {
		update_size();
		// Tell OpenGL about it
		aglUpdateContext(gl_context);
	} else if (kind == kEventWindowClosed) {
		VPYTHON_NOTE( "kEventWindowClosed");
		QuitApplicationEventLoop();
		return noErr;
		// Safest way to destroy a window is by generating ESC key event,
		// which will be passed on and interpreted as close request
		//keys.push("escape");
		/*
		destroy(); // kill the window
		if (exit) {
			QuitApplicationEventLoop(); // by default, kill the application
		}
		return noErr;
		*/
	}
	return eventNotHandledErr;
}

OSStatus
display::vpKeyboardHandler (EventHandlerCallRef target, EventRef event)
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
display::vpMouseHandler (EventHandlerCallRef target, EventRef event)
{
	WindowPartCode	part;
	WindowRef		win;
	UInt32			kind;
	Point			pt;
	EventMouseButton btn;
	bool buttons[3]; // left, right, middle buttons
	bool shiftState[4]; // shift, ctrl, option, command key down
			
	// First must check if mouse event occurred within our content area
	GetEventParameter(event, kEventParamMouseLocation,
					  typeQDPoint, NULL,
					  sizeof(pt), NULL,
					  &pt);
	part = FindWindow(pt, &win);
	if (win != window || part != inContent) {
		// Title bar, close box, ... pass to OS
		return eventNotHandledErr;
	}
	
	// Get window-relative position and any modifier keys
	GetEventParameter(event, kEventParamWindowMouseLocation,
					  typeQDPoint, NULL,
					  sizeof(pt), NULL,
					  &pt);	
	GetEventParameter(event, kEventParamMouseButton,
					  typeMouseButton, NULL,
					  sizeof(btn), NULL,
					  &btn);
	GetEventParameter(event, kEventParamKeyModifiers,
					  typeUInt32, NULL,
					  sizeof(keyModState), NULL,
					  &keyModState);
	
	shiftState[0] = modBit(keyModState, shiftKeyBit);
	shiftState[1] = modBit(keyModState, controlKeyBit);
	shiftState[2] = modBit(keyModState, optionKeyBit);
	shiftState[3] = modBit(keyModState, cmdKeyBit);

	// mouse_manager expects a release event to be reported with no button as true,
	// which is the case for Windows, but not for the Mac:
	kind = GetEventKind(event);
	buttons[0] = (btn == kEventMouseButtonPrimary && kind != kEventMouseUp);
	buttons[1] = (btn == kEventMouseButtonSecondary && kind != kEventMouseUp); // right button on 3-button mouse
	buttons[2] = (btn == kEventMouseButtonTertiary && kind != kEventMouseUp); // middle button on 3-button mouse
	
	//std::cout << "buttons=(" << buttons[0] << "," << buttons[1] << "," << buttons[2] << ")" << std::endl;

	if (buttons[1] || buttons[2]) 
		mouse.report_mouse_state( 3, buttons, pt.h, pt.v, 4, shiftState, false ); // TODO: can we lock mouse?
	else
		mouse.report_mouse_state( 1, buttons, pt.h, pt.v, 4, shiftState, false );
	
	return noErr;
}

static OSStatus 
vpEventHandler (EventHandlerCallRef target, EventRef event, void * data)
{
	UInt32	evtClass;
	display * thiswindow;
	
	thiswindow = (display *)data;
	
	evtClass = GetEventClass(event);
	
	switch (evtClass) {
		case kEventClassWindow:
			//VPYTHON_NOTE( "kEventClassWindow");
			return thiswindow->vpWindowHandler(target, event);
			break;
		case kEventClassKeyboard:
			//VPYTHON_NOTE( "kEventClassKeyboard");
			return thiswindow->vpKeyboardHandler(target, event);
			break;
		case kEventClassMouse:
			//VPYTHON_NOTE( "kEventClassMouse");
			return thiswindow->vpMouseHandler(target, event);
			break;
		default:
			break;
	}
	// Default is let OS do it
	return eventNotHandledErr;
}

bool
display::changeWindow(std::string title, int x, int y, int width, int height, int flags)
{
	/*
	if (title) {
		SetWindowTitleWithCFString(window, CFStringCreateWithCString(NULL, title, kCFStringEncodingASCII));
	}
	*/
	
	if (x >= 0 && y >= 0) {
		window_x = x;
		window_y = y;
		MoveWindowStructure(window, window_x, window_y);
	}
	if (width > 0 && height > 0) {
		window_width  = width;
		window_height = height;
		SizeWindow(window, window_width, window_height, true);
	}
	return true;
}

bool
display::initWindow(std::string title, int x, int y, int width, int height, int flags)
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
	
	VPYTHON_NOTE( "Start initWindow.");
	// Don't let window hide itself under the menu bar. Margin should be more than
	// the menu bar itself because sometimes the position gets set first and then
	// the title bar extended above that point.
	minY = GetMBarHeight() * 2;
	if (y < minY && ! (flags & display::FULLSCREEN))
		y = minY;
	// Window 
	SetRect(&bounds, x, y, x + width, y + height);
	err = CreateNewWindow(kDocumentWindowClass, kWindowStandardDocumentAttributes, &bounds, &window);
	if (err != noErr)
		return false;
	this->changeWindow(title, x, y, width, height, flags);
	VPYTHON_NOTE( "After CreateNewWindow.");
	
	// GL context
	dev = GetMainDevice();
	idx = 0;
	while (attrList[idx] != AGL_NONE) idx ++;
	// Special
	if (flags & display::FULLSCREEN) {
		attrList[idx] =	AGL_FULLSCREEN;
		idx ++;
	}
	if (flags & display::QB_STEREO) {
		attrList[idx] = AGL_STEREO;
		idx ++;
	}
	fmt = aglChoosePixelFormat(&dev, 1, (const GLint *)attrList);
	if ((fmt == NULL || aglGetError() != AGL_NO_ERROR) && (flags & display::QB_STEREO)) {
		// Try without stereo
		idx --;
		attrList[idx] = AGL_NONE;
		fmt = aglChoosePixelFormat(&dev, 1, (const GLint *)attrList);
	}

	if (fmt == NULL || aglGetError() != AGL_NO_ERROR)
		return false;
	gl_context = aglCreateContext(fmt, NULL);
	if (gl_context == NULL)
		return false;
	
	// Fullscreen on Mac
	if (flags & display::FULLSCREEN) {
		aglEnable(gl_context, AGL_FS_CAPTURE_SINGLE);
		aglSetFullScreen(gl_context, 0, 0, 0, 0);
	} else {
		aglSetDrawable(gl_context, GetWindowPort(window));
	}
	aglDestroyPixelFormat(fmt);
	
	VPYTHON_NOTE( "Next, set up event handling in initWindow.");
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
	DisposeEventHandlerUPP(upp);
	// Make visible
	if (! (flags & display::FULLSCREEN)) {
		ActivateWindow(window, true);
		ShowWindow(window);
	}
	window_visible = true; // TODO: do we need to wait for some event?
	update_size();
	
	VPYTHON_NOTE( "End initWindow.");
	return true;
}

bool
display::isOpen()
{
	return window != NULL;
}

void
display::lockMouse()
{
}

void
display::unlockMouse()
{
}

void
display::showMouse()
{
	ShowCursor();
}

void
display::hideMouse()
{
	HideCursor();
}

vector
display::getMousePos()
{
	if (!window)
		return vector(0,0,0);
	
	vector tmp = mousePos;
	tmp.x /= window_width;
	tmp.y /= window_height;
	
	return tmp;
}

vector
display::getMouseDelta()
{
	// GL units (% of window)
	vector tmp = mousePos - oldMousePos;
	oldMousePos = mousePos;
	
	return tmp;
}

int
display::getMouseButtons()
{
	return buttonState;
}

int
display::getMouseButtonsChanged()
{
	int c = buttonsChanged;
	buttonsChanged = 0;
	return c; 
}

std::string
display::getKeys()
{
	if (!keys.empty()) {
		std::string s = keys.front();
		keys.pop();
		return s;
	} 
	return std::string("");
}

/******************** gui_main implementation **********************/

gui_main* gui_main::self = 0;  // Protected by python GIL

boost::signal<void()> gui_main::on_shutdown;

gui_main::gui_main()
// : gui_thread(-1)
{
}

void
gui_main::event_loop ()
{
	VPYTHON_NOTE( "Start event_loop.");
	poll();
	RunApplicationEventLoop();
	VPYTHON_NOTE( "End event_loop.");
	ExitToShell();
}

void
gui_main::init_thread()
{
	if (!self) {
		init_platform();
		VPYTHON_NOTE( "begin init_thread");
		// We are holding the Python GIL through this process, including the wait!
		// We can't let go because a different Python thread could come along and mess us up (e.g.
		//   think that we are initialized and go on to call PostThreadMessage without a valid idThread)
		self = new gui_main;		
		thread gui( boost::bind( &gui_main::event_loop, self ) );
		lock L( self->init_lock );
		while (self->gui_thread == -1)
			self->initialized.wait( L );
		VPYTHON_NOTE( "end init_thread");
	}
}

static void call_boost_function( EventLoopTimerRef, void* f_as_pvoid ) {
	boost::function<void()>* f = reinterpret_cast<boost::function<void()>*>(f_as_pvoid);
	(*f)();
	delete f;
}

static void call_in_gui_thread_delayed( double delay, const boost::function< void() >& f) {
	EventLoopTimerRef discard;
	EventLoopTimerUPP upp = NewEventLoopTimerUPP(call_boost_function);
	InstallEventLoopTimer(GetMainEventLoop(), delay, 0, upp, new boost::function<void()>(f), &discard);
	DisposeEventLoopTimerUPP(upp);
}

void
gui_main::call_in_gui_thread( const boost::function< void() >& f )
{
	init_thread();
    call_in_gui_thread_delayed( 0.0, f );
}

void gui_main::poll() {
	// Called in gui thread when it's time to render
	// We don't need the lock here, because displays can't be created or destroyed from Python
	// without a message being processed by the GUI thread.  paint_displays() will pick
	// the lock up as necessary to synchronize access to the actual display contents.

	std::vector<display*> displays( widgets.begin(), widgets.end() );

	double interval = render_manager::paint_displays( displays );

	//std::cout << "poll " << widgets.size() << " " << interval << std::endl;
	call_in_gui_thread_delayed( interval, boost::bind(&gui_main::poll, this) );
}

} // !namespace cvisual