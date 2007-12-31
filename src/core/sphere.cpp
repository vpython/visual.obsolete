// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "sphere.hpp"
#include "util/quadric.hpp"
#include "util/errors.hpp"
#include "util/icososphere.hpp"
#include "util/gl_enable.hpp"

#include <vector>

namespace cvisual {

bool sphere::first = true;
// The first four cached descriptions are used for non-textured spheres, and the
// last four include texture data.

displaylist sphere::lod_cache[6];

static std::vector<icososphere> models;

sphere::sphere()
{
}

sphere::sphere( const sphere& other)
	: axial(other)
{
}

sphere::~sphere()
{
}

void
sphere::gl_pick_render( const view& geometry)
{
	if (degenerate())
		return;
	if (first)
    	create_cache();
	clear_gl_error();
	size_t lod = 0;
	gl_matrix_stackguard guard;
	const vector view_pos = pos * geometry.gcf;
	glTranslated( view_pos.x, view_pos.y, view_pos.z);
	model_world_transform().gl_mult();
	const vector scale = get_scale() * geometry.gcf;
	glScaled( scale.x, scale.y, scale.z);
	//lod_cache[lod].gl_render();
	models[lod].gl_render();
	check_gl_error();
}

void
sphere::gl_render( const view& geometry)
{
	if (degenerate())
		return;
	clear_gl_error();
	lighting_prepare();

	// Each of the cutoff values below were determined experimentally
	// based on looking at the terminator between the most and least illuminated
	// parts of a set of yellow spheres.  This boundary was the least forgiving
	// due to the sharp contrast on the curved surface.
	
	// coverage is the radius of this sphere in pixels:
	double coverage = geometry.pixel_coverage( pos, radius);
	int lod = 0;
	if (shiny()) {
		if (coverage < 0) // Behind the camera, but still visible.
			lod = 0;
		else if (coverage < 2)
			lod = 0;
		else if (coverage < 10)
			lod = 1;
		else if (coverage < 50)
			lod = 2;
		else if (coverage < 300)
			lod = 3;
		else if (coverage < 500)
			lod = 4;
		else
			lod = 5;
	}
	else {
		if (coverage < 0) // Behind the camera, but still visible.
			lod = 0;
		else if (coverage < 2)
			lod = 0;
		else if (coverage < 10)
			lod = 1;
		else if (coverage < 50)
			lod = 2;
		else if (coverage < 300)
			lod = 3;
		else if (coverage < 500)
			lod = 4;
		else
			lod = 5;
	}
	lod += geometry.lod_adjust;
	if (lod > 5)
		lod = 5;
	else if (lod < 0)
		lod = 0;

	// Apply transforms.
	gl_matrix_stackguard guard;
	// Position of the body in view space.
	const vector view_pos = pos * geometry.gcf;
	glTranslated( view_pos.x, view_pos.y, view_pos.z);
	model_world_transform().gl_mult();
	const vector scale = get_scale() * geometry.gcf;
	glScaled( scale.x, scale.y, scale.z);

	// Set up material properties for shininess.
	shiny_prepare();

	color.gl_set();
	// Mode specific rendering code follows
	if (tex && (color.opacity != 1.0 || tex->has_opacity())) {
		// textured, and opacity-blended sphere
		// Setup for blending
		gl_enable blend( GL_BLEND);
		gl_enable cull_face( GL_CULL_FACE);
		gl_enable tex2D( GL_TEXTURE_2D);
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Set up the texturing
		// I am limiting the maximum level of detail in this case because it can
		// bring my poor system to a screaming halt.  It still looks good at
		// this level of subdivision.
		size_t tex_lod = std::max(lod, 4);
		tex->gl_activate();

		// Render the back half.
		glCullFace( GL_FRONT);
		lod_cache[tex_lod].gl_render();

		// Render the front half.
		glCullFace( GL_BACK);
		lod_cache[tex_lod].gl_render();
	}
	else if (color.opacity != 1.0) { // Render a transparent constant color sphere
		// Since spheres have identical symmetry
		// along the view axis regardless of thier orientation, this code is
		// easy (albeit somewhat slow) thanks to GL_CULL_FACE.

		// Setup for blending
		gl_enable blend( GL_BLEND);
		gl_enable cull_face( GL_CULL_FACE);
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Render the back half.
		glCullFace( GL_FRONT);
		models[lod].gl_render();

		// Render the front half.
		glCullFace( GL_BACK);
		models[lod].gl_render();

	}
	else if (tex) { // Render a textured body
		// Shift to the textured sphere model set.
		size_t tex_lod = lod;

		// Set up the texturing
		gl_enable tex2D( GL_TEXTURE_2D);
		tex->gl_activate();

		// Render the object, using culling of the backface if the camera is not
		// within the space of the body.
		const bool hide_back = (geometry.camera - pos).mag() > radius;
		if (hide_back) {
			gl_enable cull_face( GL_CULL_FACE);
			glCullFace( GL_BACK);
			lod_cache[tex_lod].gl_render();
		}
		else
			lod_cache[tex_lod].gl_render();
	}
	else {
		// Render a simple sphere.
	//	lod_cache[lod].gl_render();
		models[lod].gl_render();
	}
	shiny_complete();
	lighting_complete();
	check_gl_error();
}

void
sphere::grow_extent( extent& e)
{
	e.add_sphere( pos, radius);
	e.add_body();
}

// Level of detail notes by Jonathan Brandmeyer:
// I think the most sensitive test is to render yellow (more contrasty) lit
// spheres, and look
// at the most distinct terminator (between the most bright part and the
// darkest part).
// The #1 model is good for objects less than .25 inch apparent diameter.
// The #2 model is good for objects less than .6 inch apparent diameter.
// The #3 model is good for objects less than 1.5 inch apparent diameter.
// TODO: perhaps another model should be right about here?
// The #4 model goes for up to 4 inches apparent diameter.
// The #5 sphere survives extremely close scrutiny, at a stiff performance
// penalty.

// December 2007, Bruce Sherwood: On my Windows laptop the choices made
// by Brandmeyer don't seem right, and I've made new ones. I find that
// excellent spheres are made with far fewer sphere slices and stacks,
// which I don't entirely understand.

void
sphere::update_cache( const view&)
	{	if (first)
         	create_cache();
	}

void
sphere::create_cache()
{
        for (int i = 0; i < 6; ++i) {
			models.push_back( icososphere(i+1));
		clear_gl_error();
		first = false;

    	quadric sph;

		sph.do_textures( true);
		
		// Choices by Bruce Sherwood
		lod_cache[0].gl_compile_begin();
		sph.render_sphere( 1.0, 2, 2);
		lod_cache[0].gl_compile_end();

		lod_cache[1].gl_compile_begin();
		sph.render_sphere( 1.0, 4, 4);
		lod_cache[1].gl_compile_end();

		lod_cache[2].gl_compile_begin();
		sph.render_sphere( 1.0, 4, 4);
		lod_cache[2].gl_compile_end();

		lod_cache[3].gl_compile_begin();
		sph.render_sphere( 1.0, 6, 6);
		lod_cache[3].gl_compile_end();

		lod_cache[4].gl_compile_begin();
		sph.render_sphere( 1.0, 8, 8);
		lod_cache[4].gl_compile_end();

		// Only for the very largest bodies.
		lod_cache[5].gl_compile_begin();
		sph.render_sphere( 1.0, 10, 10);
		lod_cache[5].gl_compile_end();
		
		/*
		// The following were the choices of Jonathan Brandmeyer:
		lod_cache[0].gl_compile_begin();
		sph.render_sphere( 1.0, 13, 7);
		lod_cache[0].gl_compile_end();

		lod_cache[1].gl_compile_begin();
		sph.render_sphere( 1.0, 19, 11);
		lod_cache[1].gl_compile_end();

		lod_cache[2].gl_compile_begin();
		sph.render_sphere( 1.0, 35, 19);
		lod_cache[2].gl_compile_end();

		lod_cache[3].gl_compile_begin();
		sph.render_sphere( 1.0, 55, 29);
		lod_cache[3].gl_compile_end();

		lod_cache[4].gl_compile_begin();
		sph.render_sphere( 1.0, 70, 34);
		lod_cache[4].gl_compile_end();

		// Only for the very largest bodies.
		lod_cache[5].gl_compile_begin();
		sph.render_sphere( 1.0, 140, 69);
		lod_cache[5].gl_compile_end();*/
		
		check_gl_error();
	}
}

vector
sphere::get_scale()
{
	return vector( radius, radius, radius);
}

bool
sphere::degenerate()
{
	return !visible || radius == 0.0;
}

PRIMITIVE_TYPEINFO_IMPL(sphere)

} // !namespace cvisual
