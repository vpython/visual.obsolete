#ifndef VPYTHON_DISPLAY_KERNEL_HPP
#define VPYTHON_DISPLAY_KERNEL_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "renderable.hpp"
#include "util/vector.hpp"
#include "util/rgba.hpp"
#include "util/extent.hpp"
#include "util/lighting.hpp"
#include "util/timer.hpp"
#include "util/thread.hpp"

#include <sigc++/signal.h>
#include <list>
#include <vector>

#include <boost/iterator/indirect_iterator.hpp>

namespace cvisual {

using boost::indirect_iterator;

/** A class that manages all OpenGL aspects of a given scene.  This class
	requires platform-specific support from render_surface to manage an OpenGL
	rendering context and mouse and keyboard interaction.
*/
class display_kernel
{
 private: // Private data
	mutable mutex mtx;
	
	float window_width; ///< The last reported width of the window.
	float window_height; ///< The last reported height of the window.
 
	shared_vector center; ///< The observed center of the display, in world space.
	shared_vector forward; ///< The direction of the camera, in world space.
	shared_vector up; ///< The vertical orientation of the scene, in world space.
	vector range; ///< The range that each axis of the scene should cover in the view.
	
	/** True initally and whenever the camera direction changes.  Set to false
	 * after every render cycle. 
	 */
	bool forward_changed;
 
	extent world_extent; ///< The extent of the current world.
	/** The number of scene renderings since the last extent calculation */
	unsigned cycles_since_extent;

	double fov; ///< The field of view, in radians
	bool autoscale; ///< True if Visual should scale the camera's position automatically.
	/** True if Visual should automatically center the center of the scene. */
	bool autocenter;
	/** True if the autoscaler should compute uniform axes. */
	bool uniform;
	/** A scaling factor determined by middle mouse button scrolling. */
	double user_scale;
	/** A scaling factor determined by the extent of the scene. */
	double world_scale;
	/** The global scaling factor.  It is used to ensure that objects with 
	 large dimensions are rendered properly.  See the .cpp file for details.
	*/
	double gcf;
	/** True if the gcf has changed since the last render cycle.  Set to false
	 * after every rendering cycle. 
	 */
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

	/** Whether or not we should display the speed of the renderer.  
	 * Default: true. 
	 */
	bool show_renderspeed;
	rgba background; ///< The background color of the scene.
	rgba forground; ///< The default color for objects to be rendered into the scene.
 
	/** Set up the OpenGL transforms from world space to view space. */
	void world_to_view_transform( view&, int whicheye = 0, bool forpick = false);
	/** Renders the scene for one eye.
		@param scene The dimensions of the scene, to be propogated to this 
			display_kernel's children.
		@param eye Which eye is being rendered.  -1 for the left, 0 for the
			center, and 1 for the right.
		@param anaglyph  True if using anaglyph stereo requiring color 
			desaturation or grayscaling.
		@param coloranaglyph  True if colors must be grayscaled, false if colors
			must be desaturated.
	*/
	bool draw( view&, int eye=0, bool anaglyph=false, bool coloranaglyph=false);
	
	/** Opaque objects to be rendered into world space. */
	std::list<shared_ptr<renderable> > layer_world;
	typedef indirect_iterator<std::list<shared_ptr<renderable> >::iterator> world_iterator;

	/** objects with a nonzero level of transparency that need to be depth sorted
		prior to rendering.
	*/
	std::vector<shared_ptr<renderable> > layer_world_transparent;
	typedef indirect_iterator<std::vector<shared_ptr<renderable> >::iterator> world_trans_iterator;
	
	// Computes the extent of the scene and takes action for autozoom and 
	// autoscaling.
	void recalc_extent();
	
	// Compute the tangents of half the vertical and half the horizontal
	// true fields-of-view.
	void tan_hfov( double* x, double* y);
	

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
	
	/** Add an additional light source. */
	void add_light( shared_ptr<light> n_light);
	/** Change the background ambient lighting. */
	void set_ambient( const rgba& color) { lock L(mtx); ambient = color; }
	rgba get_ambient() { return ambient; }
	/** Remove an existing light source. */
	void remove_light( shared_ptr<light> old_light);
	/** Get the list of lights for this window. */
	std::list< shared_ptr<light> >
	get_lights() const
	{ return lights; }
	/** Clears the set of existing lights, and creates a pair of default lights. 
		The first is at <0.25, 0.5, 1.0> inf with 0.8 gray.  The second is at 
		<-1.0, -0.25, -0> inf with 0.2 gray.  This function is automatically
		called once at construction time.
	*/
	void illuminate_default();
	
	/** Add a normal renderable object to the list of objects to be rendered into
	 *  world space.
	 */
	virtual void add_renderable( shared_ptr<renderable>);
	
	/**  Remove a renderable object from this display, regardless of which layer
	 *   it resides in.  */
	virtual void remove_renderable( shared_ptr<renderable>);

 public: // Public functions
	// Compute the location of the camera based on the current geometry.
	vector calc_camera();
	
	display_kernel();
	virtual ~display_kernel();

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
	
	/** Recenters the scene.  Call this function exactly once to move the visual
	 * center of the scene to the true center of the scene.  This will work
	 * regardless of the value of this->autocenter.
	 */
	void recenter();
	
	/** Rescales the scene.  Call this function exactly once to scale the scene
	 * such that it fits within the entire window.  This will work
	 * regardless of the value of this->autoscale.
	 */
	void rescale();
	
	// Python properties
	void set_up( const vector& n_up);
	shared_vector& get_up();

	void set_forward( const vector& n_forward);
	shared_vector& get_forward();

	void set_scale( const vector& n_scale);
	vector get_scale();
	
	void set_center( const vector& n_center);
	shared_vector& get_center();

	void set_fov( double);
	double get_fov();

	void set_uniform( bool);
	bool is_uniform();

	void set_background( const rgba&);
	rgba get_background();

	void set_forground( const rgba&);
	rgba get_forground();

	void set_autoscale( bool);
	bool get_autoscale();

	void set_autocenter( bool);
	bool get_autocenter();
	
	void set_show_renderspeed( bool);
	bool is_showing_renderspeed();
	double get_renderspeed();

	void set_range_d( double);
	void set_range( const vector&);
	vector get_range();

	// TODO: Implement me too.  Right now it is fixed since changing it didn't
	// appear to work right when moving the scene into the page.
	void set_stereodepth( double);
	double get_stereodepth();
	
	// The only mode that cannot be changed after initialization is active, 
	// which will result in a gl_error exception when rendered.  The completing
	// display class will have to perform some filtering on this parameter.  This
	// properties setter will not change the mode if the new one is invalid.
	void set_stereomode( std::string mode);
	std::string get_stereomode();
	
	// A list of all objects rendered into this display_kernel.  Modifying it
	// does not propogate to the owning display_kernel.
	std::list<shared_ptr<renderable> > get_objects() const;

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

} // !namespace cvisual

#endif // !defined VPYTHON_RENDER_SURFACE_HPP
