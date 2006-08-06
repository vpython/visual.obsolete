// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "box.hpp"
#include "util/errors.hpp"
#include "util/gl_enable.hpp"

namespace cvisual {

displaylist box::simple_model;
displaylist box::textured_model;
z_sorted_model<quad, 6> box::sorted_model;
z_sorted_model<tquad, 6> box::textured_sorted_model;
bool box::first = true;

bool
box::degenerate()
{
	double epsilon = vector(axis.mag(), width, height).mag();
	epsilon *= 0.001;
	int num_equal_to_zero = 0;
	if (axis.mag() < epsilon)
		num_equal_to_zero++;
	if (width < epsilon)
		num_equal_to_zero++;
	if (height < epsilon)
		num_equal_to_zero++;
	return num_equal_to_zero > 1;
}

box::box()
{	
}

box::box( const box& other)
	: rectangular( other)
{
}

box::~box()
{
}

void 
box::gl_pick_render( const view& scene)
{
	if (degenerate())
		return;
	if (first)
		update_cache( scene);
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
		// glPolygonOffset( 0, 2);
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
	double saved_height = height;
	double saved_width = width;
	double saved_length = axis.mag();
	
	double size = vector(width, axis.mag(), height).mag();
	size *= 0.002; // 1/500 th of its size, or about 1 pixel
	if (std::fabs(width) < size)
		width = size;
	if (std::fabs(height) < size)
		height = size;
	if (std::fabs(axis.mag()) < size)
		axis.set_mag( size);

	clear_gl_error();
	lighting_prepare();
	shiny_prepare();
	color.gl_set();
	{
		double gcf = scene.gcf;
		gl_matrix_stackguard guard;
		vector view_pos = pos * scene.gcf;
		glTranslated( view_pos.x, view_pos.y, view_pos.z);
		model_world_transform().gl_mult();
		glScaled( axis.mag() * gcf, height * gcf, width*gcf);
		
		if (tex && (color.opacity < 1.0 || tex->has_opacity())) {
			// Render the textured and transparent box.
			vector object_forward = (pos - scene.camera).norm();
			tmatrix inv = world_model_transform();
			vector model_forward = inv.times_v( object_forward).norm();
			if (axis.mag() < 0)
				model_forward.x *= -1;
			if (height < 0)
				model_forward.y *= -1;
			if (width < 0)
				model_forward.z *= -1;
			textured_sorted_model.sort( model_forward);
			
			gl_enable blend( GL_BLEND);
			gl_enable tex2d( GL_TEXTURE_2D);
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			tex->gl_activate();
			
			glBegin( GL_QUADS);
			textured_sorted_model.gl_render();
			glEnd();
		}
		else if (tex) {
			// Render the textured box
			gl_enable tex2D( GL_TEXTURE_2D);
			tex->gl_activate();
			textured_model.gl_render();
		}
		else if (color.opacity < 1.0) {
			// Render the transparent box
			vector object_forward = (pos - scene.camera).norm();
			tmatrix inv = world_model_transform();
			vector model_forward = inv.times_v( object_forward).norm();
			if (axis.mag() < 0)
				model_forward.x *= -1;
			if (height < 0)
				model_forward.y *= -1;
			if (width < 0)
				model_forward.z *= -1;
			sorted_model.sort( model_forward);

			gl_enable blend( GL_BLEND);
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			
			glBegin( GL_QUADS);
			sorted_model.gl_render();
			glEnd();
		}
		else {
			// Render the simple opaque box		
			simple_model.gl_render();
		}
	}
	shiny_complete();
	lighting_complete();
	check_gl_error();
	axis.set_mag( saved_length);
	width = saved_width;
	height = saved_height;
}

void 
box::grow_extent( extent& e)
{
    double dx = axis.mag();
    double dy = height;
    double dz = width;
    vector pos0 = pos-0.5*vector(dx,dy,dz);
    for (int i=0; i<2; i++) {
    	for (int j=0; j<2; j++) {
    		for (int k=0; k<2; k++) {
    			e.add_point(pos0+vector(i*dx,j*dy,k*dz));
    		}
    	}
    }
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
