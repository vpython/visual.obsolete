// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "cylinder.hpp"
#include "util/errors.hpp"
#include "util/displaylist.hpp"
#include "util/quadric.hpp"

namespace cvisual {

bool
cylinder::degenerate()
{
	return !visible || radius == 0.0 || axis.mag() == 0.0;
}

// TODO: This model currently uses three-deep glPushMatrix() to run.  It should
// be reduced.
static void
render_cylinder_model( size_t n_sides, size_t n_stacks = 1)
{
	quadric q;
	q.render_cylinder( 1.0, 1.0, n_sides, n_stacks);
	q.render_disk( 1.0, n_sides, n_stacks);
	glPushMatrix();
	glTranslatef( 1.0f, 0.0f, 0.0f);
	q.render_disk( 1.0, n_sides, n_stacks * 3);
	glPopMatrix();
}

static displaylist cylinder_simple_model[6];

bool cylinder::first = true;

cylinder::cylinder()
{
}

cylinder::cylinder( const cylinder& other)
	: axial( other)
{
}

void
cylinder::update_cache( const view&)
{
	if (first) {
		first = false;
		// The number of faces corrisponding to each level of detail.
		size_t n_faces[] = { 8, 16, 32, 64, 96, 188 };
		size_t n_stacks[] = {1, 1, 3, 6, 10, 20 };
		for (size_t i = 0; i < 6; ++i) {
			cylinder_simple_model[i].gl_compile_begin();
			render_cylinder_model( n_faces[i], n_stacks[i]);
			cylinder_simple_model[i].gl_compile_end();
		}
	}
}

void
cylinder::set_length( double l)
{
	axis = axis.norm() * l;
}

double
cylinder::get_length()
{
	return axis.mag();
}

void 
cylinder::gl_pick_render( const view& scene)
{
	if (degenerate())
		return;
	size_t lod = 2;
	gl_matrix_stackguard guard;
	vector view_pos = pos * scene.gcf;
	glTranslated( view_pos.x, view_pos.y, view_pos.z);
	model_world_transform().gl_mult();
	const double radial_scale = radius * scene.gcf;
	const double axial_scale = axis.mag() * scene.gcf;
	glScaled( axial_scale, radial_scale, radial_scale);
	cylinder_simple_model[lod].gl_render();
}

void 
cylinder::gl_render( const view& scene)
{
	if (degenerate())
		return;
	clear_gl_error();

	// See sphere::gl_render() for a description of the level of detail calc.
	double coverage = scene.pixel_coverage( pos, radius);
	int lod = 0;
	if (coverage < 0)
		lod = 5;
	else if (coverage < 10)
		lod = 0;
	else if (coverage < 25)
		lod = 1;
	else if (coverage < 50)
		lod = 2;
	else if (coverage < 196)
		lod = 3;
	else if (coverage < 400)
		lod = 4;
	else
		lod = 5;
	lod += scene.lod_adjust;
	if (lod < 0)
		lod = 0;
	else if (lod > 5)
		lod = 5;
	
	const bool shiny = shininess != 1.0;
	if (shiny) {
		glLightModeli( GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
		int gl_shininess = static_cast<int>(shininess * 128);
		glMaterialfv( GL_FRONT, GL_SPECULAR, &rgba( .8, .8, .8).red);
		glMateriali( GL_FRONT, GL_SHININESS, gl_shininess);		
	}
	
	gl_matrix_stackguard guard;
	vector view_pos = pos * scene.gcf;
	glTranslated( view_pos.x, view_pos.y, view_pos.z);
	model_world_transform().gl_mult();
	const double radial_scale = radius * scene.gcf;
	const double axial_scale = axis.mag() * scene.gcf;
	glScaled( axial_scale, radial_scale, radial_scale);
	
	if (color.alpha != 1.0) {
		// Setup for blending
		glEnable( GL_BLEND);
		glEnable( GL_CULL_FACE);
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		color.gl_set();
				
		// Render the back half.
		glCullFace( GL_FRONT);
		cylinder_simple_model[lod].gl_render();
		
		// Render the front half.
		glCullFace( GL_BACK);
		cylinder_simple_model[lod].gl_render();
		
		// Cleanup.
		glDisable( GL_CULL_FACE);
		glDisable( GL_BLEND);
	}
	else {
		color.gl_set();
		cylinder_simple_model[lod].gl_render();
	}
	
	// Cleanup.
	if (shiny) {
		glLightModeli( GL_LIGHT_MODEL_LOCAL_VIEWER, 0);
		glMaterialfv( GL_FRONT, GL_SPECULAR, &rgba( 0, 0, 0).red);		
	}
	check_gl_error();
}

void 
cylinder::grow_extent( extent& e)
{
	if (radius < axis.mag() * 2) {
		// Treat as a straight line.
		e.add_point( pos);
		e.add_point( pos + axis);
	}
	else {
		// A sphere centered roughly at the centroid of the body.
		e.add_sphere( pos + axis*0.5, std::max(radius, axis.mag() * 0.5));
	}
	e.add_body();
}
	
vector 
cylinder::get_center() const
{
	return pos + axis*0.5;
}

PRIMITIVE_TYPEINFO_IMPL(cylinder)

} // !namespace cvisual
