#include "frame.hpp"

#include <algorithm>

frame::frame()
	: scale( 1.0, 1.0, 1.0)
{
	model_damage();
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
	
	ret.x_column( x_axis * scale.x * gcf);
	ret.y_column( y_axis * scale.y * gcf);
	ret.z_column( z_axis * scale.z * gcf);
	ret.w_column( pos * gcf);
	ret.w_row();
	return ret;
}

tmatrix
frame::world_frame_transform( const double) const
{
	// Performs a reorientation transform.
	// ret = translation o reorientation o scale
	// ret = iscale o ireorientation o itranslation.
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
	x_axis /= scale.x;
	y_axis /= scale.y;
	z_axis /= scale.z;
	
	ret(0,0) = x_axis.x;
	ret(0,1) = x_axis.y;
	ret(0,2) = x_axis.z;
	ret(0,3) = (pos * x_axis).sum();
	ret(1,0) = y_axis.x;
	ret(1,1) = y_axis.y;
	ret(1,2) = y_axis.z;
	ret(1,3) = (pos * y_axis).sum();
	ret(2,0) = z_axis.x;
	ret(2,1) = z_axis.y;
	ret(2,2) = z_axis.z;
	ret(2,3) = (pos * z_axis).sum();
	
	return ret;
}

void 
frame::add_child( shared_ptr<renderable> obj)
{
	if (obj->color.alpha == 1.0)
		children.push_back( obj);
	else
		trans_children.push_back( obj);
}
	
void 
frame::remove_child( shared_ptr<renderable> obj)
{
	if (obj->color.alpha != 1.0) {
		std::remove( trans_children.begin(), trans_children.end(), obj);
	}
	else
		std::remove( children.begin(), children.end(), obj);
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
frame::update_cache( const view&)
{
}

void 
frame::update_z_sort( const view&)
{
}

void 
frame::gl_render( const view& v)
{
	// Ensure that we always get our cache updated to update our children.
	tmatrix wft = world_frame_transform(v.gcf);
	view local = v;
	local.camera = wft * v.camera;
	local.forward = -local.camera.norm();
	local.center = wft * v.center;
	model_damage();
	{
		gl_matrix_stackguard guard( frame_world_transform(v.gcf));
		
		for (child_iterator i = children.begin(); i != child_iterator(children.end()); ++i) {
			i->refresh_cache(local);
			rgba actual_color = i->color;
			if (v.anaglyph) {
				if (v.coloranaglyph) {
					i->color = actual_color.desaturate();
				}
				else {
					i->color = actual_color.grayscale();
				}
			}
			i->gl_render( local);
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
			i->refresh_cache( local);
			rgba actual_color = i->color;
			if (v.anaglyph) {
				if (v.coloranaglyph) {
					i->color = actual_color.desaturate();
				}
				else {
					i->color = actual_color.grayscale();
				}
			}
			i->gl_render( local);
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
	world.push_frame();
	extent local;
	for (child_iterator i = children.begin(); i != child_iterator(children.end()); ++i) {
		i->grow_extent( local);
		world.add_body();
	}
	for (trans_child_iterator i = children.begin(); i != trans_child_iterator(children.end()); ++i) {
		i->grow_extent( local);
		world.add_body();
	}
	world.add_sphere( local.center() + pos, local.scale(scale) * 0.5);
	world.pop_frame();
}
