#include "box.hpp"
#include "util/errors.hpp"
#include <iostream>

displaylist box::simple_model;
displaylist box::textured_model;
bool box::first = true;


box::box()
	: width(1.0), height(1.0)
{	
}

void
box::set_width( const double& n_width)
{
	width = n_width;
}

void
box::set_length( const double& n_length)
{
	z_damage();
	axis = axis.norm() * n_length;
}

void
box::set_height( const double& n_height)
{
	height = n_height;
}

vector 
box::get_scale() const
{
	return vector( axis.mag(), width, height);
}


void 
box::update_cache( const view&)
{
	if (first) {
		clear_gl_error();
		first = false;
		simple_model.gl_compile_begin();
		glBegin( GL_QUADS);
			// Right face.
			glNormal3f( 1, 0, 0);
			glVertex3f( 0.5, 0.5, 0.5);
			glVertex3f( 0.5, -0.5, 0.5);
			glVertex3f( 0.5, -0.5, -0.5);
			glVertex3f( 0.5, 0.5, -0.5);
			
			// Top face
			glNormal3f( 0, 1, 0);
			glVertex3f( 0.5, 0.5, 0.5);
			glVertex3f( 0.5, 0.5, -0.5);
			glVertex3f( -0.5, 0.5, -0.5);
			glVertex3f( -0.5, 0.5, 0.5);
			
			// Left face
			glNormal3f( -1, 0, 0);
			glVertex3f( -0.5, 0.5, -0.5);
			glVertex3f( -0.5, -0.5, -0.5);
			glVertex3f( -0.5, -0.5, 0.5);
			glVertex3f( -0.5, 0.5, 0.5);
			
			// Bottom face
			glNormal3f( 0, -1, 0);
			glVertex3f( -0.5, -0.5, 0.5);
			glVertex3f( -0.5, -0.5, -0.5);
			glVertex3f( 0.5, -0.5, -0.5);
			glVertex3f( 0.5, -0.5, 0.5);
			
			// Front face
			glNormal3f( 0, 0, 1);
			glVertex3f( 0.5, 0.5, 0.5);
			glVertex3f( -0.5, 0.5, 0.5);
			glVertex3f( -0.5, -0.5, 0.5);
			glVertex3f( 0.5, -0.5, 0.5);
			
			// Back face
			glNormal3f( 0, 0, -1);
			glVertex3f( 0.5, 0.5, -0.5);
			glVertex3f( 0.5, -0.5, -0.5);
			glVertex3f( -0.5, -0.5, -0.5);
			glVertex3f( -0.5, 0.5, -0.5);
		glEnd();
		simple_model.gl_compile_end();
		textured_model.gl_compile_begin();
		// Same geometry, but this time adds texture coordinates.  The texture
		// is mapped directly to each face of the box objects.
		glBegin( GL_QUADS);
			// Right face.
			glNormal3f( 1, 0, 0);
			glTexCoord2f( 0, 1);
			glVertex3f( 0.5, 0.5, 0.5);
			glTexCoord2f( 0, 0);
			glVertex3f( 0.5, -0.5, 0.5);
			glTexCoord2f( 1, 0);
			glVertex3f( 0.5, -0.5, -0.5);
			glTexCoord2f( 1, 1);
			glVertex3f( 0.5, 0.5, -0.5);
			
			// Top face
			glNormal3f( 0, 1, 0);
			glTexCoord2f( 1, 0);
			glVertex3f( 0.5, 0.5, 0.5);
			glTexCoord2f( 1, 1);
			glVertex3f( 0.5, 0.5, -0.5);
			glTexCoord2f( 0, 1);
			glVertex3f( -0.5, 0.5, -0.5);
			glTexCoord2f( 0, 0);
			glVertex3f( -0.5, 0.5, 0.5);
			
			// Left face
			glNormal3f( -1, 0, 0);
			glTexCoord2f( 0, 1);
			glVertex3f( -0.5, 0.5, -0.5);
			glTexCoord2f( 0, 0);
			glVertex3f( -0.5, -0.5, -0.5);
			glTexCoord2f( 1, 0);
			glVertex3f( -0.5, -0.5, 0.5);
			glTexCoord2f( 1, 1);
			glVertex3f( -0.5, 0.5, 0.5);
			
			// Bottom face
			glNormal3f( 0, -1, 0);
			glTexCoord2f( 0, 1);
			glVertex3f( -0.5, -0.5, 0.5);
			glTexCoord2f( 0, 0);
			glVertex3f( -0.5, -0.5, -0.5);
			glTexCoord2f( 1, 0);
			glVertex3f( 0.5, -0.5, -0.5);
			glTexCoord2f( 1, 1);
			glVertex3f( 0.5, -0.5, 0.5);
			
			// Front face
			glNormal3f( 0, 0, 1);
			glTexCoord2f( 1, 1);
			glVertex3f( 0.5, 0.5, 0.5);
			glTexCoord2f(0, 1);
			glVertex3f( -0.5, 0.5, 0.5);
			glTexCoord2f( 0, 0);
			glVertex3f( -0.5, -0.5, 0.5);
			glTexCoord2f( 1, 0);
			glVertex3f( 0.5, -0.5, 0.5);
			
			// Back face
			glNormal3f( 0, 0, -1);
			glTexCoord2f( 0, 1);
			glVertex3f( 0.5, 0.5, -0.5);
			glTexCoord2f( 0, 0);
			glVertex3f( 0.5, -0.5, -0.5);
			glTexCoord2f( 1, 0);
			glVertex3f( -0.5, -0.5, -0.5);
			glTexCoord2f( 1, 1);
			glVertex3f( -0.5, 0.5, -0.5);
		glEnd();
		textured_model.gl_compile_end();
		check_gl_error();
	}
}


