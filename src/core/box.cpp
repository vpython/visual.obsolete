// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "box.hpp"
#include "util/errors.hpp"

namespace cvisual {

displaylist box::simple_model;
displaylist box::textured_model;
z_sorted_model<quad, 6> box::sorted_model;
z_sorted_model<tquad, 6> box::textured_sorted_model;
bool box::first = true;

bool
box::degenerate()
{
	int num_equal_to_zero = 0;
	if (axis.mag() == 0.0)
		num_equal_to_zero++;
	if (width == 0.0)
		num_equal_to_zero++;
	if (height == 0.0)
		num_equal_to_zero++;
	return num_equal_to_zero > 1;
}

box::box()
{	
}

box::box( const box& other)
	: rectangular( other), tex( other.tex)
{
}

box::~box()
{
}

void
box::set_texture( shared_ptr<texture> t)
{
	lock L(mtx);
	tex = t;
}

shared_ptr<texture>
box::get_texture()
{
	return tex;
}

void 
box::gl_pick_render( const view& scene)
{
	if (degenerate())
		return;
	double gcf = scene.gcf;
	gl_matrix_stackguard guard;
	vector view_pos = pos * scene.gcf;
	glTranslated( view_pos.x, view_pos.y, view_pos.z);
	model_world_transform().gl_mult();
	glScaled( axis.mag() * gcf, height * gcf, width * gcf);
	simple_model.gl_render();
}

void 
box::update_cache( const view&)
{
	if (first) {
		glPolygonOffset( 0, 2);
		clear_gl_error();
		first = false;
		// First set up the non-textured model.
		calc_sorted_model();
		simple_model.gl_compile_begin();
		glBegin( GL_QUADS);
			sorted_model.gl_render();
		glEnd();
		simple_model.gl_compile_end();

		// Same geometry, but this time adds texture coordinates.  The texture
		// is mapped directly to each face of the box objects.
		calc_textured_sorted_model();
		textured_model.gl_compile_begin();
		glBegin( GL_QUADS);
		textured_sorted_model.gl_render();
		glEnd();
		textured_model.gl_compile_end();
		
		check_gl_error();
	}
}


void 
box::gl_render( const view& scene)
{
	if (degenerate())
		return;
	bool flat = axis.mag() == 0.0 || width == 0.0 || height == 0.0;
	
	clear_gl_error();
	lighting_prepare();
	shiny_prepare();
	color.gl_set();
	{
		if (flat)
			glEnable( GL_POLYGON_OFFSET_FILL);
		double gcf = scene.gcf;
		gl_matrix_stackguard guard;
		vector view_pos = pos * scene.gcf;
		glTranslated( view_pos.x, view_pos.y, view_pos.z);
		model_world_transform().gl_mult();
		glScaled( axis.mag() * gcf, height * gcf, width*gcf);
		
		if (tex && color.alpha != 1.0) {
			// Render the textured and transparent box.
			vector object_forward = (pos - scene.camera).norm();
			tmatrix inv = world_model_transform();
			vector model_forward = inv.times_v( object_forward).norm();
			textured_sorted_model.sort( model_forward);
			
			glEnable( GL_BLEND);
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable( GL_TEXTURE_2D);
			tex->gl_activate();
			
			glBegin( GL_QUADS);
			textured_sorted_model.gl_render();
			glEnd();
			
			glDisable( GL_TEXTURE_2D);
			glDisable( GL_BLEND);
		}
		else if (tex) {
			// Render the textured box
			glEnable( GL_TEXTURE_2D);
			tex->gl_activate();
			textured_model.gl_render();
			glDisable( GL_TEXTURE_2D);
		}
		else if (color.alpha < 1.0) {
			// Render the transparent box
			vector object_forward = (pos - scene.camera).norm();
			tmatrix inv = world_model_transform();
			vector model_forward = inv.times_v( object_forward).norm();
			sorted_model.sort( model_forward);

			glEnable( GL_BLEND);
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			
			glBegin( GL_QUADS);
			sorted_model.gl_render();
			glEnd();
			glDisable( GL_BLEND);
		}
		else {
			// Render the simple opaque box		
			simple_model.gl_render();
		}
		if (flat)
			glDisable( GL_POLYGON_OFFSET_FILL);
	}
	shiny_prepare();
	lighting_prepare();
	check_gl_error();
}

void 
box::grow_extent( extent& e)
{
    e.add_sphere( pos, 
        1.415*std::max( height*0.5, std::max(axis.mag()*0.5, width*0.5)));
	e.add_body();
}

void
box::calc_sorted_model()
{	
	// Calculate the sorted model.
	sorted_model.faces[0] = quad( // Right face
		vector( 0.5, 0.5, 0.5), vector( 0.5, -0.5, 0.5),
		vector( 0.5, -0.5, -0.5), vector( 0.5, 0.5, -0.5));
	sorted_model.faces[1] = quad( // Top face
		vector( 0.5, 0.5, 0.5), vector( 0.5, 0.5, -0.5),
		vector( -0.5, 0.5, -0.5), vector( -0.5, 0.5, 0.5));
	sorted_model.faces[2] = quad( // Left face
		vector( -0.5, 0.5, -0.5), vector( -0.5, -0.5, -0.5),
		vector( -0.5, -0.5, 0.5), vector( -0.5, 0.5, 0.5));
	sorted_model.faces[3] = quad( // Bottom face
		vector( -0.5, -0.5, 0.5), vector( -0.5, -0.5, -0.5),
		vector( 0.5, -0.5, -0.5), vector( 0.5, -0.5, 0.5));
	sorted_model.faces[4] = quad( // Front face
		vector( 0.5, 0.5, 0.5), vector( -0.5, 0.5, 0.5),
		vector( -0.5, -0.5, 0.5), vector( 0.5, -0.5, 0.5));
	sorted_model.faces[5] = quad( // Back face
		vector( 0.5, 0.5, -0.5), vector( 0.5, -0.5, -0.5),
		vector( -0.5, -0.5, -0.5), vector( -0.5, 0.5, -0.5));
}

void
box::calc_textured_sorted_model()
{
	// Calculate the textured, sorted model.
	textured_sorted_model.faces[0] = tquad( // Right face
		vector( 0.5, 0.5, 0.5), tcoord( 0, 1),
		vector( 0.5, -0.5, 0.5), tcoord( 0, 0),
		vector( 0.5, -0.5, -0.5), tcoord( 1, 0),
		vector( 0.5, 0.5, -0.5), tcoord( 1, 1));
	textured_sorted_model.faces[1] = tquad( // Top face
		vector( 0.5, 0.5, 0.5), tcoord( 1, 0),
		vector( 0.5, 0.5, -0.5), tcoord( 1, 1),
		vector( -0.5, 0.5, -0.5), tcoord( 0, 1),
		vector( -0.5, 0.5, 0.5), tcoord( 0, 0));
	textured_sorted_model.faces[2] = tquad( // Left face
		vector( -0.5, 0.5, -0.5), tcoord( 0, 1),
		vector( -0.5, -0.5, -0.5), tcoord( 0, 0),
		vector( -0.5, -0.5, 0.5), tcoord( 1, 0), 
		vector( -0.5, 0.5, 0.5), tcoord( 1, 1));
	textured_sorted_model.faces[3] = tquad( // Bottom face
		vector( -0.5, -0.5, 0.5), tcoord( 0, 1),
		vector( -0.5, -0.5, -0.5), tcoord( 0, 0),
		vector( 0.5, -0.5, -0.5), tcoord( 1, 0),
		vector( 0.5, -0.5, 0.5), tcoord( 1, 1));
	textured_sorted_model.faces[4] = tquad( // Front face
		vector( 0.5, 0.5, 0.5), tcoord( 1, 1),
		vector( -0.5, 0.5, 0.5), tcoord( 0, 1),
		vector( -0.5, -0.5, 0.5), tcoord( 0, 0), 
		vector( 0.5, -0.5, 0.5), tcoord( 1, 0));
	textured_sorted_model.faces[5] = tquad( // Back face
		vector( 0.5, 0.5, -0.5), tcoord( 0, 1),
		vector( 0.5, -0.5, -0.5), tcoord( 0, 0),
		vector( -0.5, -0.5, -0.5), tcoord( 1, 0), 
		vector( -0.5, 0.5, -0.5), tcoord( 1, 1));
}

PRIMITIVE_TYPEINFO_IMPL(box)

} // !namespace cvisual
