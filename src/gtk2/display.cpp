#include "gtk2/display.hpp"
#include "vpython-config.h"
#include "util/errors.hpp"
#include "python/gil.hpp"
#include "util/gl_free.hpp"
#include "font_renderer.hpp"

#include <boost/thread/thread.hpp>
#include <boost/lexical_cast.hpp>
using boost::lexical_cast;

#include <gtkmm/gl/init.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/stock.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/main.h>
#include <gdk/gdkkeysyms.h>

#include <gdkmm/gl/query.h>

#include <gtkmm/toggletoolbutton.h>
#include <gtkmm/radiotoolbutton.h>

#include <cstdlib>
#include <sstream>
#include <iostream>

namespace cvisual {
using boost::thread;

#if (defined(_WIN32) || defined(_MSC_VER))
	const int border_width = 8;
	const int border_height = 3;
#else
	// Ubuntu Linux; unknown what situation is on Mac
	const int border_width = 9;
	const int border_height = 6;
#endif

namespace {
Glib::ustring dataroot = "";
}

void
display::set_dataroot( const std::wstring& _dataroot)
{
	dataroot = w2u( _dataroot );
}

display::display()
{
}

display::~display()
{	
}

void
display::activate( bool active )
{
	if (active) {
		VPYTHON_NOTE( "Opening a window from Python.");
		gui_main::add_display( this);
	} else {
		VPYTHON_NOTE( "Closing a window from Python.");
		gui_main::remove_display(this);
	}
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

namespace {

inline void 
widget_fail( const Glib::ustring& name)
{
	std::ostringstream msg;
	msg << "Getting widget named: " << name << " failed!\n";
	VPYTHON_CRITICAL_ERROR( msg.str());
	std::exit(1);
}

template <typename GtkWidget>
inline GtkWidget*
get_widget( Glib::RefPtr<Gnome::Glade::Xml> tree, const Glib::ustring& name)
{
	GtkWidget* result = dynamic_cast<GtkWidget*>( tree->get_widget( name));
    if (!result) {
    	widget_fail( name);
    }
    return result;
}
} // !namespace (anon)

void
display::create()
{
	area.reset( new render_surface(*this, mouse, stereo_mode == ACTIVE_STEREO));
	
	// window_width and window_height give the size of the window (including borders and such)
	// determine the size of the render_surface
	// xxx Some way to use less magic constants here?
	int w = window_width, h = window_height;
	w -= border_width; h -= border_height;
	h -= get_titlebar_height();
	if (show_toolbar) h -= get_toolbar_height();
	if (stereo_mode == PASSIVE_STEREO || stereo_mode == CROSSEYED_STEREO)
		w *= 2;
	
	area->set_size_request( w, h );
	area->signal_key_press_event().connect(
		sigc::mem_fun( *this, &display::on_key_pressed));
	
	// Glade-based UI.
	if (show_toolbar) {
		glade_file = Gnome::Glade::Xml::create( dataroot + "vpython.glade");
		get_widget<Gtk::ToolButton>(glade_file, "quit_button")->signal_clicked().connect(
			sigc::mem_fun( *this, &display::on_quit_clicked));
		get_widget<Gtk::ToolButton>(glade_file, "fullscreen_button")->signal_clicked().connect(
			sigc::mem_fun( *this, &display::on_fullscreen_clicked));
		get_widget<Gtk::ToolButton>(glade_file, "rotate_zoom_button")->signal_clicked().connect(
			sigc::mem_fun( *this, &display::on_rotate_clicked));
		get_widget<Gtk::ToolButton>(glade_file, "pan_button")->signal_clicked().connect(
			sigc::mem_fun( *this, &display::on_pan_clicked));
		get_widget<Gtk::ToolButton>(glade_file, "zoom_to_fit_button")->signal_clicked().connect(
			sigc::mem_fun( *this, &display::on_zoom_clicked));
		window = get_widget<Gtk::Window>(glade_file, "window1");
		get_widget<Gtk::VBox>( glade_file, "vbox1")->pack_start( *area);
	}
	else { // this does not work; it crashes:
		glade_file = Gnome::Glade::Xml::create( dataroot + "vpython_notoolbar.glade");
		window = get_widget<Gtk::Window>(glade_file, "window1");
		get_widget<Gtk::VBox>( glade_file, "vbox1")->pack_start( *area);
	}

	window->set_title( title);
	
	window->signal_delete_event().connect( sigc::mem_fun( *this, &display::on_window_delete));
	
	//window->set_position(Gtk::WIN_POS_NONE); // seems not needed
	
	// It would make more sense to show_all after move, to avoid possible
	// flashing of the window from an initial position to its final
	// position. But at least on Windows this causes problems, at least
	// with multiple physical displays (extended desktop).
	window->show_all();
	if (window_x >= 0 && window_y >= 0) { // if user specified x or y
		window->move( (int)window_x, (int)window_y);
	}
	if (fullscreen)
		window->fullscreen();
	area->grab_focus();
	assert( area->can_focus());
	while (Gtk::Main::events_pending())
		Gtk::Main::iteration();
}

void
display::destroy()
{
	timer.disconnect();
	window->hide();
	window = 0;
	area.reset();
	glade_file.clear();
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
display::on_zoom_clicked()
{
	set_center(vector(0, 0, 0));
	set_forward(vector(0, 0, -1));
	set_up(vector(0, 1, 0));
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
	window = NULL;
	area.reset();
	glade_file.clear();

	gui_main::report_window_delete( this);
	if (exit) {
		VPYTHON_NOTE( "Initiating shutdown from window closure");
		if (area) {
			gl_free();
		}
		gui_main::quit();
	}
		
	return true;
}

void
display::on_quit_clicked()
{
	VPYTHON_NOTE( "Initiating shutdown from the GUI.");
	if (area)
		gl_free();
	gui_main::quit();
}

bool
display::on_key_pressed( GdkEventKey* key)
{
	// Note that this algorithm will proably fail if the user is using anything 
	// other than a US keyboard.
	guint k = key->keyval;
	std::string ctrl_str;
	// First trap for shift, ctrl, and alt.
	if (key->state & GDK_CONTROL_MASK) {
		ctrl_str += "ctrl+";
	}
	if (key->state & GDK_MOD1_MASK) {
		ctrl_str += "alt+";
	}
	if ((key->state & GDK_SHIFT_MASK) || (key->state & GDK_LOCK_MASK)) {
		if ( !isprint(k) )
			ctrl_str += "shift+";
	}
	
	// Specials, try to match those in wgl.cpp
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
	else if ( isprint(k) && !ctrl_str.empty() ) {
		// A control character
		ctrl_str += static_cast<char>( k);
		keys.push(ctrl_str);
	}
	else if ( isprint(k) ) {
		// Anything else.
		keys.push( std::string( key->string));
	}
	
	return true;
}

display::EXTENSION_FUNCTION
display::getProcAddress(const char* name) {
	return (EXTENSION_FUNCTION)Gdk::GL::get_proc_address( name );
}

////////////////////////////////// gui_main implementation ////////////////////
gui_main* gui_main::self = 0;
mutex* gui_main::init_lock = 0;
condition* gui_main::init_signal = 0;


gui_main::gui_main()
	: kit( 0, 0), caller( 0), returned( false), 
	thread_exited(false), shutting_down( false)
{
	Gtk::GL::init( 0, 0);
	if (!Glib::thread_supported())
		Glib::thread_init();
	signal_add_display.connect( sigc::mem_fun( *this, &gui_main::add_display_impl));
	signal_remove_display.connect( sigc::mem_fun( *this, &gui_main::remove_display_impl));
	signal_shutdown.connect( sigc::mem_fun( *this, &gui_main::shutdown_impl));
}

void
gui_main::run()
{
	kit.run();
	lock L(call_lock);
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
	self->kit.quit();
}

boost::signal<void()> gui_main::on_shutdown;

} // !namespace cvisual
