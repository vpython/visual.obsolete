#ifndef VPYTHON_RENDERABLE_HPP
#define VPYTHON_RENDERABLE_HPP

#include "util/rgba.hpp"
#include "util/extent.hpp"
#include <boost/shared_ptr.hpp>

using boost::shared_ptr;
class z_comparator;

/** This primarily serves as a means of communicating information down to the
	various primitives that may or may not need it from the render_surface.  Most
	of the members are simply references to the real values in the owning 
	render_surface.
*/
struct view
{
	/// The position of the camera in world space.
	vector camera;
	/// The direction the camera is pointing - a unit vector.
	vector forward;
	/// The center of the scene in world space.
	vector center;
	/// The true up direction of the scene in world space.
	vector up;
	/// The width of the window in pixels.
	float window_width;
	/// The height of the window in pixels.
	float& window_height;
	/// True if the forward vector changed since the last rending operation.
	bool forward_changed;
	/// The Global Scaling Factor
	double& gcf;
	/// True if gcf changed since the last render cycle.
	bool gcf_changed;
	/// The user adjustment to the level-of-detail.
	int lod_adjust;
	/// True in anaglyph stereo rendering modes.
	bool anaglyph;
	/// True in coloranaglyph stereo rendering modes.
	bool coloranaglyph;
	double tan_hfov_x; ///< The tangent of half the horzontal field of view.
	double tan_hfov_y; ///< The tangent of half the vertical field of view.
	
	inline view( vector& n_forward, vector& n_center, float& n_width, 
		float& n_height, bool n_forward_changed, double& n_gcf, 
		bool n_gcf_changed)
		: forward( n_forward), center(n_center), window_width( n_width), 
		window_height( n_height), forward_changed( n_forward_changed), 
		gcf( n_gcf), gcf_changed( n_gcf_changed), lod_adjust(0)
	{
	}
	
	// Compute the apparent diameter, in pixels, of a circle that is parallel
	// to the screen, with a center at pos, and some radius.  If pos is behind
	// the camera, it will return negative.
	double pixel_coverage( const vector& pos, double radius) const;
};

/** Virtual base class for all renderable objects and composites.
 */
class renderable
{
private:
	/** True if the model geometry needs to be recalculated.  This will lead to
	 * refresh_cache being called.
	 */
	bool model_damaged;	
	/** True if the model needs to be resorted.  update_z_sort() will be called
	 * if this is true and color.alpha != 1.0 in the render cycle. 
	 */
	bool z_damaged;

public:
	/** The base color of this body.  Ignored by the variable-color composites
	 * (curve, faces, frame).
	 */
	rgba color;
	/** Default base constructor.  Creates a white, model_damaged object. */
	renderable();
	virtual ~renderable();
	
	/** Called by the render cycle when drawing to the screen.  The default
	 * is to do nothing.
	 */
	virtual void gl_render(const view&);
	
	/** Called when rendering for mouse hit testing.  Since the result is not
	 *  visible, subclasses should not perform texture mapping or blending,
	 * and should use the lowest-quality level of detail that covers the
	 * geometry.
	 */
	virtual void gl_pick_render( const view&);
		
	/** Report the total extent of the object. */
	virtual void grow_extent( extent&);

	/** Report the approximate center of the object.  This is used for depth
	 * sorting of the transparent models.  */
	virtual vector get_center() const = 0;
	
	/** Called by the render loop to determine if an object needs to be updated.
	 * It is called unconditionally by the owning render_surface and determines
	 * whether or not to call update_cache() and/or update_z_sort() based on the
	 * damage states.
	 */
	void refresh_cache( const view&);

protected:
	/** If a subclass changes a property that affects its cached state, it must
		call this function to ensure that its cache is updated on the next render
		pass.
	*/
	inline void model_damage() { model_damaged = true; }
	/** If a subclass changes its state such that it is no longer sorted, but 
	 * does not need to recalculate its entire geometry, it must call this
	 * funciton. */
	inline void z_damage() { z_damaged = true; }
	
	/** True if the object should be rendered on the screen. */
	bool visible;
	friend class z_comparator;
	/** A function that must be overridden if a subclass wants to cache its state
		for rendering optimization.
	*/
	virtual void update_cache( const view& v);
	// This function is called if the only thing that was damaged was the z-order
	// that the primitives are rendered in.
	virtual void update_z_sort( const view& forward);
};

/** A depth sorting criterion for STL-compatable sorting algorithms.  This 
   implementation only performs 4 adds, 6 multiplies, and one comparison.  It
   could be made faster if the virtual function get_center() was somehow made
   non-virtual, but that isn't possible right now since some bodies	
   have such a different notion of the "center" of the body compared to the other
   objects.
 */
class z_comparator
{
 private:
	/** A unit vector along the visual depth axis.*/
	vector forward;
 public:
	/** Create a new comparator based on the specified depth direction. 
	 * @param fore A unit vector along the sorting axis.
	 */
	z_comparator( const vector& fore)
		: forward( fore) {}
	
	/** Apply the sorting criteria.
		@return true if lhs is farther away than rhs. 
	*/
	inline bool 
	operator()( const shared_ptr<renderable> lhs, const shared_ptr<renderable> rhs) const
	{ return forward.dot( lhs->get_center()) > forward.dot( rhs->get_center()); }

	/** Apply the sorting criteria.
		@return true if lhs is farther away than rhs. 
	*/
	inline bool 
	operator()( const renderable* lhs, const renderable* rhs) const
	{ return forward.dot( lhs->get_center()) > forward.dot( rhs->get_center()); }
	
};

/** A utility function that clamps a value to within a specified range.
 * @param lower The lower bound for the value.
 * @param value The value to be clamped.
 * @param upper The upper bound for the value.
 * @return value if it is between lower and upper, otherwise one of the bounds.
 */
template <typename T>
T clamp( T const& lower, T const& value, T const& upper)
{
	if (lower > value)
		return lower;
	if (upper < value)
		return upper;
	return value;
}

#endif // !defined VPYTHON_RENDERABLE_HPP
