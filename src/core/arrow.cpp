#include "arrow.hpp"
#include "util/vector.hpp"
#include "util/errors.hpp"
#include "util/sorted_model.hpp"

#include <GL/gl.h>

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
arrow::cache_transparent_model( const view& scene)
{
	assert( color.alpha < 1.0);
	// Compile the rendering code for the beastie.
	clear_gl_error();
	model.gl_compile_begin();
	{
		gl_matrix_stackguard guard;
		vector view_pos = pos * scene.gcf;
		glTranslated( view_pos.x, view_pos.y, view_pos.z);		
		model_world_transform().gl_mult();
		
		glEnable( GL_BLEND);
		// glEnable( GL_POLYGON_SMOOTH);
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBegin( GL_TRIANGLES);
			sorted_model->gl_render();
		glEnd();
		// glDisable( GL_POLYGON_SMOOTH);
		glDisable( GL_BLEND);
	}
	model.gl_compile_end();
	check_gl_error();
}

arrow::arrow()
	: fixedwidth(false), headwidth(0), headlength(0), shaftwidth(0)
{
}

vector
arrow::get_center() const
{
	return (pos + axis)/2.0;
}

void 
arrow::gl_pick_render( const view&)
{
	if (visible && axis.mag() != 0.0)
		model.gl_render();
}

void
arrow::gl_render( const view&)
{
	if (visible && axis.mag() != 0.0) {
		// Uncomment the following and comment out the respecitive lines in
		// refresh_cache() if you need to see the transforms at runtime.
		// gl_matrix_stackguard guard( model_world_transform());
		// dump_glmatrix();
		color.gl_set();
		model.gl_render();
	}
}

void
arrow::grow_extent( extent& world)
{
	world.add_point( pos);
	world.add_point( pos + axis);
	world.add_body();
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
	effective_geometry( eff_headwidth, eff_shaftwidth, eff_length, 
		eff_headlength, scene.gcf);
	eff_shaftlength = eff_length - eff_headlength;
	
	if (color.alpha != 1.0) {
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
			gl_matrix_stackguard guard;
			vector view_pos = pos * scene.gcf;
			glTranslated( view_pos.x, view_pos.y, view_pos.z);			
			model_world_transform().gl_mult();
			
			const vector tip( eff_length, 0, 0);
			const vector backface[4] = { 
				vector(0, eff_shaftwidth, -eff_shaftwidth), 
				vector(0, eff_shaftwidth, eff_shaftwidth),
				vector(0, -eff_shaftwidth, eff_shaftwidth),
				vector(0, -eff_shaftwidth, -eff_shaftwidth) 
			};
			const vector side_corners[4] = {
				backface[0] + vector(eff_shaftlength, 0, 0),
				backface[1] + vector(eff_shaftlength, 0, 0),
				backface[2] + vector(eff_shaftlength, 0, 0),
				backface[3] + vector(eff_shaftlength, 0, 0)	
			};
			const vector head_corners[4] = {
				// top, back, lower, front.
				vector(eff_shaftlength, eff_headwidth, -eff_headwidth ),
				vector(eff_shaftlength, eff_headwidth, eff_headwidth ),
				vector(eff_shaftlength, -eff_headwidth, eff_headwidth ),
				vector(eff_shaftlength, -eff_headwidth, -eff_headwidth )		
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

SIMPLE_DISPLAYOBJECT_TYPEINFO_IMPL(arrow)

void
arrow::effective_geometry( 
	double& eff_headwidth, double& eff_shaftwidth, double& eff_length, 
	double& eff_headlength, double gcf)
{
	// First calculate the actual geometry based on the specs for headwidth,
	// shaftwidth, shaftlength, and fixedwidth.  This geometry is calculated
	// in world space and multiplied
	static const double min_sw = 0.02; // minimum shaftwidth
	static const double def_sw = 0.05; // default shaftwidth
	static const double def_hw = 2.0; // default headwidth multiplier. (x shaftwidth)
	static const double def_aspect = 3.0; // default aspect ratio for the head.
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
		eff_headlength = eff_headwidth * def_aspect;
	
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
