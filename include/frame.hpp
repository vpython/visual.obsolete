#ifndef VPYTHON_FRAME_HPP
#define VPYTHON_FRAME_HPP

#include "simple_displayobject.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/iterator/indirect_iterator.hpp>
using boost::shared_ptr;
using boost::indirect_iterator;

#include <vector>

/*
Operations on frame objects include:
get_center() : Use the average of all its children.
update_cache() : Calls refresh_cache() on all its children
update_z_sort() : Never called.  Always re-sort this body's translucent children
	in gl_render().
gl_render() : Calls gl_render() on all its children.  Calls model_damage() to 
	ensure that update_cache() is called later.
grow_extent() : Calls grow_extent() for each of its children, then transforms
	the vertexes of the bounding box and uses those as its bounds.
gl_pick_render() : PushName() on to the Name Stack, and renders its children.
	When looking up names later, the render_core calls lookup_name() with a
	vector<uint>, which the frame uses to recursively look through frames to 
	find the right object.

oolie case: When the frame is scaled up to a superhuge universe and the
	child is very small, the frame_world_transform may overflow OpenGL.  The
	problem lies in the scale variable.
	
another oolie: A transparent object that intersects a frame containing other
	transparent object's will not be rendered in the right order.
*/

class frame : public renderable
{
 private:
	vector pos;
	vector axis;
	vector up;
	/** Establishes the coordinate system into which this object's children
 		are rendered.
 		@param gcf: the global correction factor, propogated from gl_render(). 
 	*/
	tmatrix frame_world_transform( const double gcf) const;
	tmatrix world_frame_transform( const double gcf) const;
	
	std::vector<shared_ptr<renderable> > children;
	typedef indirect_iterator<std::vector<shared_ptr<renderable> >::iterator> 
		child_iterator;
	typedef indirect_iterator<std::vector<shared_ptr<renderable> >::const_iterator> 
		const_child_iterator;
	
	std::vector<shared_ptr<renderable> > trans_children;
	typedef indirect_iterator<std::vector<shared_ptr<renderable> >::iterator> 
		trans_child_iterator;
	typedef indirect_iterator<std::vector<shared_ptr<renderable> >::const_iterator> 
		const_trans_child_iterator;

 public:
	frame();
	vector scale;
	
	void add_child( shared_ptr<renderable> child);
	void remove_child( shared_ptr<renderable> child);
 
	void set_pos( const vector& n_pos);
	void set_axis( const vector& n_axis);
	void set_up( const vector& n_up);
	void set_scale( const vector& n_scale) { scale = n_scale; }
	
	// Lookup the target that belongs to this name.
	shared_ptr<renderable> lookup_name( 
		const unsigned int* name_top, const unsigned int* name_end);
 
 protected:	
	virtual vector get_center() const;
	virtual void update_cache( const view& v);
	virtual void update_z_sort( const view& forward);
	virtual void gl_render( const view&);
	virtual void gl_pick_render( const view&);
	virtual void grow_extent( extent&);
};

#endif // !defined VPYTHON_FRAME_HPP