void 
box::gl_render( const view& scene)
{
	// std::cout << "box::gl_render()\n";
	// assert( visible);
	if (!visible)
		return;
	
	clear_gl_error();
	color.gl_set();
	{
		double gcf = scene.gcf;
		gl_matrix_stackguard guard;
		vector view_pos = pos * scene.gcf;
		glTranslated( view_pos.x, view_pos.y, view_pos.z);
		model_world_transform().gl_mult();
		glScaled( axis.mag() * gcf, width * gcf, height * gcf);
		
		if (tex) {
			// Render the textured box
			glEnable( GL_TEXTURE_2D);
			tex->gl_activate();
			textured_model.gl_render();
			glDisable( GL_TEXTURE_2D);
		}
		else if (color.alpha < 1.0) {
			// Render the transparent box
			if (!sorted_model) {
				sorted_model.reset( new z_sorted_model<quad, 6>());
				calc_sorted_model();
				vector object_forward = (pos - scene.camera).norm();
				tmatrix inv = world_model_transform();
				vector model_forward = inv.times_v( object_forward).norm();
				sorted_model->sort( model_forward);
			}
			glEnable( GL_BLEND);
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			
			glBegin( GL_QUADS);
			sorted_model->gl_render();
			glEnd();
			glDisable( GL_BLEND);
		}
		else {
			// Render the simple opaque box		
			simple_model.gl_render();
		}
	}
	check_gl_error();
}

void 
box::grow_extent( extent& e)
{
	e.add_point( pos);
	e.add_point( pos + axis);
}

void 
box::update_z_sort( const view& scene)
{
	if (!sorted_model) {
		sorted_model.reset( new z_sorted_model<quad, 6>());
		calc_sorted_model();
	}
	vector object_forward = (pos - scene.camera).norm();
	tmatrix inv = world_model_transform();
	vector model_forward = inv.times_v( object_forward).norm();
	sorted_model->sort( model_forward);
}

void
box::calc_sorted_model()
{	
	// Calculate the sorted model.
	assert( false != sorted_model);
	sorted_model->faces[0] = quad( // Right face
		vector( 0.5, 0.5, 0.5), vector( 0.5, -0.5, 0.5),
		vector( 0.5, -0.5, -0.5), vector( 0.5, 0.5, -0.5));
	sorted_model->faces[1] = quad( // Top face
		vector( 0.5, 0.5, 0.5), vector( 0.5, 0.5, -0.5),
		vector( -0.5, 0.5, -0.5), vector( -0.5, 0.5, 0.5));
	sorted_model->faces[2] = quad( // Left face
		vector( -0.5, 0.5, -0.5), vector( -0.5, -0.5, -0.5),
		vector( -0.5, -0.5, 0.5), vector( -0.5, 0.5, 0.5));
	sorted_model->faces[3] = quad( // Bottom face
		vector( -0.5, -0.5, 0.5), vector( -0.5, -0.5, -0.5),
		vector( 0.5, -0.5, -0.5), vector( 0.5, -0.5, 0.5));
	sorted_model->faces[4] = quad( // Front face
		vector( 0.5, 0.5, 0.5), vector( -0.5, 0.5, 0.5),
		vector( -0.5, -0.5, 0.5), vector( 0.5, -0.5, 0.5));
	sorted_model->faces[5] = quad( // Back face
		vector( 0.5, 0.5, -0.5), vector( 0.5, -0.5, -0.5),
		vector( -0.5, -0.5, -0.5), vector( -0.5, 0.5, -0.5));
}
