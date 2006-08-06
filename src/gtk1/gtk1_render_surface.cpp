#include "gtk1/render_surface.hpp"
#include <algorithm>
#include <iostream>
#include <sigc++/sigc++.h>
#include <gtk/gtk.h>

render_surface::render_surface()
{
	int attrlist[] = {
		GDK_GL_RGBA,
		GDK_GL_DOUBLEBUFFER,
		GDK_GL_DEPTH_SIZE, 12,
		GDK_GL_NONE, // May set this value to something useful farther down.
		GDK_GL_NONE
	};
	area = GTK_WIDGET( gtk_gl_area_new(attrlist));
	if (!area) {
		std::cerr << "Unable to create OpenGL display widget.\n";
		std::exit(1);
	}
	gtk_widget_set_events( GTK_WIDGET (area),
		GDK_EXPOSURE_MASK 
		| GDK_BUTTON_PRESS_MASK
		| GDK_BUTTON_RELEASE_MASK
		| GDK_POINTER_MOTION_MASK
		| GDK_ENTER_NOTIFY_MASK 
		| GDK_BUTTON2_MOTION_MASK
		| GDK_BUTTON3_MOTION_MASK);

	// Connect signal handlers to this class from the widget
	gtk_signal_connect( GTK_OBJECT(area), "configure_event",
	                    GTK_SIGNAL_FUNC(&render_surface::on_configure_event),
	                    (gpointer)this);
	gtk_signal_connect( GTK_OBJECT(area), "motion_notify_event",
	                    GTK_SIGNAL_FUNC(&render_surface::on_motion_notify_event),
	                    (gpointer)this);
	gtk_signal_connect( GTK_OBJECT(area), "enter_notify_event",
	                    GTK_SIGNAL_FUNC(&render_surface::on_enter_notify_event),
	                    (gpointer)this);
	gtk_signal_connect( GTK_OBJECT(area), "realize",
	                    GTK_SIGNAL_FUNC(&render_surface::on_realize_event),
	                    (gpointer)this);
	                    
	int width = 300;
	int height = 300;
	gtk_gl_area_size( GTK_GL_AREA(area), width, height);
	

	// Connect signal handlers from the core to this class.
	core.gl_begin.connect( SigC::slot( *this, &render_surface::gl_begin));
	core.gl_end.connect( SigC::slot( *this, &render_surface::gl_end));
	core.gl_swap_buffers.connect( 
		SigC::slot( *this, &render_surface::gl_swap_buffers));
	
}

void 
render_surface::add_renderable( shared_ptr<renderable> obj)
{
	if (obj->color.opacity == 1.0)
		core.layer_world.push_back( obj);
	else
		core.layer_world_transparent.push_back( obj);
}

void 
render_surface::add_renderable_screen( shared_ptr<renderable> obj)
{
	core.layer_screen.push_back( obj);
}
	
void 
render_surface::remove_renderable( shared_ptr<renderable> obj)
{
	// Choice of removal algorithms:  For containers that support thier own
	// removal methods (list, set), use the member function.  Else, use 
	// std::remove.
	if (obj->color.opacity != 1.0) {
		core.layer_world.remove( obj);
	}
	else
		std::remove( core.layer_world_transparent.begin(), 
			core.layer_world_transparent.end(), obj);
}
	
void 
render_surface::remove_renderable_screen( shared_ptr<renderable> obj)
{
	core.layer_screen.remove(obj);
}

void
render_surface::gl_begin()
{
	gtk_gl_area_make_current( GTK_GL_AREA (area));	
	// std::cout << "render_surface::gl_begin()\n";
}

void
render_surface::gl_end()
{
	// A no-op.
	// std::cout << "render_surface::gl_end()\n";
}

void
render_surface::gl_swap_buffers()
{
	gtk_gl_area_swapbuffers (GTK_GL_AREA (area));
	// std::cout << "render_surface::gl_swap_buffers()\n";
}

gboolean
render_surface::forward_render_scene( gpointer data)
{
	render_surface* This = (render_surface*)data;
	This->core.render_scene();
	return true;
}

bool 
render_surface::on_motion_notify_event( GtkWidget*, GdkEventMotion* event, gpointer data)
{
	render_surface* This = (render_surface*)data;
	float dy = static_cast<float>( event->y) - This->last_mousepos_y;
	float dx = static_cast<float>( event->x) - This->last_mousepos_x;
	bool buttondown = false;
	
	if (event->state & GDK_BUTTON2_MASK) {
		This->core.report_mouse_motion( dx, dy, render_core::MIDDLE);
		buttondown = true;
	}
	if (event->state & GDK_BUTTON3_MASK) {
		This->core.report_mouse_motion( dx, dy, render_core::RIGHT);
		buttondown = true;
	}
	else if (!buttondown) {
		This->core.report_mouse_motion( dx, dy, render_core::NONE);
	}
	This->last_mousepos_x = static_cast<float>(event->x);
	This->last_mousepos_y = static_cast<float>(event->y);
	return true;

}

// Called when the window is resized.
bool 
render_surface::on_configure_event( GtkWidget*, GdkEventConfigure* event, gpointer data)
{	
	render_surface* This = (render_surface*)data;
	This->core.report_resize( 
		static_cast<float>(event->width), 
		static_cast<float>(event->height));
	return true;
}

// Called when the mouse enters the scene.
bool 
render_surface::on_enter_notify_event( GtkWidget*, GdkEventCrossing* event, gpointer data)
{
	render_surface* This = (render_surface*)data;
	This->last_mousepos_x = event->x;
	This->last_mousepos_y = event->y;
	return true;
}

// Called when the window is established for the first time.
void 
render_surface::on_realize_event(GtkWidget*, gpointer data)
{
	render_surface* This = (render_surface*)data;
	This->core.report_realize();
	g_timeout_add( 33,  &render_surface::forward_render_scene, data);
}

basic_app::init::init()
{
	gtk_init(NULL, NULL);
}

basic_app::basic_app( const char* title)
{
	window = gtk_window_new( GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title( GTK_WINDOW(window), title);
	gtk_container_set_border_width( GTK_CONTAINER(window), 0);
	gtk_container_add(GTK_CONTAINER (window),  scene.area);
	gtk_signal_connect( GTK_OBJECT(window), "delete_event", 
		GTK_SIGNAL_FUNC( gtk_main_quit), NULL);
}

void
basic_app::run()
{
	gtk_widget_show_all( window);
	gtk_main();
}

