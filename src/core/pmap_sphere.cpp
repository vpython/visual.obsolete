// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "pmap_sphere.hpp"
#include "util/quadric.hpp"
#include "util/errors.hpp"
#include "util/clipping_plane.hpp"
#include <iostream>
#include <GL/glu.h>

namespace cvisual {

bool pmap_sphere::first = true;
// The first six cached descriptions are used for non-textured spheres, and the
// last six include texture data.
displaylist pmap_sphere::lod_cache[2];

// Renders a unit sphere with polar equidistant texture coordinates.
void
render_pmap_half_sphere( int slices, int stacks)
{
	// Optimizations:
	// Use tmatrix for all rotations vice sin/cos (x4).
	// Use mirror flipping to quickly render both the upper and lower halves.
	// Only compute each vertex position exactly once by means of interleaving.
	// Since it is a unit sphere, the normals are the same as the vertexes. (x2)
	
	// Verify that there are an even number of stacks.
	assert( stacks % 2 == 0);
	// Verify an adequate number of stacks for this algorithm.
	assert( stacks > 3);
	// Verify that the sphere is not a flat disk.
	assert( slices > 2);
	
	// A rotation tmatrix for the vertex/normal data.  It rotates CW about +y.
	tmatrix rotator = rotation( 2 * M_PI / slices, vector( 0, 1, 0));
	// A declination tmatrix for the vertex/normal data.  It rotates CW about +z.
	tmatrix declinator = rotation( M_PI / stacks, vector(0,0,-1));
	// A rotation matrix for the texture data.  It rotates CCW about the left
	// half of the texture.
	tmatrix tex_rotator 
		= rotation( 2*M_PI / slices, vector(0,0,-1), vector(0.25, 0.25));
	// The incriment to move across the texture (in declination).
	double tex_declination = 0.5 / stacks;

	// Data storage for the bands in the middle.
	vector vertex_coord_band[slices*2+2];
	double tex_coord_band[slices*2+2][2];

	// Start with the top and bottom end caps as a triangle fans.
	{
		vector vertex_coord[slices+2];
		double tex_coord[slices+2][2];
		// The coordinate at the N pole.
		tex_coord[0][0] = 0.25;
		tex_coord[0][1] = 0.25;
		vertex_coord[0] = vector(0,1,0);
		// The first coordinate around the pole.
		vector last_tex_coord( 0.25 + tex_declination, 0.25);
		tex_coord[1][0] = last_tex_coord.x;
		tex_coord[1][1] = last_tex_coord.y;
		vertex_coord[1] = declinator * vertex_coord[0];
		for (int i = 2; i < slices+2; ++i) {
			last_tex_coord = tex_rotator * last_tex_coord;
			tex_coord[i][0] = last_tex_coord.x;
			tex_coord[i][1] = last_tex_coord.y;
			vertex_coord[i] = rotator * vertex_coord[i-1];
		}
		// glEnableClientState( GL_NORMAL_ARRAY);
		glEnableClientState( GL_VERTEX_ARRAY);
		glEnableClientState( GL_TEXTURE_COORD_ARRAY);
		glVertexPointer( 3, GL_DOUBLE, 0, &vertex_coord[0].x);
		// glNormalPointer( GL_DOUBLE, 0, &vertex_coord[0].x);
		glTexCoordPointer( 2, GL_DOUBLE, 0, tex_coord[0]);
		glDrawArrays( GL_TRIANGLE_FAN, 0, slices+2);
	}
	// Initialize the first band.
	vertex_coord_band[0] = declinator * vector(0,1,0);
	vertex_coord_band[1] = declinator * vertex_coord_band[0];
	vector upper_tex_coord = vector( 0.25 + tex_declination, 0.25);
	vector lower_tex_coord = vector( 0.25 + tex_declination*2, 0.25);
	tex_coord_band[0][0] = upper_tex_coord.x;
	tex_coord_band[0][1] = upper_tex_coord.y;
	tex_coord_band[1][0] = lower_tex_coord.x;
	tex_coord_band[1][1] = lower_tex_coord.y;
	for (int i = 2; i < slices*2 + 2; i+= 2) {
		vertex_coord_band[i] = rotator * vertex_coord_band[i-2];
		vertex_coord_band[i+1] = rotator * vertex_coord_band[i-1];
		upper_tex_coord = tex_rotator * upper_tex_coord;
		lower_tex_coord = tex_rotator * lower_tex_coord;
		tex_coord_band[i][0] = upper_tex_coord.x;
		tex_coord_band[i][1] = upper_tex_coord.y;
		tex_coord_band[i+1][0] = lower_tex_coord.x;
		tex_coord_band[i+1][1] = lower_tex_coord.y;
	}
	glVertexPointer( 3, GL_DOUBLE, 0, &vertex_coord_band[0].x);
	// glNormalPointer( GL_DOUBLE, 0, &vertex_coord_band[0].x);
	glTexCoordPointer(2, GL_DOUBLE, 0, tex_coord_band[0]);
	glDrawArrays( GL_TRIANGLE_STRIP, 0, slices*2+2);
	
	// Draw the remaining bands.
	for (int band = 1; band < (stacks/2)-1; ++band) {
		vector next_tex_coord( 
			tex_coord_band[1][0] + tex_declination, tex_coord_band[1][1]);
		tex_coord_band[0][0] = tex_coord_band[1][0];
		tex_coord_band[0][1] = tex_coord_band[1][1];
		tex_coord_band[1][0] += tex_declination;
		
		vertex_coord_band[0] = vertex_coord_band[1];
		vertex_coord_band[1] = declinator * vertex_coord_band[1];
		for (int i = 2; i < slices*2 + 2; i += 2) {
			vertex_coord_band[i] = vertex_coord_band[i+1];
			vertex_coord_band[i+1] = rotator * vertex_coord_band[i-1];
			next_tex_coord = tex_rotator * next_tex_coord;
			tex_coord_band[i][0] = tex_coord_band[i+1][0];
			tex_coord_band[i][1] = tex_coord_band[i+1][1];
			tex_coord_band[i+1][0] = next_tex_coord.x;
			tex_coord_band[i+1][1] = next_tex_coord.y;
		}
		glDrawArrays( GL_TRIANGLE_STRIP, 0, slices*2+2);
	}
	
	// glDisableClientState( GL_NORMAL_ARRAY);
	glDisableClientState( GL_VERTEX_ARRAY);
	glDisableClientState( GL_TEXTURE_COORD_ARRAY);
}


pmap_sphere::pmap_sphere()
	: radius( 1.0)
{
}

pmap_sphere::pmap_sphere( int)
	: radius(1.0)
{
}

pmap_sphere::~pmap_sphere()
{
}

void
pmap_sphere::set_radius( const double& r)
{
	model_damage();
	radius = r;
}

double
pmap_sphere::get_radius() const
{
	return radius;
}

void
pmap_sphere::set_texture( shared_ptr<texture> t)
{
	tex = t;
}

shared_ptr<texture>
pmap_sphere::get_texture()
{
	return tex;
}

void
pmap_sphere::gl_render( const view& geometry)
{
	clear_gl_error();
	
	int lod = 1;
	
	// Apply transforms.
	gl_matrix_stackguard guard;
	// Position of the body in view space.
	const vector view_pos = pos * geometry.gcf;
	glTranslated( view_pos.x, view_pos.y, view_pos.z);
	model_world_transform().gl_mult();
	const vector scale = get_scale() * geometry.gcf;
	glScaled( scale.x, scale.y, scale.z);

	if (tex) { // Render a textured body
		// Set up the texturing
		glEnable( GL_TEXTURE_2D);
		glDisable( GL_LIGHTING);
		tex->gl_activate();
		// Use white lighting to modulate the texture.
		rgba(1.0, 1.0, 1.0, 1.0).gl_set();
	
		lod_cache[lod].gl_render();
		
		// Cleanup.
		glEnable( GL_LIGHTING);
		glDisable( GL_TEXTURE_2D);		
	}
	else {
		// Render a simple sphere.
		color.gl_set();
		lod_cache[lod].gl_render();
	}
	check_gl_error();
}

void
pmap_sphere::grow_extent( extent& e)
{
	e.add_sphere( pos, radius);
	e.add_body();
}

void
pmap_sphere::update_cache( const view&)
{
	if (first) {
		clear_gl_error();
		first = false;
		lod_cache[0].gl_compile_begin();
		glMatrixMode( GL_TEXTURE);
		glPushMatrix();
		glScaled( 1, 2, 1);
		render_pmap_half_sphere( 140, 70);
		glPopMatrix();
		glMatrixMode( GL_MODELVIEW);
		lod_cache[0].gl_compile_end();

		lod_cache[1].gl_compile_begin();
		lod_cache[0].gl_render();
		glScaled( 1, -1, 1);
		#if 1
		glMatrixMode( GL_TEXTURE);
		glTranslated( -0.5, 0, 0);
		glScaled( -1, 1, 1);
		glTranslated( 0.5, 0, 0);
		glMatrixMode( GL_MODELVIEW);
		#endif
		lod_cache[0].gl_render();
		lod_cache[1].gl_compile_end();
		check_gl_error();
	}
}

vector
pmap_sphere::get_scale()
{
	return vector( radius, radius, radius);
}

SIMPLE_DISPLAYOBJECT_TYPEINFO_IMPL(pmap_sphere)

} // !namespace cvisual
