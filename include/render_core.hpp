#ifndef VPYTHON_RENDER_CORE_HPP
#define VPYTHON_RENDER_CORE_HPP

// #include <gtkmm/gl/drawingarea.h>
#include "util/vector.hpp"
#include "util/rgba.hpp"
#include "renderable.hpp"
#include "util/extent.hpp"
#include "util/lighting.hpp"
#include "util/timer.hpp"

#include <FTGLPixmapFont.h>
#include <sigc++/signal.h>
#include <list>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/iterator/indirect_iterator.hpp>
using boost::shared_ptr;
using boost::indirect_iterator;

// TODO: make sure that all render_surface objects share the same 
// Gdk::GL::Context.
class render_core
{
 private: // Private data
	float window_width;
	float window_height;
 
	vector center; // The observed center of the display
	vector forward; // The direction of the camera
	vector up; // The vertical axis of rotation.
	// True initally and whenever the camera direction changes.
	bool forward_changed;

	extent world_extent; // The extent of the current world.
	// The number of scene renderings since the last extent calculation
	unsigned cycles_since_extent;

	double fov; // The field of view.
	bool autoscale; // True if Visual should scale the camera's position.
	//< True if Visual should automatically center the center of the scene.
	bool autocenter;
	// A scaling factor determined by middle mouse button scrolling.
	double user_scale;
	// A scaling factor determined by the extent of the scene.
	double world_scale;
	// The gcf, or global scaling factor, is used to ensure that objects with 
	// large dimensions are rendered properly.  See the .cpp file for details.
	double gcf;
	// True if the gcf has changed since the last render cycle.
	bool gcf_changed;
 
	// The last known coordinates of the mouse.
	float last_mousepos_x;
	float last_mousepos_y;
	
	std::vector<shared_ptr<light> > lights;
	rgba ambient;
	void enable_lights();
	void disable_lights();
	
	hist_timer fps;
	size_t cycles_since_fps;
	shared_ptr<FTGLPixmapFont> fps_font;
 
private:
	// Helper function that encapsulates OpenGL transforms from world space to 
	// view space.
	enum eye { LEFT_EYE, RIGHT_EYE, CENTERED };
	void world_to_view_transform( view&, int whicheye = 0);
	bool draw( view&, int eye=0, bool anaglyph=false, bool coloranaglyph=false);

public: // Public Data.
	enum mouse_mode_t { ZOOM_ROTATE, ZOOM_ROLL, PAN, FIXED } mouse_mode;
	enum mouse_button { NONE, LEFT, RIGHT, MIDDLE };
	enum stereo_mode_t { NO_STEREO, PASSIVE_STEREO, ACTIVE_STEREO,
		REDBLUE_STEREO, REDCYAN_STEREO, YELLOWBLUE_STEREO, GREENMAGENTA_STEREO 
	} stereo_mode;
	
	// Older machines should set this to some number between -6 and 0.  All of
	// the tesselated models choose a lower level of detail based on this value
	// when it is less than 0.
	int lod_adjust;
	rgba background;

	/** Opaque objects to be rendered into world space. */
	std::list<shared_ptr<renderable> > layer_world;
	typedef indirect_iterator<std::list<shared_ptr<renderable> >::iterator> world_iterator;

	/** objects with a nonzero level of transparency that need to be depth sorted
	 prior to rendering.
	*/
	std::vector<shared_ptr<renderable> > layer_world_transparent;
	typedef indirect_iterator<std::vector<shared_ptr<renderable> >::iterator> world_trans_iterator;
	
	/** Objects to be rendered after all others in screen space. */
	std::list<shared_ptr<renderable> > layer_screen;
	typedef indirect_iterator<std::list<shared_ptr<renderable> >::iterator> screen_iterator;
	
	void add_light( shared_ptr<light> n_light);
	void set_ambient( const rgba& color) { ambient = color; }
	void remove_light( shared_ptr<light> old_light);
	/** Get the list of lights for this window. */
	const std::vector< shared_ptr<light> >& 
	get_lights() const
	{ return lights; }
	/** Creates a pair of default lights. 
		The first is at <0.25, 0.5, 1.0> inf with 0.8 gray.  The second is at 
		<-1.0, -0.25, -0> inf with 0.2 gray.
	*/
	void illuminate_default();
	
 public: // Public functions
	render_core();
	void recalc_extent();

 	// Renders the scene.  The enveloping widget is resposible for calling this
	// function appropriately.  If it returns false, something has gone very wrong
	// and the application should be shut down.
	bool render_scene();
	
	// Inform this object that the window has been allocated.
	void report_realize();
 
	// Report that the mouse moved with one mouse button down.  dx and dy are
	// changes in mouse position on the screen in pixels.  If the mouse moves
	// with two buttons down, it must be reported twice.
	void report_mouse_motion( float dx, float dy, mouse_button button);
 
	// Report that the size of the widget has changed.
	void report_resize( float new_width, float new_height);
 
	// The wrapping widget must supply callbacks that make it the active rendering
	// context.
	SigC::Signal0<void> gl_begin;
	SigC::Signal0<void> gl_end;
	SigC::Signal0<void> gl_swap_buffers;
};


#endif // !defined VPYTHON_RENDER_SURFACE_HPP
