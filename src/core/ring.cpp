// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "ring.hpp"
#include "util/displaylist.hpp"
#include "util/errors.hpp"
#include <utility>

namespace cvisual {

ring::ring()
	: thickness( 0.0), radius( 1.0)
{
}

void
ring::set_radius( double r)
{
	radius = r;
}

void
ring::set_thickness( double t)
{
	thickness = t;
}

void
ring::gl_pick_render( const view& scene)
{
	do_render_opaque( scene, 7, 10);
	return;
}

// TODO: Figure out how to do transparency for this body.
void
ring::gl_render( const view& scene)
{
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

	do_render_opaque( scene, rings, bands);
	return;
}

void
ring::grow_extent( extent& world)
{
	world.add_sphere( pos, radius + thickness ? thickness : (radius * 0.1));
	world.add_body();
}

void 
ring::do_render_opaque( const view& scene, size_t rings, size_t bands)
{
	// Implementation.  In software, I create a pair of arrays for vertex and
	// normal data, filling them with the coordinates for one band of 
	// the ring as a triangle strip.  Each successive band is created with 
	// sequential calls to glRotate() using the same vertex array, thereby 
	// taking advantage of OpenGL's hardware for the bulk of the transform labor.
	if (radius == 0.0 || !visible)
		return;
		
	double scaled_radius = radius * scene.gcf;
	double scaled_thickness = scaled_radius * 0.1;
	if (thickness != 0.0)
		scaled_thickness = thickness * scene.gcf;

	// The first band is a triangle strip at the point where the default ring
	// passes through the yz plane through the +z axis.  The extra pair of vertexes
	// and normals is to form a closed loop.  The format is of a pair of
	// interleaved rings, the first (in the xz plane) is stored at the even indexes;
	// the second (rotated slightly above the first) is stored at the odd indexes.
	vector vertexes[bands * 2 + 2];
	vector normals[bands * 2 + 2];
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
	
	// Point OpenGL at the vertex data for the first triangle strip.
	clear_gl_error();
	glEnableClientState( GL_VERTEX_ARRAY);
	glEnableClientState( GL_NORMAL_ARRAY);
	glVertexPointer( 3, GL_DOUBLE, 0, vertexes);
	glNormalPointer( GL_DOUBLE, 0, normals);
	color.gl_set();
	{	
		gl_matrix_stackguard guard;
		vector scaled_pos = pos * scene.gcf;
		glTranslated( scaled_pos.x, scaled_pos.y, scaled_pos.z);
		model_world_transform().gl_mult();
		// Draw the first strip.
		glDrawArrays( GL_TRIANGLE_STRIP, 0, bands * 2 + 2);
		for (size_t i = 0; i < rings; ++i) {
			// Successively render the same triangle strip for each band, 
			// rotated about the model's x axis into the next position.
			glRotated( 360.0 / rings, 1, 0, 0);
			glDrawArrays( GL_TRIANGLE_STRIP, 0, bands * 2 + 2);
		}
	}
	
	glDisableClientState( GL_VERTEX_ARRAY);
	glDisableClientState( GL_NORMAL_ARRAY);
	check_gl_error();
	return;
}

SIMPLE_DISPLAYOBJECT_TYPEINFO_IMPL(ring)

} // !namespace cvisual
