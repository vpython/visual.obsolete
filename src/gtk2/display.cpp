#include "gtk2/display.hpp"
#include "vpython-config.h"
#include "util/errors.hpp"

#include <boost/thread/thread.hpp>
#include <gtkmm/gl/init.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/stock.h>
#include <gtkmm/radiobutton.h>
#include <gdk/gdkkeysyms.h>


namespace cvisual {
using boost::thread;
	
/* Startup:  When the first display is created, from Python, set_selected is
	called with itself as the argument.
	Various attributes of the display object itself are set by the user from
	Python.
	When the first renderable object is made visible, it calls 
	add_renderable(self) for the display that owns it.  If the global main object
	does not exist, it is created.  This initializes a Gtk::Main object and enters
	the event loop.
	Next, the display calls main->add_display(self), This places the pointer to
	the display in static memory and invokes a asynchronous callback function through a 
	Glib::Dispatcher.  The Python thread then blocks with a condition variable
	on the state of the 'active' data member.
	The Gtk thread recieves the callback function and forwards it to the display
	object that requested it.  The display creates the required Gtk resources,
	creates the display, makes it visible, adds a timer, sets 'active' to true, 
	and signals	the condition.
	The Python thread is awakened upon signaling of the condition, and returns
	control to the user.
	The Gtk thread returns to the event loop.
*/

/* Shutdown:  This may be invoked from three paths.
	
	First: The Python program's simulation loop is closed, and the program
	reaches the end.  Python calls the atexit() handler cvisual.waitclosed(),
	and blocks after calling Py_BEGIN_ALLOW_THREADS().
	At this point, all of the active displays continue to accept user input
	until closed by the user.  When the first window is closed whose exit_on_close
	variable is true, all of the windows are called with hide_all and deleted.
	kit.quit() is called which return control to the thread function which signals
	waitclosed() before exiting the thread.
	
	Second.  The Python loop may explicitly exit VPython by calling shutdown().
	In this case, the same shutdown procedure is invoked as before, but the thread
	exits without signaling waitclosed().
	
	Third:  The Python program's simulation loop is infinate.  In this case the
	program runs until a display is closed whose exit_on_close variable is true,
	which initiates the shutdown as before.  In this case, Py_AddPendingCall is 
	called with exit() as the required func.
*/

/* Visibility:  When a display is explicity made visible, than startup commences
 * as normal without any objects within it.  When an object is explicitly made
 * invisible, the owning window is hidden and destoyed.  It is recreated as needed.
 */
	

shared_ptr<display> display::selected;

display::display()
	: last_mousepos_x(0.0f),
	last_mousepos_y(0.0f),
	active( false),
	x( -1),
	y(-1),
	width( 384),
	height( 256),
	exit(false),
	visible(true),
	fullscreen(false),
	title( "VPython")
{
}

display::~display()
{	
}


void 
display::set_x( float _x)
{
	if (active)
		throw std::invalid_argument( "Cannot move the window once it is active.");
	x = _x;
}

float
display::get_x()
{
	lock L(mtx);
	return x;
}

void 
display::set_y( float _y)
{
	if (active)
		throw std::invalid_argument( "Cannot move the window once it is active.");
	y = _y;
}

float
display::get_y()
{
	lock L(mtx);
	return y;
}

void 
display::set_width( float _width)
{
	if (active)
		throw std::invalid_argument( "Cannot move the window once it is active.");
	width = _width;
}

float
display::get_width()
{
	lock L(mtx);
	return width;
}
void 
display::set_height( float _height)
{
	if (active)
		throw std::invalid_argument( "Cannot move the window once it is active.");
	height = _height;
}

float
display::get_height()
{
	lock L(mtx);
	return height;
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
display::set_title( std::string _title)
{
	if (active)
		throw std::runtime_error( "Cannot change the window's title after it is active.");
	title = _title;
}

std::string
display::get_title()
{
	return title;
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

bool
display::is_fullscreen()
{
	return fullscreen;
}

void
display::set_fullscreen( bool fs)
{
	if (active)
		throw std::runtime_error( 
			"Cannot change the window's state after initialization.");
	fullscreen = fs;
}

void
display::create()
{
	area.reset( new render_surface(*this, stereo_mode == ACTIVE_STEREO));
	
	if (stereo_mode == PASSIVE_STEREO)
		area->set_size_request( (int)(width * 2.0), (int)height);
	else
		area->set_size_request( (int)width, (int)height);
	area->signal_key_press_event().connect(
		SigC::slot( *this, &display::on_key_pressed));
	
	Glib::RefPtr<Gdk::Pixbuf> fs_img = Gdk::Pixbuf::create_from_file( 
		VPYTHON_PREFIX "/data/galeon-fullscreen.png");
	
	using namespace Gtk::Toolbar_Helpers;
	Gtk::Toolbar* tb = Gtk::manage( new Gtk::Toolbar());
	tb->tools().push_back( StockElem( 
		Gtk::Stock::QUIT,  
		SigC::slot( *this, &display::on_quit_clicked),
		"Exit VPython"));
	// Fullscreen toggle button.
	tb->tools().push_back( ToggleElem( 
		"Fullscreen",
		*Gtk::manage( new Gtk::Image(fs_img)),
		SigC::slot( *this, &display::on_fullscreen_clicked),
		"Toggle fullscreen on/off"));
	// The mouse control button group.
	Gtk::RadioButton::Group mouse_ctl;
	tb->tools().push_back( Space());
	tb->tools().push_back( RadioElem(
		mouse_ctl,
		"Rotate/Zoom",
		SigC::slot( *this, &display::on_rotate_clicked)));
	tb->tools().push_back( RadioElem(
		mouse_ctl,
		"Pan",
		SigC::slot( *this, &display::on_pan_clicked)));
	
	Gtk::VBox* frame = Gtk::manage( new Gtk::VBox());
	frame->pack_start( *tb, Gtk::PACK_SHRINK, 0);
	frame->pack_start( *area);
	
	window.reset( new Gtk::Window());
	window->set_title( title);
	window->set_icon_from_file(	VPYTHON_PREFIX "/data/logo_t.gif");
	window->add( *frame);
	window->signal_delete_event().connect( SigC::slot( *this, &display::on_window_delete));
	window->show_all();
	if (x > 0 && y > 0)
		window->move( (int)x, (int)y);
	if (fullscreen)
		window->fullscreen();
	active = true;
	area->grab_focus();
	assert( area->can_focus());
}

void
display::destroy()
{
	timer.disconnect();
	window->hide();
	active = false;
	window.reset();
	area.reset();
}

atomic_queue<std::string>& 
display::get_kb()
{
#if 0
	// TODO: Figure out exactly how this should work...
	if (!active && visible)
		gui_main::add_display(this);
#endif
	return keys;
}

void
display::on_fullscreen_clicked()
{
	static bool fullscreen = false;
	if (fullscreen) {
		window->unfullscreen();
		fullscreen = false;
	}
	else {
		window->fullscreen();
		fullscreen = true;
	}
}

void
display::on_pan_clicked()
{
	mouse_mode = PAN;
}

void
display::on_rotate_clicked()
{
	mouse_mode = ZOOM_ROTATE;
}

bool
display::on_window_delete(GdkEventAny*)
{
	VPYTHON_NOTE( "Closing a window from the GUI.");
	timer.disconnect();
	active = false;
	window.reset();
	area.reset();
	
	gui_main::report_window_delete( this);
	if (exit)
		gui_main::quit();
	return true;
}

void
display::on_quit_clicked()
{
	VPYTHON_NOTE( "Initiating shutdown from the GUI.");
	gui_main::quit();
}

bool
display::on_key_pressed( GdkEventKey* key)
{
	// Note that this algorithm will proably fail if the user is using anything 
	// other than a US keyboard.
	std::string ctrl_str;
	// First trap for shift, ctrl, and alt.
	if ((key->state & GDK_SHIFT_MASK) || (key->state & GDK_LOCK_MASK)) {
		ctrl_str += "shift+";
	}
	else if (key->state & GDK_CONTROL_MASK) {
		ctrl_str += "ctrl+";
	}
	else if (key->state & GDK_MOD1_MASK) {
		ctrl_str += "alt+";
	}
	
	// Specials, try to match those in wgl.cpp
	guint k = key->keyval;
	std::string key_str;
	switch (k) {
		case GDK_F1:
		case GDK_F2:
		case GDK_F3:
		case GDK_F4:
		case GDK_F5:
		case GDK_F6:
		case GDK_F7:
		case GDK_F8:
		case GDK_F9:
		case GDK_F10:
		case GDK_F11:
		case GDK_F12: {
			// Use braces to destroy s.
			std::ostringstream s;
			s << key_str << 'f' << k-GDK_F1 + 1;
			key_str = s.str();
		}   break;
		case GDK_Page_Up:
			key_str += "page up";
			break;
		case GDK_Page_Down:
			key_str += "page down";
			break;
		case GDK_End:
			key_str += "end";
			break;
		case GDK_Home:
			key_str += "home";
			break;
		case GDK_Left:
			key_str += "left";
			break;
		case GDK_Up:
			key_str += "up";
			break;
		case GDK_Right:
			key_str += "right";
			break;
		case GDK_Down:
			key_str += "down";
			break;	
		case GDK_Print:
			key_str += "print screen";
			break;
		case GDK_Insert:
			key_str += "insert";
			break;
		case GDK_Delete:
			key_str += "delete";
			break;
		case GDK_Num_Lock:
			key_str += "numlock";
			break;
		case GDK_Scroll_Lock:
			key_str += "scrlock";
			break;
		case GDK_BackSpace:
			key_str += "backspace";
			break;
		case GDK_Tab:
			key_str += "\t";
			break;
		case GDK_Return:
			key_str += "\n";
			break;
		case GDK_Escape:
			// Allow the user to delete a fullscreen window this way
			destroy();
			gui_main::report_window_delete(this);
			if (exit)
				gui_main::quit();
			return false;
	}
  
	if (!key_str.empty()) {
		// A special key.
		ctrl_str += key_str;
		keys.push( ctrl_str);
	}
	else if ( isprint(k) && !ctrl_str.empty()) {
		// A control character
		ctrl_str += static_cast<char>( k);
		keys.push(ctrl_str);
	}
	else if ( strlen(key->string) && isprint( key->string[0])) {
		// Anything else.
		keys.push( std::string( key->string));
	}
	
	return true;
}


////////////////////////////////// gui_main implementation ////////////////////
gui_main* gui_main::self = 0;


gui_main::gui_main()
	: kit( 0, 0), caller( 0), returned( false), waiting_allclosed(false), thread_exited(false)
{
	Gtk::GL::init( 0, 0);
	if (!Glib::thread_supported())
		Glib::thread_init();
	signal_add_display.connect( SigC::slot( *this, &gui_main::add_display_impl));
	signal_remove_display.connect( SigC::slot( *this, &gui_main::remove_display_impl));
	signal_shutdown.connect( SigC::slot( *this, &gui_main::shutdown_impl));
}



void
gui_main::run()
{
	kit.run();
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
	self = new gui_main();
	self->run();
	VPYTHON_NOTE( "Terminating GUI thread.");
	gui_main::on_shutdown();
}

void
gui_main::init_thread(void)
{
	if (!self) {
		VPYTHON_NOTE( "Starting GUI thread.");
		thread gtk( &gui_main::thread_proc);
		while (!self) {
		}
	}
}

// TODO: This may not be safe.
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
	VPYTHON_NOTE( "Adding new display object.");
	self->caller = d;
	self->returned = false;
	self->signal_add_display();
	while (!self->returned)
		self->call_complete.wait(L);
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
	lock L(self->call_lock);
	self->caller = d;
	self->returned = false;
	self->signal_remove_display();
	while (!self->returned)
		self->call_complete.wait(L);
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
		self->call_complete.wait(L);
}

void
gui_main::shutdown_impl()
{
	lock L(call_lock);
	for (std::list<display*>::iterator i = displays.begin(); i != displays.end(); ++i) {
		(*i)->destroy();
	}
	self->returned = true;
	call_complete.notify_all();
	kit.quit();
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
	for (std::list<display*>::iterator i = self->displays.begin(); 
			i != self->displays.end(); ++i) {
		(*i)->destroy();
	}
	self->displays.clear();
	self->kit.quit();
}

SigC::Signal0<void> gui_main::on_shutdown;

} // !namespace cvisual
