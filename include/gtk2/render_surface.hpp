#ifndef VPYTHON_GTK2_RENDER_SURFACE_HPP
#define VPYTHON_GTK2_RENDER_SURFACE_HPP

#include <gtkmm/gl/drawingarea.h>
#include <gtkmm.h>

#include "render_core.hpp"

class render_surface : public Gtk::GL::DrawingArea
{
 private:
	float last_mousepos_x;
	float last_mousepos_y;
 
 public:
	render_surface();
	void add_renderable( shared_ptr<renderable>);
	void add_renderable_screen( shared_ptr<renderable>);
	void remove_renderable( shared_ptr<renderable>);
	void remove_renderable_screen( shared_ptr<renderable>);
	render_core core;
 
 protected:
	// Low-level signal handlers
	// Called when the user moves the mouse across the screen.
	virtual bool on_motion_notify_event( GdkEventMotion* event);
	// Called when the window is resized.
	virtual bool on_configure_event( GdkEventConfigure* event);
	// Called when the mouse enters the scene.
	virtual bool on_enter_notify_event( GdkEventCrossing* event);
	// Called when the window is established for the first time.
 	virtual void on_realize();
	virtual bool on_expose_event( GdkEventExpose*);
 
 private:
	// Functions to be used as callbacks for connections via SigC::slots.
	void gl_begin();
	void gl_end();
	void gl_swap_buffers();
	bool forward_render_scene();
};

class basic_app : public SigC::Object
{
 private:
	Gtk::Main kit;
	struct _init
	{
		_init();
	} init;
	
	Gtk::Window window;
	Gtk::VBox frame;
	Gtk::HButtonBox buttons;
	Gtk::ToggleButton fs;
	Gtk::Button pan;
	Gtk::Button rotate_zoom;
	Gtk::Button quit;
	
 private:
	void on_fullscreen_clicked();
	void on_pan_clicked();
	void on_rotate_clicked();
 
 public:
	render_surface scene;
	basic_app( const char* title);
	void run();	
};

#endif // !defined VPYTHON_GTK2_RENDER_SURFACE_HPP
