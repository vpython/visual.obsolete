#include "sphere.hpp"
#include "util/quadric.hpp"
#include "util/errors.hpp"
#include "util/clipping_plane.hpp"
#include <iostream>
#include <GL/glu.h>

bool sphere::first = true;
// The first four cached descriptions are used for non-textured spheres, and the
// last four include texture data.
displaylist sphere::lod_cache[12];


sphere::sphere()
	: radius( 1.0)
{
}

sphere::sphere( int)
	: radius(1.0)
{
}

sphere::~sphere()
{
}

void
sphere::set_radius( const double& r)
{
	model_damage();
	radius = r;
}

double
sphere::get_radius() const
{
	return radius;
}

void
sphere::set_texture( shared_ptr<texture> t)
{
	tex = t;
}

shared_ptr<texture>
sphere::get_texture()
{
	return tex;
}

void
sphere::gl_render( const view& geometry)
{
	clear_gl_error();
	
	// All of the models are slightly different when shininess is specified.
	const bool shiny = shininess != 1.0;
	
	// The level of detail determinant captures the idea that objects farther
	// away appear to be smaller in the view.  It is a very rough idea of 
	// "apparent size" and is used to determine what value of lod should be 
	// used.  Each of the cutoff values below were determined experimentally
	// based on looking at the terminator between the most and least illuminated
	// parts of a set of yellow spheres.  This boundary was the least forgiving
	// due to the sharp contrast on the curved surface.
	double camera_dist = ((pos - geometry.camera) * geometry.gcf).mag();
	double lod_determinant = radius * geometry.gcf / camera_dist;
	int lod = 0;
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
	lod += geometry.lod_adjust;
	if (lod > 5)
		lod = 5;
	if (lod < 0)
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
	if (shiny) {
		glLightModeli( GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
		int gl_shininess = static_cast<int>(shininess * 128);
		glMaterialfv( GL_FRONT, GL_SPECULAR, &rgba( .8, .8, .8).red);
		glMateriali( GL_FRONT, GL_SHININESS, gl_shininess);
	}
	
	// Mode specific rendering code follows
	if (color.alpha != 1.0) { // Render a transparent sphere.
		// Since spheres have identical symmetry
		// along the view axis regardless of thier orientation, this code is
		// easy (albeit somewhat slow) thanks to GL_CULL_FACE. 
		
		// Setup for blending
		glEnable( GL_BLEND);
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		color.gl_set();
		
		glEnable( GL_CULL_FACE);
		
		// Render the back half.
		glCullFace( GL_FRONT);
		lod_cache[lod].gl_render();
		
		// Render the front half.
		glCullFace( GL_BACK);
		lod_cache[lod].gl_render();
		
		// Cleanup.
		glDisable( GL_CULL_FACE);
		glDisable( GL_BLEND);
	}
	else if (tex) { // Render a textured body
		// Shift to the textured sphere model set.
		size_t tex_lod = lod + 6;
		
		// Set up the texturing
		glEnable( GL_TEXTURE_2D);
		tex->gl_activate();
		// Use white lighting to modulate the texture.
		rgba(1.0, 1.0, 1.0, 1.0).gl_set();
		
		// Use separate specular highlights, when requested.
		if (shiny)
			glLightModeli( GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
		
		// Render the object, using culling of the backface if the camera is not
		// within the space of the body.
		const bool hide_back = (geometry.camera - pos).mag() > radius;
		if (hide_back) {
			glEnable( GL_CULL_FACE);
			glCullFace( GL_BACK);
		}
		lod_cache[tex_lod].gl_render();
		if (hide_back) {
			glDisable( GL_CULL_FACE);
		}
		
		// Cleanup.
		glDisable( GL_TEXTURE_2D);
		if (shiny)
			glLightModeli( GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);			
	}
	else {
		// Render a simple sphere.
		color.gl_set();
		lod_cache[lod].gl_render();
	}
	if (shiny) {
		glLightModeli( GL_LIGHT_MODEL_LOCAL_VIEWER, 0);
		glMaterialfv( GL_FRONT, GL_SPECULAR, &rgba( 0, 0, 0).red);
	}

	check_gl_error();
}

void
sphere::grow_extent( extent& e)
{
	e.add_sphere( pos, radius);
}

// Level of detail notes: 
// I think the most sensitive test is to render yellow (more contrasty) lit 
// spheres, and look
// at the most distinct terminator (between the most bright part and the
// darkest part).
// The #3 model is good for objects < .25 in in apparent diameter.
// The #2 model is good for objects less than .75 inch apparent diameter.
// TODO: perhaps another model should be right about here?
// The #1 model goes for up to 3 inches apparent diameter.
// The #0 sphere survives extremely close scrutiny, at a stiff performance
// penalty.
void
sphere::update_cache( const view&)
{
	if (first) {
		clear_gl_error();
		first = false;
		quadric sph;
		
		// To be uesd for very distant spheres.
		// .25 in
		lod_cache[0].gl_compile_begin();
		sph.render_sphere( 1.0, 13, 7);
		lod_cache[0].gl_compile_end();
		
		// .5 in
		lod_cache[1].gl_compile_begin();
		sph.render_sphere( 1.0, 19, 11);
		lod_cache[1].gl_compile_end();

		// 1 in (not quite, but pretty close)
		lod_cache[2].gl_compile_begin();
		sph.render_sphere( 1.0, 35, 19);
		lod_cache[2].gl_compile_end();

		// 2 in
		lod_cache[3].gl_compile_begin();
		sph.render_sphere( 1.0, 55, 29);
		lod_cache[3].gl_compile_end();
		
		// 5 in
		lod_cache[4].gl_compile_begin();
		sph.render_sphere( 1.0, 70, 34);
		lod_cache[4].gl_compile_end();
		
		// Only for the very largest bodies.
		lod_cache[5].gl_compile_begin();
		sph.render_sphere( 1.0, 140, 69);
		lod_cache[5].gl_compile_end();
	
		// All of the higher-numbered models also include texture coordinates.
		sph.do_textures( true);
		lod_cache[6].gl_compile_begin();
		sph.render_sphere( 1.0, 13, 7);
		lod_cache[6].gl_compile_end();

		lod_cache[7].gl_compile_begin();
		sph.render_sphere( 1.0, 19, 11);
		lod_cache[7].gl_compile_end();

		lod_cache[8].gl_compile_begin();
		sph.render_sphere( 1.0, 35, 19);
		lod_cache[8].gl_compile_end();

		lod_cache[9].gl_compile_begin();
		sph.render_sphere( 1.0, 55, 29);
		lod_cache[9].gl_compile_end();
		
		lod_cache[10].gl_compile_begin();
		sph.render_sphere( 1.0, 70, 34);
		lod_cache[10].gl_compile_end();
		
		// Only for the very largest bodies.
		lod_cache[11].gl_compile_begin();
		sph.render_sphere( 1.0, 140, 69);
		lod_cache[11].gl_compile_end();
		check_gl_error();
	}
}

vector
sphere::get_scale()
{
	return vector( radius, radius, radius);
}

SIMPLE_DISPLAYOBJECT_TYPEINFO_IMPL(sphere)
