// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "renderable.hpp"
#include "material.hpp"

namespace cvisual {

view::view( const vector& n_forward, vector& n_center, float& n_width,
	float& n_height, bool n_forward_changed,
	double& n_gcf, vector& n_gcfvec,
	bool n_gcf_changed, gl_extensions& glext)
	: forward( n_forward), center(n_center), window_width( n_width),
	window_height( n_height), forward_changed( n_forward_changed),
	gcf( n_gcf), gcfvec( n_gcfvec), gcf_changed( n_gcf_changed), lod_adjust(0),
	anaglyph(false), coloranaglyph(false), tan_hfov_x(0), tan_hfov_y(0),
	screen_objects( z_comparator( forward)), glext(glext)
{
}

view::view( const view& other, const tmatrix& wft)
	: camera( other.camera),
	forward( wft.times_v(other.forward)),
	center( wft * other.center),
	up( wft.times_v(other.up)),
	window_width( other.window_width),
	window_height( other.window_height),
	forward_changed( true),
	gcf( other.gcf),
	gcfvec( other.gcfvec),
	gcf_changed( other.gcf_changed),
	lod_adjust( other.lod_adjust),
	anaglyph( other.anaglyph),
	coloranaglyph( other.coloranaglyph),
	// TODO: tan_hfov_x and tan_hfov_y must be revisited in the face of
	// nonuniform scaling.  It may be more appropriate to describe the viewing
	// frustum in a different way entirely.
	tan_hfov_x( other.tan_hfov_x),
	tan_hfov_y( other.tan_hfov_y),
	screen_objects( z_comparator( forward)),
	glext(other.glext)
{
}

double
view::pixel_coverage( const vector& pos, double radius) const
{
	// The distance from the camera to this position, in the direction of the
	// camera.  This is the distance to the viewing plane that the coverage
	// circle lies in.
	
	double dist = (pos - camera).dot(forward);
	// Half of the width of the viewing plane at this distance.
	double apparent_hwidth = tan_hfov_x * dist;
	// The fraction of the apparent width covered by the coverage circle.
	double coverage_fraction = radius / apparent_hwidth;
	// Convert from fraction to pixels.
	return coverage_fraction * window_width;

}

renderable::renderable()
	: model_damaged(true), z_damaged(true), visible(true), lit(true),
		shininess(0.0)
{
}

renderable::renderable( const renderable& other)
	: model_damaged(true), z_damaged(true), tex(other.tex),
		visible(other.visible), lit(other.lit), shininess(other.shininess)
{
}

renderable::~renderable()
{
}

void 
renderable::outer_render( const view& v ) 
{
	refresh_cache( v );
	
	rgba actual_color = color;
	if (v.anaglyph) {
		if (v.coloranaglyph)
			color = actual_color.desaturate();
		else
			color = actual_color.grayscale();
	}

	tmatrix material_matrix;
	get_material_matrix(v, material_matrix);
	apply_material use_mat( v, mat.get(), material_matrix );
	gl_render(v);

	if (v.anaglyph)
		color = actual_color;
}

void
renderable::gl_render( const view&)
{
	return;
}

void
renderable::gl_pick_render( const view&)
{
}

void
renderable::grow_extent( extent&)
{
	return;
}

// A function that must be overridden if an object wants to cache its state
// for rendering optimization.
void
renderable::update_cache(const view&)
{
	return;
}

// By default, z-sorting is ignored.
void
renderable::update_z_sort( const view&)
{
	return;
}

void
renderable::refresh_cache(const view& geometry)
{
	if (color.opacity != 1.0 && (z_damaged || geometry.forward_changed)) {
		if (model_damaged || geometry.gcf_changed)
			update_cache( geometry);
		else
			update_z_sort( geometry);
		model_damaged = false;
		z_damaged = false;
	}
	else if (model_damaged || geometry.gcf_changed) {
		update_cache( geometry);
		model_damaged = false;
	}
}

void
renderable::set_shininess( const float s)
{
	model_damage();
	shininess = clamp( 0.0f, s, 1.0f);
}

float
renderable::get_shininess()
{
	return shininess;
}

void
renderable::set_lit(bool l)
{
	lit = l;
}

bool
renderable::is_lit()
{
	return lit;
}

void
renderable::set_texture( shared_ptr<texture> t)
{
	tex = t;
}

shared_ptr<texture>
renderable::get_texture()
{
	return tex;
}

void 
renderable::set_material( shared_ptr<class material> m )
{
	model_damage();
	mat = m;
}

shared_ptr<class material> 
renderable::get_material() {
	return mat;
}

bool
renderable::shiny( void)
{
	return !mat && shininess != 0.0;
}

void
renderable::lighting_prepare( void)
{
	if (!lit)
		glDisable( GL_LIGHTING);
}

void
renderable::shiny_prepare( void)
{

	if (shiny()) {

		glLightModeli( GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
		glLightModeli( GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
		int gl_shininess = std::min( 127, static_cast<int>(shininess * 128));
		//AS changes to &color.red from &rgba(.8,.8,.8).red

		glMaterialfv( GL_FRONT, GL_SPECULAR, &color.red);
		glMateriali( GL_FRONT, GL_SHININESS, gl_shininess);
	}
}

void
renderable::shiny_complete( void)
{
	if (shiny()) {
		glLightModeli( GL_LIGHT_MODEL_LOCAL_VIEWER, 0);
		glLightModeli( GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);
		//AS changes to &color.red from &rgba(.8,.8,.8).red

		glMaterialfv( GL_FRONT, GL_SPECULAR, &color.red);
	}
}

void
renderable::lighting_complete( void)
{
	if (!lit)
		glEnable( GL_LIGHTING);
}

} // !namespace cvisual
