#ifndef VPTYHON_GTK1_RENDER_SURFACE_HPP
#define VPYTHON_GTK1_RENDER_SURFACE_HPP

#include "render_core.hpp"

#include <gdk/gdk.h>
#include <gtk/gtkwidget.h>
#include <gtkgl/gtkglarea.h>
#include <sigc++/object.h>

class basic_app;

class render_surface : public SigC::Object
{
 private:
	float last_mousepos_x;
	float last_mousepos_y;
	render_core core;
 
 public:
	render_surface();
	void add_renderable( shared_ptr<renderable>);
	void add_renderable_screen( shared_ptr<renderable>);
	void remove_renderable( shared_ptr<renderable>);
	void remove_renderable_screen( shared_ptr<renderable>);
 
 protected:
	// Low-level signal handlers
	// Called when the user moves the mouse across the screen.
	static bool on_motion_notify_event( GtkWidget* widget, GdkEventMotion* event, gpointer data);
	// Called when the window is resized.
	static bool on_configure_event( GtkWidget* widget, GdkEventConfigure* event, gpointer data);
	// Called when the mouse enters the scene.
	static bool on_enter_notify_event( GtkWidget* widget, GdkEventCrossing* event, gpointer data);
	// Called when the window is established for the first time.
 	static void on_realize_event(GtkWidget* widget, gpointer data);
 
 private:
	GtkWidget* area;
	// Functions to be used as callbacks for connections via SigC::slots.
	void gl_begin();
	void gl_end();
	void gl_swap_buffers();
	// Calls core.render_scene() so that core does not need to inherit from SigC
	// ::Object.
	static gboolean forward_render_scene( gpointer data);
	friend class basic_app;
};


class basic_app
{
 private:
	class init
	{
	 public:
		init();
	} i;
	GtkWidget* window;
	
 public:
	render_surface scene;
	basic_app( const char* title);
	void run();	
};

#endif // !defined VPYTHON_GTK1_RENDER_SURFACE_HPP
