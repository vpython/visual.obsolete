#ifndef VPYTHON_RENDER_CORE_HPP
#define VPYTHON_RENDER_CORE_HPP

#include "util/vector.hpp"
#include "util/rgba.hpp"
#include "renderable.hpp"
#include "util/extent.hpp"
#include "util/lighting.hpp"
#include "util/timer.hpp"

// #include <FTGLPixmapFont.h>
#include <sigc++/signal.h>
#include <list>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/iterator/indirect_iterator.hpp>
using boost::shared_ptr;
using boost::indirect_iterator;

/** A class that manages all OpenGL aspects of a given scene.  This class
	requires platform-specific support from render_surface to manage an OpenGL
	rendering context.
	
	TODO: make sure that all render_surface objects share the same 
	Gdk::GL::Context.
*/
class render_core
{
 private: // Private data
	float window_width; ///< The last reported width of the window.
	float window_height; ///< The last reported height of the window.
 
	vector center; ///< The observed center of the display, in world space.
	vector forward; ///< The direction of the camera, in world space.
	vector up; ///< The vertical orientation of the camera, in world space.
	/** True initally and whenever the camera direction changes. */
	bool forward_changed;
 
	extent world_extent; ///< The extent of the current world.
	/** The number of scene renderings since the last extent calculation */
	unsigned cycles_since_extent;

	double fov; ///< The field of view, in degrees.
	bool autoscale; ///< True if Visual should scale the camera's position automatically.
	/** True if Visual should automatically center the center of the scene. */
	bool autocenter;
	/** A scaling factor determined by middle mouse button scrolling. */
	double user_scale;
	/** A scaling factor determined by the extent of the scene. */
	double world_scale;
	/** The global scaling factor.  It is used to ensure that objects with 
	 large dimensions are rendered properly.  See the .cpp file for details.
	*/
	double gcf;
	/** True if the gcf has changed since the last render cycle. */
	bool gcf_changed;
	
	/** The set of active lights. */
	std::list<shared_ptr<light> > lights;
	typedef indirect_iterator<std::list<shared_ptr<light> >::iterator> light_iterator;
	
	rgba ambient; ///< The ambient light color.
	/** Called at the beginning of a render cycle to establish lighting. */
	void enable_lights();
	/** Called at the end of a render cycle to complete lighting. */
	void disable_lights();
	
	/** A historesis timer to calculate the time to render each frame. */
	hist_timer fps;
	/** Experimental support for rendering text using FTGL. */
	// shared_ptr<FTGLPixmapFont> fps_font;
 
private:
	/** Set up the OpenGL transforms from world space to view space. */
	void world_to_view_transform( view&, int whicheye = 0, bool forpick = false);
	/** Renders the scene for one eye.
		@param scene The dimensions of the scene, to be propogated to this 
			render_core's children.
		@param eye Which eye is being rendered.  -1 for the left, 0 for the
			center, and 1 for the right.
		@param anaglyph  True if using anaglyph stereo requiring color 
			desaturation or grayscaling.
		@param coloranaglyph  True if colors must be grayscaled, false if colors
			must be desaturated.
	*/
	bool draw( view&, int eye=0, bool anaglyph=false, bool coloranaglyph=false);

public: // Public Data.
	enum mouse_mode_t { ZOOM_ROTATE, ZOOM_ROLL, PAN, FIXED } mouse_mode;
	enum mouse_button { NONE, LEFT, RIGHT, MIDDLE };
	enum stereo_mode_t { NO_STEREO, PASSIVE_STEREO, ACTIVE_STEREO,
		REDBLUE_STEREO, REDCYAN_STEREO, YELLOWBLUE_STEREO, GREENMAGENTA_STEREO 
	} stereo_mode;
	
	/** Older machines should set this to some number between -6 and 0.  All of
		the tesselated models choose a lower level of detail based on this value
		when it is less than 0.
	*/
	int lod_adjust;
	rgba background; ///< The background color of the scene.

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
	
	/** Add an additional light source. */
	void add_light( shared_ptr<light> n_light);
	/** Change the background ambient lighting. */
	void set_ambient( const rgba& color) { ambient = color; }
	/** Remove an existing light source. */
	void remove_light( shared_ptr<light> old_light);
	/** Get the list of lights for this window. */
	const std::list< shared_ptr<light> >& 
	get_lights() const
	{ return lights; }
	/** Clears the set of existing lights, and creates a pair of default lights. 
		The first is at <0.25, 0.5, 1.0> inf with 0.8 gray.  The second is at 
		<-1.0, -0.25, -0> inf with 0.2 gray.  This function is automatically
		called once at construction time.
	*/
	void illuminate_default();
	
 public: // Public functions
	render_core();
	void recalc_extent();

	/** Renders the scene once.  The enveloping widget is resposible for calling
		 this function appropriately.
 		@return If false, something catastrophic has happened and the
 		application should probably exit.
	*/
	bool render_scene();
	
	/** Inform this object that the window has been allocated.  This function
 		performs once-only initialization tasks that can only be performed
 		after the window has been allocated.
 	*/
	void report_realize();
 
	/** Report that the mouse moved with one mouse button down.  
 		@param dx horizontal change in mouse position in pixels.
 		@param dy vertical change in mouse position in pixels.
	*/
	void report_mouse_motion( float dx, float dy, mouse_button button);
 
	/** Report that the size of the widget has changed. 
 		@param new_width: The new width of the window. 
 		@param new_height: The new height of the window.
 		*/
	void report_resize( float new_width, float new_height);
	
	/** Determine which object (if any) was picked by the cursor.
 	    @param x the x-position of the mouse cursor, in pixels.
		@param y the y-position of the mouse cursor, in pixels.
		@param d_pixels: the allowable variation in pixels to successfully score
			a hit.
		@return  the nearest selected object.  May be NULL if nothing was hit.
	*/
	shared_ptr<renderable> pick( float x, float y, float d_pixels = 2.0);

	/** A signal that makes the wrapping widget's rendering context 
		active.  The wrapping object must connect to it.
	*/
	SigC::Signal0<void> gl_begin;
	/** A signal that deactivates the wrapping widget's rendering context.  
		The wrapping object must connect to it.
	*/
	SigC::Signal0<void> gl_end;
	/** A signal that swaps the buffers for its rendering context.  The wrapping
		object must connect to it.
	*/
	SigC::Signal0<void> gl_swap_buffers;
};


#endif // !defined VPYTHON_RENDER_SURFACE_HPP
