#include "gtk2/display.hpp"
#include "vpython-config.h"
#include "util/errors.hpp"

#include <boost/thread/thread.hpp>
#include <gtkmm/gl/init.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/stock.h>
#include <gtkmm/radiobutton.h>


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
	
// A singlton that represents the background thread.

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
	title( "VPython")
{
	display_kernel::gl_begin.connect( 
		SigC::slot( *this, &display::gl_begin));
	display_kernel::gl_end.connect( 
		SigC::slot( *this, &display::gl_end));
	display_kernel::gl_swap_buffers.connect( 
		SigC::slot( *this, &display::gl_swap_buffers));
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
		gtk2_main::add_display( this);
	}
	else if (!vis && active) {
		gtk2_main::remove_display(this);
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
		gtk2_main::add_display(this);
	}
}

void
display::add_renderable_screen( shared_ptr<renderable> obj)
{
	display_kernel::add_renderable( obj);
	if (!active && visible)
		gtk2_main::add_display(this);
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

Glib::RefPtr<Gdk::GL::Context> display::share_list;

void
display::create()
{
	Glib::RefPtr<Gdk::GL::Config> config;
	// Create the DrawingArea widget first.
	switch (stereo_mode) {
		case ACTIVE_STEREO:
			config = Gdk::GL::Config::create( Gdk::GL::MODE_RGB
				| Gdk::GL::MODE_DOUBLE
				| Gdk::GL::MODE_STEREO
				| Gdk::GL::MODE_DEPTH );
			if (config)
				break;
			VPYTHON_WARNING( "'active' stereo mode not available, falling back"
				" to :'nostereo'");
		default:
			config = Gdk::GL::Config::create( Gdk::GL::MODE_RGB
				| Gdk::GL::MODE_DOUBLE
				| Gdk::GL::MODE_DEPTH );
	}
	if (!config) {
		VPYTHON_CRITICAL_ERROR( "Failed to create any OpenGL context.");
		std::exit(1);
	}
	
	if (share_list) {
		area.reset( new Gtk::GL::DrawingArea( config, share_list, true));
	}
	else {
		area.reset( new Gtk::GL::DrawingArea( config));
	}
	
	area->add_events( Gdk::EXPOSURE_MASK
		| Gdk::BUTTON_PRESS_MASK
		| Gdk::BUTTON_RELEASE_MASK
		| Gdk::POINTER_MOTION_MASK 
		| Gdk::ENTER_NOTIFY_MASK 
		| Gdk::BUTTON2_MOTION_MASK
		| Gdk::BUTTON3_MOTION_MASK);
	if (stereo_mode == PASSIVE_STEREO)
		area->set_size_request( (int)(width * 2.0), (int)height);
	else
		area->set_size_request( (int)width, (int)height);
	area->signal_realize().connect( SigC::slot( *this, &display::on_realize));
	area->signal_motion_notify_event().connect( 
		SigC::slot( *this, &display::on_motion_notify));
	area->signal_configure_event().connect( SigC::slot( *this, &display::on_configure));
	area->signal_enter_notify_event().connect( 
		SigC::slot( *this, &display::on_enter_notify));
	area->signal_expose_event().connect(
		SigC::slot( *this, &display::on_expose));
		
		
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
	active = true;
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
	timer.disconnect();
	active = false;
	window.reset();
	area.reset();
	
	gtk2_main::report_window_delete( this);
	if (exit)
		gtk2_main::quit();
	return false;
}

void
display::on_quit_clicked()
{
	gtk2_main::quit();
}

bool 
display::on_motion_notify( GdkEventMotion* event)
{
	// Direct the core to perform rendering ops.
	
	// The vertical and horizontal changes of the mouse position since the last
	// call.
	float dy = static_cast<float>( event->y) - last_mousepos_y;
	float dx = static_cast<float>( event->x) - last_mousepos_x;
	bool buttondown = false;
	
	if (event->state & GDK_BUTTON2_MASK) {
		report_mouse_motion( dx, dy, display_kernel::MIDDLE);
		buttondown = true;
	}
	if (event->state & GDK_BUTTON3_MASK) {
		report_mouse_motion( dx, dy, display_kernel::RIGHT);
		buttondown = true;
	}
	else if (!buttondown) {
		report_mouse_motion( dx, dy, display_kernel::NONE);
	}
	last_mousepos_x = static_cast<float>(event->x);
	last_mousepos_y = static_cast<float>(event->y);
	return true;
}

bool 
display::on_configure( GdkEventConfigure* event)
{
	report_resize( 
		static_cast<float>(event->width), 
		static_cast<float>(event->height));
	return true;
}

bool 
display::on_enter_notify( GdkEventCrossing* event)
{
	last_mousepos_x = event->x;
	last_mousepos_y = event->y;
	return true;
}

void
display::on_realize()
{
	report_realize();
	if (!share_list) {
		share_list = area->get_gl_context();
	}
	assert( share_list);
	
	timer = Glib::signal_timeout().connect( 
		SigC::slot( *this, &display::render_scene),
		28);
}

bool
display::on_expose( GdkEventExpose*)
{
	render_scene();
	return true;
}

void
display::gl_begin()
{
	// TODO: Use the mouse motion signal hinting to poll its state at the end
	// of a render cycle and pass a single mouse_motion event for it.
	assert( area->get_gl_window()->gl_begin(area->get_gl_context()));
}

void
display::gl_end()
{
	area->get_gl_window()->gl_end();
}

void
display::gl_swap_buffers()
{
	area->get_gl_window()->swap_buffers();
}

////////////////////////////////// gtk2_main implementation ////////////////////
gtk2_main* gtk2_main::self = 0;


gtk2_main::gtk2_main()
	: kit( 0, 0), caller( 0), returned( false), waiting_allclosed(false)
{
	Gtk::GL::init( 0, 0);
	if (!Glib::thread_supported())
		Glib::thread_init();
	signal_add_display.connect( SigC::slot( *this, &gtk2_main::add_display_impl));
	signal_remove_display.connect( SigC::slot( *this, &gtk2_main::remove_display_impl));
	signal_shutdown.connect( SigC::slot( *this, &gtk2_main::shutdown_impl));
}

#if 0
static void
py_quit( void*)
{
	std::exit(0);
}
#endif

void
gtk2_main::run()
{
	kit.run();
	lock L(call_lock);
	if (waiting_allclosed) {
		returned = true;
		self = 0;
		call_complete.notify_all();
	}
#if 0
	else {
		Py_AddPendingCall( &py_quit);
	}
#endif
}

void 
gtk2_main::thread_proc(void)
{
	self = new gtk2_main();
	self->run();
	self = 0;
}

void
gtk2_main::init_thread(void)
{
	if (!self) {
		thread gtk( &gtk2_main::thread_proc);
		while (!self) {
		}
	}
}

// TODO: This isn't safe.
bool
gtk2_main::allclosed()
{
	if (!self)
		return true;
	lock L(self->call_lock);
	return self->displays.empty();
}

void
gtk2_main::waitclosed()
{
	if (!self)
		return;
	lock L(self->call_lock);
	self->waiting_allclosed = true;
	self->returned = false;
	while (self) {
		self->call_complete.wait(L);
	}
}

void
gtk2_main::add_display( display* d)
{
	init_thread();
	lock L(self->call_lock);
	self->caller = d;
	self->returned = false;
	self->signal_add_display();
	while (!self->returned)
		self->call_complete.wait(L);
	self->caller = 0;
}

void
gtk2_main::add_display_impl()
{
	lock L(call_lock);
	caller->create();
	displays.push_back(caller);
	returned = true;
	call_complete.notify_all();
}

void
gtk2_main::remove_display( display* d)
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
gtk2_main::remove_display_impl()
{
	lock L(call_lock);
	caller->destroy();
	displays.remove( caller);
	returned = true;
	call_complete.notify_all();
}

void
gtk2_main::shutdown()
{
	if (!self)
		return;
	lock L(self->call_lock);
	self->returned = false;
	self->signal_shutdown();
	while (!self->returned)
		self->call_complete.wait(L);
}

void
gtk2_main::shutdown_impl()
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
gtk2_main::report_window_delete( display* window)
{
	assert( self != 0);
	bool display_empty = false;
	{
		lock L(self->call_lock);
		self->displays.remove( window);
		display_empty = self->displays.empty();
	}
	if (display_empty && self->waiting_allclosed)
		gtk2_main::quit();
}

void
gtk2_main::quit()
{
	assert( self != 0);
	for (std::list<display*>::iterator i = self->displays.begin(); 
			i != self->displays.end(); ++i) {
		(*i)->destroy();
	}
	self->kit.quit();
}

} // !namespace cvisual
