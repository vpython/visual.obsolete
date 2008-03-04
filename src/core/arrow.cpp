// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "arrow.hpp"
#include "util/errors.hpp"
#include "util/gl_enable.hpp"

namespace cvisual {
	
bool
arrow::degenerate()
{
	return axis.mag() == 0.0;
}

// This model can be resorted in about 22 usec on my PIII 800 MHz machine.
void
arrow::recalc_sorted_model( const double& gcf) 
{	
	double sw = 0.0;
	double sl = 0.0;
	double hw = 0.0;
	double l = 0.0;
	double hl = 0.0;
	effective_geometry( hw, sw, l, hl, gcf);
	sl = l - hl;
	// Left face
	sorted_model->faces[0] = triangle( 
		vector(0, sw, sw), vector(0, sw, -sw), vector(0, -sw, -sw));
	sorted_model->faces[1] = triangle(
		vector(0, -sw, -sw), vector(0, -sw, sw), vector(0, sw, sw));
	// Front triangle
	sorted_model->faces[2] = triangle( 
		vector(0, sw, sw), vector(0, -sw, sw), vector(sl, sw, sw));
	sorted_model->faces[2].center = vector( sl / 2.0, 0, sw);
	sorted_model->faces[3] = triangle(
		vector(sl, sw, sw), vector(0, -sw, sw), vector(sl, -sw, sw));
	sorted_model->faces[3].center = vector( sl / 2.0, 0, sw);
	// Back face
	sorted_model->faces[4] = triangle( 
		vector(0, sw, -sw), vector(sl, sw, -sw), vector(0, -sw, -sw));
	sorted_model->faces[4].center = vector( sl / 2.0, 0, -sw);
	sorted_model->faces[5] = triangle(
		vector(sl, sw, -sw), vector(sl, -sw, -sw), vector(0, -sw, -sw));
	sorted_model->faces[5].center = vector( sl / 2.0, 0, -sw);
	// Top Face
	sorted_model->faces[6] = triangle(
		vector(0, sw, sw), vector(sl, sw, sw), vector(sl, sw, -sw));
	sorted_model->faces[6].center = vector( sl / 2.0, sw, 0);
	sorted_model->faces[7] = triangle(
		vector(sl, sw, -sw), vector(0, sw, -sw), vector(0, sw, sw));
	sorted_model->faces[7].center = vector( sl / 2.0, sw, 0);
	// Bottom triangle
	sorted_model->faces[8] = triangle(
		vector(0, -sw, sw), vector(sl, -sw, -sw), vector(sl, -sw, sw));
	sorted_model->faces[8].center = vector( sl / 2.0, -sw, 0);
	sorted_model->faces[9] = triangle(
		vector(sl, -sw, -sw), vector(0, -sw, sw), vector(0, -sw, -sw));
	sorted_model->faces[9].center = vector( sl / 2.0, -sw, 0);
	// Arrowhead back
	// TODO: Figure out why this object isn't getting sorted properly.
	sorted_model->faces[10] = triangle(
		vector(sl, -sw, sw), vector(sl, -hw, hw), vector(sl, sw, sw));
	// sorted_model.faces[10].center = vector( sl, 0, 0);
	sorted_model->faces[11] = triangle(
		vector(sl, sw, sw), vector(sl, -hw, hw), vector(sl, hw, hw));
	// sorted_model.faces[11].center = vector( sl, 0, 0);
	sorted_model->faces[12] = triangle(
		vector(sl, hw, hw), vector(sl, sw, -sw), vector(sl, sw, sw));
	// sorted_model.faces[12].center = vector( sl, 0, 0);
	sorted_model->faces[13] = triangle(
		vector(sl, hw, hw), vector(sl, hw, -hw), vector(sl, sw, -sw));
	// sorted_model.faces[13].center = vector( sl, 0, 0);
	sorted_model->faces[14] = triangle(
		vector(sl, sw, -sw), vector(sl, hw, -hw), vector(sl, -sw, -sw));
	// sorted_model.faces[14].center = vector( sl, 0, 0);
	sorted_model->faces[15] = triangle(
		vector(sl, -sw, -sw), vector(sl, hw, -hw), vector(sl, -hw, -hw));
	// sorted_model.faces[15].center = vector( sl, 0, 0);
	sorted_model->faces[16] = triangle(
		vector(sl, -hw, -hw), vector(sl, -sw, sw), vector(sl, -sw, -sw));
	// sorted_model.faces[16].center = vector( sl, 0, 0);
	sorted_model->faces[17] = triangle(
		vector(sl, -hw, -hw), vector(sl, -hw, hw), vector(sl, -sw, sw));
	// sorted_model.faces[17].center = vector( sl, 0, 0);
	// Arrowhead tip.
	sorted_model->faces[18] = triangle( // TOP
		vector(l, 0, 0), vector(sl, hw, -hw), vector(sl, hw, hw));
	sorted_model->faces[19] = triangle( // FRONT
		vector(l, 0, 0), vector(sl, hw, hw), vector(sl, -hw, hw));
	sorted_model->faces[20] = triangle( // BACK
		vector(l, 0, 0), vector(sl, -hw, -hw), vector(sl, hw, -hw));
	sorted_model->faces[21] = triangle( // BOTTOM
		vector(l, 0, 0), vector(sl, -hw, -hw), vector(sl, -hw, hw));
}

void
arrow::cache_transparent_model( const view&)
{
	assert( color.opacity < 1.0);
	// Compile the rendering code for the beastie.
	clear_gl_error();
	model.gl_compile_begin();
	{	
		gl_enable blend( GL_BLEND);
		// gl_enable polygon_smooth( GL_POLYGON_SMOOTH);
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBegin( GL_TRIANGLES);
			sorted_model->gl_render();
		glEnd();
	}
	model.gl_compile_end();
	check_gl_error();
}

arrow::arrow()
	: fixedwidth(false), headwidth(0), headlength(0), shaftwidth(0)
{
}

arrow::arrow( const arrow& other)
	: primitive(other), fixedwidth( other.fixedwidth), 
	headwidth( other.headwidth), headlength( other.headlength), 
	shaftwidth( other.shaftwidth)
{	
}

arrow::~arrow()
{
}

void
arrow::set_headwidth( double hw)
{
	model_damage();
	headwidth = hw;
}

double
arrow::get_headwidth()
{
	return headwidth;
}
	
void
arrow::set_headlength( double hl)
{
	model_damage();
	headlength = hl;
}


double
arrow::get_headlength()
{
	return headlength;
}
	
void
arrow::set_shaftwidth( double sw)
{
	model_damage();
	shaftwidth = sw;
}

double
arrow::get_shaftwidth()
{
	return shaftwidth;
}
	
void
arrow::set_fixedwidth( bool fixed)
{
	model_damage();
	fixedwidth = fixed;
}

bool
arrow::is_fixedwidth()
{
	return fixedwidth;
}
	
void
arrow::set_length( double l)
{
	model_damage();
	axis = axis.norm() * l;
}

double
arrow::get_length()
{
	return axis.mag();
}

vector
arrow::get_center() const
{
	return (pos + axis)/2.0;
}

void 
arrow::gl_pick_render( const view& scene)
{
	if (degenerate() || !model)
		return;
	if (!model)
		update_cache( scene);
	gl_matrix_stackguard guard;
	vector view_pos = pos * scene.gcf;
	glTranslated( view_pos.x, view_pos.y, view_pos.z);
	model_world_transform().gl_mult();
	model.gl_render();
}

void
arrow::gl_render( const view& scene)
{
	if (degenerate())
		return;

	color.gl_set();
	gl_matrix_stackguard guard;
	vector view_pos = pos * scene.gcf;
	glTranslated( view_pos.x, view_pos.y, view_pos.z);			
	model_world_transform().gl_mult();
	lighting_prepare();
	shiny_prepare();
	model.gl_render();
	shiny_complete();
	lighting_complete();
}

void
arrow::grow_extent( extent& world)
{
	if (degenerate())
		return;
#if 1
	// Hmm.  A more accurate, but much slower calc of the extent of this body.
	double hl;
	double hw;
	double len;
	double sw;
	effective_geometry( hw, sw, len, hl, 1.0);
	// A sphere large enough to contain all four corners of the head.
	world.add_sphere( pos + axis.norm()*(len-hl), hw * 1.414213562);
	// A sphere large enough to contain all four corners of the base.
	world.add_sphere( pos, sw * 1.414213562);
#else
	world.add_point( pos);
#endif
	// The tip.
	world.add_point( pos + axis);
	world.add_body();
}

void
arrow::get_material_matrix(const view& v, tmatrix& out)
{
	double hl, hw, len, sw;
	effective_geometry( hw, sw, len, hl, v.gcf);

	out.translate( vector(0,.5,.5) );
	out.scale( vector(1,1,1) * (1 / std::max( axis.mag()*v.gcf, hw )) );
}

void
arrow::update_z_sort( const view& scene)
{
	if (!sorted_model) {
		sorted_model.reset( new z_sorted_model<triangle, 22>());
		recalc_sorted_model( scene.gcf);
	}
	// A unit vector poining towards the object from the camera, in world space.
	vector object_forward = ((pos + axis*0.5) - scene.camera).norm();
	// The transformation from world space to model space.
	tmatrix inv = world_model_transform();
	// A unit vector pointing towards the object from the camera, in model space.
	vector model_forward = inv.times_v( object_forward).norm();
	sorted_model->sort( model_forward);
	
	cache_transparent_model(scene);
}

void
arrow::update_cache( const view& scene)
{
	double eff_shaftlength;
	double eff_shaftwidth;
	double eff_length;
	double eff_headwidth;
	double eff_headlength;
	double eff_halfwidth;
	double eff_halfheadwidth;
	effective_geometry( eff_headwidth, eff_shaftwidth, eff_length, 
		eff_headlength, scene.gcf);
	eff_shaftlength = eff_length - eff_headlength;
	eff_halfwidth = 0.5*eff_shaftwidth;
	eff_halfheadwidth = 0.5*eff_headwidth;
	
	if (color.opacity != 1.0) {
		if (!sorted_model) {
			sorted_model.reset( new z_sorted_model<triangle, 22>());
		}
		recalc_sorted_model( scene.gcf);
		tmatrix inv = world_model_transform();
		vector model_forward = (inv * scene.forward).norm();
		sorted_model->sort( model_forward);
		cache_transparent_model(scene);
	}
	else {
		clear_gl_error();
		model.gl_compile_begin();
		{			
			const vector tip( eff_length, 0, 0);
			const vector backface[4] = { 
				vector(0, eff_halfwidth, -eff_halfwidth), 
				vector(0, eff_halfwidth, eff_halfwidth),
				vector(0, -eff_halfwidth, eff_halfwidth),
				vector(0, -eff_halfwidth, -eff_halfwidth) 
			};
			const vector side_corners[4] = {
				backface[0] + vector(eff_shaftlength, 0, 0),
				backface[1] + vector(eff_shaftlength, 0, 0),
				backface[2] + vector(eff_shaftlength, 0, 0),
				backface[3] + vector(eff_shaftlength, 0, 0)	
			};
			const vector head_corners[4] = {
				// top, back, lower, front.
				vector(eff_shaftlength, eff_halfheadwidth, -eff_halfheadwidth ),
				vector(eff_shaftlength, eff_halfheadwidth, eff_halfheadwidth ),
				vector(eff_shaftlength, -eff_halfheadwidth, eff_halfheadwidth ),
				vector(eff_shaftlength, -eff_halfheadwidth, -eff_halfheadwidth )		
			};
			const vector head_normals[4] = {
				// Normal vectors for each triangle: top, front, lower, back.
				-(tip - head_corners[0]).cross( head_corners[1] - head_corners[0]).norm(),
				-(tip - head_corners[1]).cross( head_corners[2] - head_corners[1]).norm(),
				-(tip - head_corners[2]).cross( head_corners[3] - head_corners[2]).norm(),
				-(tip - head_corners[3]).cross( head_corners[0] - head_corners[3]).norm()
			};
			glBegin(GL_QUADS);
				// Shaft end
				vector(-1, 0, 0).gl_normal();
				backface[3].gl_render();
				backface[2].gl_render();
				backface[1].gl_render();
				backface[0].gl_render();
				// Shaft top
				vector(0, 1, 0).norm().gl_normal();
				backface[0].gl_render();
				backface[1].gl_render();
				side_corners[1].gl_render();
				side_corners[0].gl_render();
				// Shaft Bottom
				vector(0, -1, 0).gl_normal();
				backface[2].gl_render();
				backface[3].gl_render();
				side_corners[3].gl_render();
				side_corners[2].gl_render();
				// Shaft Front
				vector(0, 0, 1).gl_normal();
				backface[1].gl_render();
				backface[2].gl_render();
				side_corners[2].gl_render();
				side_corners[1].gl_render();
				// Shaft Back
				vector(0, 0, -1).gl_normal();
				backface[3].gl_render();
				backface[0].gl_render();
				side_corners[0].gl_render();
				side_corners[3].gl_render();
				// base of the arrow.
				vector(-1, 0, 0).gl_normal();
				head_corners[3].gl_render();
				head_corners[2].gl_render();
				head_corners[1].gl_render();
				head_corners[0].gl_render();
			glEnd();
			glBegin(GL_TRIANGLES);
				// Render the head of the arrow, in top, front, bottom, back order
				head_normals[0].gl_normal();
				tip.gl_render();
				head_corners[0].gl_render();
				head_corners[1].gl_render();
				
				head_normals[1].gl_normal();
				tip.gl_render();
				head_corners[1].gl_render();
				head_corners[2].gl_render();
				
				head_normals[2].gl_normal();
				tip.gl_render();
				head_corners[2].gl_render();
				head_corners[3].gl_render();
				
				head_normals[3].gl_normal();
				tip.gl_render();
				head_corners[3].gl_render();
				head_corners[0].gl_render();
			glEnd();
		}
		model.gl_compile_end();
		check_gl_error();
	}
}

PRIMITIVE_TYPEINFO_IMPL(arrow)

void
arrow::effective_geometry( 
	double& eff_headwidth, double& eff_shaftwidth, double& eff_length, 
	double& eff_headlength, double gcf)
{
	// First calculate the actual geometry based on the specs for headwidth,
	// shaftwidth, shaftlength, and fixedwidth.  This geometry is calculated
	// in world space and multiplied
	static const double min_sw = 0.02; // minimum shaftwidth
	static const double def_sw = 0.1; // default shaftwidth
	static const double def_hw = 2.0; // default headwidth multiplier. (x shaftwidth)
	static const double def_hl = 3.0; // default headlength multiplier. (x shaftwidth)
	// maximum fraction of the total arrow length allocated to the head.
	static const double max_headlength = 0.5;
	
	eff_length = axis.mag() * gcf;
	if (shaftwidth)
		eff_shaftwidth = shaftwidth * gcf;
	else
		eff_shaftwidth = eff_length * def_sw;

	if (headwidth)
		eff_headwidth = headwidth * gcf;
	else
		eff_headwidth = eff_shaftwidth * def_hw;
	
	if (headlength)
		eff_headlength = headlength * gcf;
	else
		eff_headlength = eff_shaftwidth * def_hl;
	
	if (fixedwidth) {
		if (eff_headlength > max_headlength * eff_length)
			eff_headlength = max_headlength * eff_length;
	}
	else {
		if (eff_shaftwidth < eff_length * min_sw) {
			double scale = eff_length * min_sw / eff_shaftwidth;
			eff_shaftwidth = eff_length * min_sw;
			eff_headwidth *= scale;
			eff_headlength *= scale;
		}
		if (eff_headlength > eff_length * max_headlength) {
			double scale = eff_length * max_headlength / eff_headlength;
			eff_headlength = eff_length * max_headlength;
			eff_headwidth *= scale;
			eff_shaftwidth *= scale;
		}
	}
}

} // !namespace cvisual
