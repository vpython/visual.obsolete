#include "frame.hpp"
#include <GL/gl.h>

frame::frame()
	: scale( 1.0, 1.0, 1.0)
{
}

void 
frame::set_pos( const vector& n_pos)
{
	pos = n_pos;
}

void 
frame::set_axis( const vector& n_axis)
{
	axis = n_axis;
}

void 
frame::set_up( const vector& n_up)
{
	up = n_up;
}

tmatrix 
frame::frame_world_transform( const double gcf) const
{
	// Performs a reorientation transform.
	// ret = translation o reorientation o scale
	tmatrix ret;
	// A unit vector along the z_axis.
	vector z_axis = vector(0,0,1);
	if (std::fabs(axis.dot(up) / std::sqrt( up.mag2() * axis.mag2())) > 0.98) {
		if (std::fabs(axis.norm().dot( vector(-1,0,0))) > 0.98)
			z_axis = axis.cross( vector(0,0,1)).norm();
		else
			z_axis = axis.cross( vector(-1,0,0)).norm();
	}
	else {
		z_axis = axis.cross( up).norm();
	}
	
	vector y_axis = z_axis.cross(axis).norm();
	vector x_axis = axis.norm();
	ret.x_column( x_axis * scale.x);
	ret.y_column( y_axis * scale.y);
	ret.z_column( z_axis * scale.z);
	ret.w_column( pos * gcf);
	ret.w_row();
	return ret;
}

shared_ptr<renderable> 
frame::lookup_name( 
	const unsigned int* name_top,
	const unsigned int* name_end)
{
	assert( name_top < name_end);
	assert( *name_top < children.size() + trans_children.size());
	using boost::dynamic_pointer_cast;
	
	shared_ptr<renderable> ret;
	if (*name_top < children.size()) {
		ret = children[*name_top];
	}
	else
		ret = trans_children[*(name_top - children.size())];
	
	if (name_end - name_top > 1) {
		frame* ref_frame = dynamic_cast<frame*>(ret.get());
		assert( ref_frame != NULL);
		return ref_frame->lookup_name(name_top + 1, name_end);
	}
	else
		return ret;
}

vector 
frame::get_center() const
{
	vector ret;
	int ctr = 0;
	const_child_iterator i = children.begin();
	const_child_iterator i_end = children.end();
	while (i != i_end) {
		ret += i->get_center();
		ctr++;
		++i;
	}
	const_trans_child_iterator j = trans_children.begin();
	const_trans_child_iterator j_end = trans_children.end();
	while (j != j_end) {
		ret += j->get_center();
		ctr++;
		++j;
	}
	if (ctr)
		ret /= ctr;
	return ret;
}
	
void 
frame::update_cache( const view& v)
{
	for (child_iterator i = children.begin(); i != child_iterator(children.end()); ++i) {
		i->refresh_cache(v);
	}
	for (trans_child_iterator i = children.begin(); 
		i != trans_child_iterator(children.end()); 
		++i) {
		i->refresh_cache(v);
	}
}

void 
frame::update_z_sort( const view&)
{
}

void 
frame::gl_render( const view& v)
{
	// Ensure that we always get our cache updated to update our children.
	model_damage();
	{
		gl_matrix_stackguard guard( frame_world_transform(v.gcf));
		for (child_iterator i = children.begin(); i != child_iterator(children.end()); ++i) {
			rgba actual_color = i->color;
			if (v.anaglyph) {
				if (v.coloranaglyph) {
					i->color = actual_color.desaturate();
				}
				else {
					i->color = actual_color.grayscale();
				}
			}
			i->gl_render( v);
			if (v.anaglyph)
				i->color = actual_color;
		}
		
		// Perform a depth sort of the transparent children from forward to backward.
		if (trans_children.size() > 1)
			std::stable_sort( trans_children.begin(), trans_children.end(),
				z_comparator( (pos*v.gcf - v.camera).norm()));
		
		for (trans_child_iterator i = trans_children.begin(); 
			i != trans_child_iterator(trans_children.end());
			++i) {
			rgba actual_color = i->color;
			if (v.anaglyph) {
				if (v.coloranaglyph) {
					i->color = actual_color.desaturate();
				}
				else {
					i->color = actual_color.grayscale();
				}
			}
			i->gl_render( v);
			if (v.anaglyph)
				i->color = actual_color;			
		}
	}
}

void
frame::gl_pick_render( const view& scene)
{
	// Push name
	glPushName(0);
	{
		gl_matrix_stackguard guard( frame_world_transform(scene.gcf));
		std::vector<shared_ptr<renderable> >::iterator i = children.begin();
		std::vector<shared_ptr<renderable> >::iterator i_end = children.end();
		// The unique integer to pass to OpenGL.
		unsigned int name = 0;
		while (i != i_end) {
			glLoadName(name);
			(*i)->gl_pick_render( scene);
			++i;
			++name;
		}
		
		if (trans_children.size() > 1)
			std::stable_sort( trans_children.begin(), trans_children.end(),
				z_comparator( (pos*scene.gcf - scene.camera).norm()));
		
		i = trans_children.begin();
		i_end = trans_children.end();
		while (i != i_end) {
			glLoadName(name);
			(*i)->gl_pick_render(scene);
			++i;
			++name;
		}
	}
	// Pop name
	glPopName();
}

void 
frame::grow_extent( extent& world)
{
	extent local;
	for (child_iterator i = children.begin(); i != child_iterator(children.end()); ++i) {
		i->grow_extent( local);
	}
	for (trans_child_iterator i = children.begin(); i != trans_child_iterator(children.end()); ++i) {
		i->grow_extent( local);
	}
	world.add_sphere( local.center() + pos, local.scale(scale) * 0.5);
}
