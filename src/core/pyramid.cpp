// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "pyramid.hpp"
#include "util/errors.hpp"
#include "util/gl_enable.hpp"

namespace cvisual {

displaylist pyramid::simple_model;
scoped_ptr< z_sorted_model<triangle, 6> > pyramid::sorted_model;

PRIMITIVE_TYPEINFO_IMPL(pyramid)

bool
pyramid::degenerate()
{
	return axis.mag() == 0.0 || width == 0.0 || height == 0.0;
}

pyramid::pyramid()
{
}

pyramid::pyramid( const pyramid& other)
	: rectangular( other)
{
}

pyramid::~pyramid()
{
}

void 
pyramid::gl_pick_render( const view& scene)
{
	if (degenerate())
		return;
	if (!simple_model)
		update_cache( scene);
	clear_gl_error();
	vector view_pos = pos * scene.gcf;
	gl_matrix_stackguard guard;
	glTranslated( view_pos.x, view_pos.y, view_pos.z);
	model_world_transform().gl_mult();
	glScaled( axis.mag() * scene.gcf, height * scene.gcf, width * scene.gcf);
	simple_model.gl_render();
	check_gl_error();
}

void 
pyramid::update_cache( const view&)
{
	// This object follows a very simple caching strategy.  A displaylist is
	// used to store the model that doesn't use transparancy.  A single
	// z_sorted_model is initialized when the first transparent object needs
	// it.  Transparent pyramids unconditionally resort thier models prior
	// to rendering them.  Since they only take a few usec to do so, it isn't
	// a bottleneck.
	if (opacity != 1.0 && !sorted_model) {
		// Initialize the sortable model
		vector corners[4] = {
			vector(0, 0.5, 0.5),
			vector(0, -0.5, 0.5),
			vector(0, -0.5, -0.5),
			vector(0, 0.5, -0.5)
		};
		vector tip( 1, 0, 0);
		sorted_model.reset( new z_sorted_model<triangle, 6>());
		sorted_model->faces[0] = triangle( corners[3], corners[0], tip);
		sorted_model->faces[1] = triangle( corners[1], corners[2], tip);
		sorted_model->faces[2] = triangle( corners[0], corners[1], tip);
		sorted_model->faces[3] = triangle( corners[2], corners[3], tip);
		sorted_model->faces[4] = triangle( corners[0], corners[3], corners[2]);
		sorted_model->faces[4].center = vector();
		sorted_model->faces[5] = triangle( corners[0], corners[2], corners[1]);
		sorted_model->faces[5].center = vector();
	}
	if (!simple_model) {
		// Initialize the simple model
		vector corners[4] = {
			vector(0, 0.5, 0.5),
			vector(0, -0.5, 0.5),
			vector(0, -0.5, -0.5),
			vector(0, 0.5, -0.5)
		};
		vector tip( 1, 0, 0);
		
		clear_gl_error();
		simple_model.gl_compile_begin();
		glBegin( GL_TRIANGLES);
			// Top triangle first.
			vector(1, 2).norm().gl_normal();
			corners[3].gl_render();
			corners[0].gl_render();
			tip.gl_render();
			// Bottom triangle
			vector(1, -2).norm().gl_normal();
			corners[1].gl_render();
			corners[2].gl_render();
			tip.gl_render();
			// Front triangle
			vector(1, 0, 2).norm().gl_normal();
			corners[0].gl_render();
			corners[1].gl_render();
			tip.gl_render();
			// Back triangle
			vector( 1, 0, -2).norm().gl_normal();
			corners[3].gl_render();
			tip.gl_render();
			corners[2].gl_render();
			// Left pair of triangles
			vector(-1).gl_normal();
			corners[0].gl_render();
			corners[3].gl_render();
			corners[2].gl_render();
			corners[0].gl_render();
			corners[2].gl_render();
			corners[1].gl_render();
		glEnd();
		simple_model.gl_compile_end();
		check_gl_error();
	}
}

void 
pyramid::gl_render( const view& scene)
{
	if (degenerate())
		return;
	clear_gl_error();
	lighting_prepare();
	shiny_prepare();
	{
		vector view_pos = pos * scene.gcf;
		gl_matrix_stackguard guard;
		glTranslated( view_pos.x, view_pos.y, view_pos.z);
		tmatrix mwt = model_world_transform();
		mwt.gl_mult();
		glScaled( axis.mag() * scene.gcf, height * scene.gcf, width * scene.gcf);
		
		color.gl_set();
		if (opacity != 1.0) {
			vector forward = mwt.times_inv((pos - scene.camera)).norm();
			sorted_model->sort( forward);
			gl_enable blend( GL_BLEND);
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBegin( GL_TRIANGLES);
				sorted_model->gl_render();
			glEnd();
		}
		else
			simple_model.gl_render();
	}
	shiny_complete();
	lighting_complete();
	check_gl_error();
}

void 
pyramid::grow_extent( extent& world_extent)
{
	if (degenerate())
		return;
	tmatrix orient = model_world_transform();
	vector vwidth = orient * vector( 0, 0, width * 0.5);
	vector vheight = orient * vector( 0, height * 0.5, 0);
	world_extent.add_point( pos + axis);
	world_extent.add_point( pos + vwidth + vheight);
	world_extent.add_point( pos - vwidth + vheight);
	world_extent.add_point( pos + vwidth - vheight);
	world_extent.add_point( pos - vwidth - vheight);
	world_extent.add_body();
}

vector
pyramid::get_center() const
{
	return pos + axis * 0.33333333333333;
}

void 
pyramid::get_material_matrix( const view&, tmatrix& out )
{
	out.translate( vector(0,.5,.5) );
	vector scale( axis.mag(), height, width );
	out.scale( scale * (1.0 / std::max(scale.x, std::max(scale.y, scale.z))) );
}

} // !namespace cvisual
