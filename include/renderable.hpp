#ifndef VPYTHON_RENDERABLE_HPP
#define VPYTHON_RENDERABLE_HPP

#include "util/rgba.hpp"
#include "util/extent.hpp"
#include <boost/shared_ptr.hpp>

using boost::shared_ptr;
class z_comparator;

// This primarily serves as a means of communicating information down to the
// various primitives that may or may not need it from the render_surface.  Most
// of the members are simply references to the real values in the owning 
// render_surface.
struct view
{
	// The position of the camera in world space.
	vector camera;
	// The direction the camera is pointing - a unit vector.
	vector& forward;
	// The center of the scene in world space.
	vector& center;
	// The width of the window in pixels.
	float window_width;
	// The height of the window in pixels.
	float& window_height;
	// True if the forward vector changed since the last rending operation.
	bool forward_changed;
	// The Global Scaling Factor
	double& gcf;
	bool gcf_changed;
	int lod_adjust;
	
	inline view( vector& n_forward, vector& n_center, float& n_width, 
		float& n_height, bool n_forward_changed, double& n_gcf, 
		bool n_gcf_changed)
		: forward( n_forward), center(n_center), window_width( n_width), 
		window_height( n_height), forward_changed( n_forward_changed), 
		gcf( n_gcf), gcf_changed( n_gcf_changed), lod_adjust(0)
	{
	}
};

// Virtual base class for all renderable objects and composites.
class renderable
{
private:
	// The model geometry needs to be recalculated
	bool model_damaged;	
	// The z-axis damage needs to be recalculated (only done if color.alpha 
	// != 1.0)
	bool z_damaged;

public:
	rgba color;
	renderable();
	virtual ~renderable();
	
	// Renders itself.
	virtual void gl_render(const view&);
	
	// Renders itself for picking.  Since it is not visible, the object should
	// not worry about things like transparency or texture mapping.
	virtual void gl_pick_render( const view&);
		
	// Report the total extent of the object.
	virtual void grow_extent( extent&);

	// Report the approximate center of the object.
	virtual vector get_center() const = 0;
	
	// Called unconditionally by the render loop prior to rendering the object.
	void refresh_cache( const view&);

protected:
	// If a subclass changes a property that affects its cached state, it must
	// call this function to ensure that its cache is updated on the next render
	// pass.
	inline void model_damage() { model_damaged = true; }
	inline void z_damage() { z_damaged = true; }
	
	bool visible;
	friend class z_comparator;
	// A function that must be overridden if an object wants to cache its state
	// for rendering optimization.
	virtual void update_cache( const view& v);
	// This function is called if the only thing that was damaged was the z-order
	// that the primitives are rendered in.
	virtual void update_z_sort( const view& forward);
};

// A depth sorting criterion for STL-compatable sorting algorithms.  This 
// implementation only performs 4 adds, 6 multiplies, and one comparison.  It
// could be made faster if the virtual function get_center() was somehow made
// non-virtual, but that isn't possible right now since curve, convex, and faces
// have such a different notion of the "center" of the body compared to the other
// objects.
class z_comparator
{
 private:
	vector forward;
 public:
	z_comparator( const vector& f)
		: forward( f) {}
	
	// Multiple versions of this are provided to enable various implementations
	// of the internal storage format for renderable objects.
	inline bool 
	operator()( const shared_ptr<renderable> lhs, const shared_ptr<renderable> rhs) const
	{ return forward.dot( lhs->get_center()) > forward.dot( rhs->get_center()); }

	inline bool 
	operator()( const renderable* lhs, const renderable* rhs) const
	{ return forward.dot( lhs->get_center()) > forward.dot( rhs->get_center()); }
	
};

#endif // !defined VPYTHON_RENDERABLE_HPP
