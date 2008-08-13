// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "ring.hpp"
#include "util/displaylist.hpp"
#include "util/errors.hpp"
#include "util/gl_enable.hpp"

#include <utility>
#include <boost/scoped_array.hpp>
using boost::scoped_array;

namespace cvisual {
	
bool
ring::degenerate()
{
	return radius == 0.0;
}

ring::ring()
	: thickness( 0.0)
{
}

ring::ring( const ring& other)
	: axial( other), thickness( other.thickness)
{	
}

ring::~ring()
{
}

void
ring::set_thickness( double t)
{
	thickness = t;
}

double
ring::get_thickness()
{
	return thickness;
}

void
ring::gl_pick_render( const view& scene)
{
	gl_matrix_stackguard guard;
	do_render_opaque( scene, 7, 10);
	return;
}

void
ring::gl_render( const view& scene)
{
	if (degenerate())
		return;
	// Level of detail estimation.  See sphere::gl_render().

	// The number of subdivisions around the hoop's radial direction.
	double band_coverage = (thickness ? scene.pixel_coverage( pos, thickness) 
			: scene.pixel_coverage(pos, radius*0.1));
	int bands = static_cast<int>(band_coverage * 0.3);
	if (bands < 10)
		bands *= 4;
	else if (bands < 30)
		bands *= 2;
	if (bands < 0)
		bands = 60;
	bands = clamp( 7, bands, 80);
	// The number of subdivions around the hoop's tangential direction.
	double ring_coverage = scene.pixel_coverage( pos, radius);
	int rings = static_cast<int>(ring_coverage * 0.3);
	if (rings < 10)
		rings *= 4;
	else if (rings < 30)
		rings *= 2;
	if (rings < 0)
		rings = 80;
	rings = clamp( 7, rings, 80);
	
	clear_gl_error(); 
	{
		gl_enable_client vertex_array( GL_VERTEX_ARRAY);
		gl_enable_client normal_array( GL_NORMAL_ARRAY);
		gl_matrix_stackguard guard;
		if (opacity == 1.0)
			do_render_opaque( scene, rings, bands);
		else
			do_render_translucent( scene, rings, bands);
	}
	check_gl_error();
	return;
}

void
ring::grow_extent( extent& world)
{
	if (degenerate())
		return;
	world.add_sphere( pos, radius + (thickness ? thickness : (radius * 0.1)));
	world.add_body();
}

void 
ring::band_prepare( const view& scene, size_t rings, size_t bands, scoped_array<vector> & vertexes, scoped_array<vector>& normals)
{
	// Implementation.  In software, I create a pair of arrays for vertex and
	// normal data, filling them with the coordinates for one band of 
	// the ring as a triangle strip.  Each successive band is created with 
	// sequential calls to glRotate() using the same vertex array, thereby 
	// taking advantage of OpenGL's hardware for the bulk of the transform labor.
		
	double scaled_radius = radius * scene.gcf;
	double scaled_thickness = scaled_radius * 0.1;
	if (thickness != 0.0)
		scaled_thickness = thickness * scene.gcf;

	// The first band is a triangle strip at the point where the default ring
	// passes through the yz plane through the +z axis.  The extra pair of vertexes
	// and normals is to form a closed loop.  The format is of a pair of
	// interleaved rings, the first (in the xz plane) is stored at the even indexes;
	// the second (rotated slightly above the first) is stored at the odd indexes.
	const size_t n_verts = bands*2+2;
	vertexes.reset( new vector[n_verts] );
	normals.reset( new vector[n_verts] );
	vertexes[0] = vertexes[ bands * 2] = 
		vector( 0, 0, scaled_radius + scaled_thickness);
	normals[0] = normals[bands * 2] = vertexes[0].norm();
	tmatrix rotator = rotation(
		2.0 * M_PI / bands, 
		vector(0, 1, 0), 
		vector( 0, 0, scaled_radius));
	tmatrix normal_rotator = rotation(2.0 *  M_PI / bands, vector( 0, 1, 0));
	for (size_t i = 2; i < bands * 2; i += 2) {
		vertexes[i] = rotator * vertexes[i-2];
		normals[i] = normal_rotator * normals[i-2];
	}
	// Rotate the single circle about the +x axis to produce the second 
	// interleaved circle.
	rotator = rotation( M_PI * 2.0 / rings, vector(1,0,0));
	for (size_t i = 1; i < bands * 2; i += 2) {
		vertexes[i] = rotator * vertexes[i-1];
		normals[i] = rotator * normals[i-1];
	}
	vertexes[bands*2+1] = vertexes[1];
	normals[bands*2+1] = normals[1];

	glVertexPointer( 3, GL_DOUBLE, 0, vertexes.get());
	glNormalPointer( GL_DOUBLE, 0, normals.get());
	color.gl_set(opacity);
	vector scaled_pos = pos * scene.gcf;
	glTranslated( scaled_pos.x, scaled_pos.y, scaled_pos.z);
	model_world_transform().gl_mult();
}

void
ring::gl_draw( const view& scene, size_t rings, size_t bands)
{
	// Draw the first strip.
	for (size_t i = 0; i < rings; ++i) {
		// Successively render the same triangle strip for each band, 
		// rotated about the model's x axis into the next position.
		// TODO: I believe this is very slow!
		glRotated( 360.0 / rings, 1, 0, 0);
		glDrawArrays( GL_TRIANGLE_STRIP, 0, bands * 2 + 2);
	}
}

void 
ring::do_render_opaque( const view& scene, size_t rings, size_t bands)
{
	scoped_array<vector> vertexes, normals;
	band_prepare( scene, rings, bands, vertexes, normals );
	gl_draw( scene, rings, bands);
	return;
}

void
ring::do_render_translucent( const view& scene, size_t rings, size_t bands)
{
	// TODO: I believe this is quite wrong

	scoped_array<vector> vertexes, normals;
	band_prepare( scene, rings, bands, vertexes, normals );
	
	gl_enable clip0( GL_CLIP_PLANE0);
	gl_enable cull_face( GL_CULL_FACE);
	
	tmatrix modelview; modelview.gl_modelview_get();
	vertex eye_center = modelview.project( vector()); // same as w_column?
	// Correct for perspective division
	double eye_radius = (vector(eye_center) - modelview * vector(radius*scene.gcf, 0)).mag();
	eye_center.z -= eye_radius * std::cos( M_PI*0.5 - std::atan( scene.tan_hfov_x));
	// The plane equations
	double show_back[] = { 0, 0, -1, eye_center.z / eye_center.w };
	double show_front[] = { 0, 0, 1, -eye_center.z / eye_center.w };
	
	{ 	gl_matrix_stackguard g;
		glLoadIdentity();
		glClipPlane( GL_CLIP_PLANE0, show_back);
	}
	glCullFace( GL_FRONT);
	gl_draw( scene, rings, bands);
	glCullFace( GL_BACK);
	gl_draw( scene, rings, bands);
	
	{	gl_matrix_stackguard g;
		glLoadIdentity();
		glClipPlane( GL_CLIP_PLANE0, show_front);
	}
	glCullFace( GL_FRONT);
	gl_draw( scene, rings, bands);
	glCullFace( GL_BACK);
	gl_draw( scene, rings, bands);
}

PRIMITIVE_TYPEINFO_IMPL(ring)

} // !namespace cvisual
