#include "cylinder.hpp"
#include "util/errors.hpp"
#include "util/displaylist.hpp"
#include "util/quadric.hpp"
#include <GL/gl.h>
#include <vector>

#include <boost/scoped_array.hpp>
using boost::scoped_array;

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
	: radius(1.0)
{
}

void
cylinder::update_cache( const view&)
{
	if (first) {
		first = false;
		// The number of faces corrisponding to each level of detail.
		size_t n_faces[] = { 7, 11, 19, 29, 37, 69 };
		size_t n_stacks[] = {1, 1, 2, 3, 5, 10 };
		for (size_t i = 0; i < 6; ++i) {
			cylinder_simple_model[i].gl_compile_begin();
			render_cylinder_model( n_faces[i], n_stacks[i]);
			cylinder_simple_model[i].gl_compile_end();
		}
	}
}

void
cylinder::set_radius( double r)
{
	radius = r;
}

void 
cylinder::gl_pick_render( const view& scene)
{
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
	clear_gl_error();

	// See sphere::gl_render() for a description of the level of detail calc.
	double camera_dist = ((pos - scene.camera) * scene.gcf).mag();
	double lod_determinant = radius * scene.gcf / camera_dist;
	size_t lod = 0;
	if (lod_determinant > .3)
		lod = 5;
	else if (lod_determinant > .12)
		lod = 4;
	else if (lod_determinant > .035)
		lod = 3;
	else if (lod_determinant > .02)
		lod = 2;
	else if (lod_determinant > .01)
		lod = 1;
	if (lod >= -scene.lod_adjust)
		lod += scene.lod_adjust;
	assert( lod <= 5);
	
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
}
	
vector 
cylinder::get_center() const
{
	return pos + axis*0.5;
}

SIMPLE_DISPLAYOBJECT_TYPEINFO_IMPL(cylinder)
