// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "box.hpp"
#include "util/errors.hpp"
#include "util/gl_enable.hpp"

namespace cvisual {

displaylist box::lod_cache[6];
displaylist box::lod_textured_cache[6];

// Models to be used for rendering opaque objects.
z_sorted_model<quad, 6*box_L0*box_L0> box::simple_model_0;
z_sorted_model<quad, 6*box_L1*box_L1> box::simple_model_1;
z_sorted_model<quad, 6*box_L2*box_L2> box::simple_model_2;
z_sorted_model<quad, 6*box_L3*box_L3> box::simple_model_3;
z_sorted_model<quad, 6*box_L4*box_L4> box::simple_model_4;
z_sorted_model<quad, 6*box_L5*box_L5> box::simple_model_5;
// Models to be used for transparent and textured objects.
z_sorted_model<tquad, 6*box_L0*box_L0> box::textured_model_0;
z_sorted_model<tquad, 6*box_L1*box_L1> box::textured_model_1;
z_sorted_model<tquad, 6*box_L2*box_L2> box::textured_model_2;
z_sorted_model<tquad, 6*box_L3*box_L3> box::textured_model_3;
z_sorted_model<tquad, 6*box_L4*box_L4> box::textured_model_4;
z_sorted_model<tquad, 6*box_L5*box_L5> box::textured_model_5;

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
	lod_cache[0].gl_render();
}

void 
box::update_cache( const view&)
{
	if (first) {
		// glPolygonOffset( 0, 2);
		clear_gl_error();
		first = false;
		
        for(size_t j = 0; j < 6; j++) {
        	int d; // each face of box divided into d x d regions
			if (j == 0) { 
				d = box_L0;
				calc_simple_model(simple_model_0.faces, d);
				calc_textured_model(textured_model_0.faces, d);  
			}
			else if (j == 1) { 
				d = box_L1;
				calc_simple_model(simple_model_1.faces, d);
				calc_textured_model(textured_model_1.faces, d); 
			}
			else if (j == 2) {  
				d = box_L2;
				calc_simple_model(simple_model_2.faces, d);
				calc_textured_model(textured_model_2.faces, d);  
			}
			else if (j == 3) {  
				d = box_L3;
				calc_simple_model(simple_model_3.faces, d);
				calc_textured_model(textured_model_3.faces, d);   
			}
			else if (j == 4) {
				d = box_L4;
				calc_simple_model(simple_model_4.faces, d);
				calc_textured_model(textured_model_4.faces, d);    
			}
			else if (j == 5) {  
				d = box_L5;
				calc_simple_model(simple_model_5.faces, d);
				calc_textured_model(textured_model_5.faces, d);   
			}
        	// First set up the non-textured model.
			lod_cache[j].gl_compile_begin();
			glBegin( GL_QUADS);
			if (j == 0) simple_model_0.gl_render();
			else if (j == 1) simple_model_1.gl_render();
			else if (j == 2) simple_model_2.gl_render();
			else if (j == 3) simple_model_3.gl_render();
			else if (j == 4) simple_model_4.gl_render();
			else if (j == 5) simple_model_5.gl_render();
			glEnd();
			lod_cache[j].gl_compile_end();
			
			// Same geometry, but this time adds texture coordinates. The
			// texture is mapped directly to each face of the box objects.
			lod_textured_cache[j].gl_compile_begin();
			glBegin( GL_QUADS);
			if (j == 0) textured_model_0.gl_render();
			else if (j == 1) textured_model_1.gl_render();
			else if (j == 2) textured_model_2.gl_render();
			else if (j == 3) textured_model_3.gl_render();
			else if (j == 4) textured_model_4.gl_render();
			else if (j == 5) textured_model_5.gl_render();
			glEnd();
			lod_textured_cache[j].gl_compile_end();
		}
		
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
		
		// coverage is number of pixels corresponding to size of box
		double coverage = scene.pixel_coverage( pos, 500*size);
		int lod = 0;
		if (!mat && shininess) {
			// "Level of detail" for boxes is needed for specular vertex lighitng
			if (coverage < 0) lod = 5;
			else if (coverage < 10) lod = 0;
			else if (coverage < 25) lod = 1;
			else if (coverage < 100) lod = 2;
			else if (coverage < 200) lod = 3;
			else if (coverage < 600) lod = 4;
			else lod = 5;
		}
		
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
			
			if (lod == 0) textured_model_0.sort( model_forward);
			else if (lod == 1) textured_model_1.sort( model_forward);
			else if (lod == 2) textured_model_2.sort( model_forward);
			else if (lod == 3) textured_model_3.sort( model_forward);
			else if (lod == 4) textured_model_4.sort( model_forward);
			else if (lod == 5) textured_model_5.sort( model_forward);
			gl_enable blend( GL_BLEND);
			gl_enable tex2D( tex->enable_type() );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			tex->gl_activate(scene);
			
			glBegin( GL_QUADS);
			if (lod == 0) textured_model_0.gl_render();
			else if (lod == 1) textured_model_1.gl_render();
			else if (lod == 2) textured_model_2.gl_render();
			else if (lod == 3) textured_model_3.gl_render();
			else if (lod == 4) textured_model_4.gl_render();
			else if (lod == 5) textured_model_5.gl_render();
			glEnd();
		}
		else if (tex) {
			// Render the textured box
			gl_enable tex2D( tex->enable_type() );
			tex->gl_activate(scene);
			lod_textured_cache[lod].gl_render();
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
			if (lod == 0) simple_model_0.sort( model_forward);
			else if (lod == 1) simple_model_1.sort( model_forward);
			else if (lod == 2) simple_model_2.sort( model_forward);
			else if (lod == 3) simple_model_3.sort( model_forward);
			else if (lod == 4) simple_model_4.sort( model_forward);
			else if (lod == 5) simple_model_5.sort( model_forward);
			
			gl_enable blend( GL_BLEND);
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			
			glBegin( GL_QUADS);
			if (lod == 0) simple_model_0.gl_render();
			else if (lod == 1) simple_model_1.gl_render();
			else if (lod == 2) simple_model_2.gl_render();
			else if (lod == 3) simple_model_3.gl_render();
			else if (lod == 4) simple_model_4.gl_render();
			else if (lod == 5) simple_model_5.gl_render();
			glEnd();
		}
		else {
			// Render the simple opaque box		
			lod_cache[lod].gl_render();
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
box::calc_simple_model(quad *faces, int level)
{
	// Calculate the non-textured, sorted model.
	// There are level*level squares on each of the 6 sides of the box.
	double spacing = 1.0/level;
	for(size_t i = 0; i < level; i++)
	{
		for(size_t j = 0; j < level; j++)
		{
			faces[0*level*level+(i*level+j)] = quad( // Right face
				vector( 0.5, -0.5+(j+1)*spacing, -0.5+(i+1)*spacing),
				vector( 0.5, -0.5+    j*spacing, -0.5+(i+1)*spacing),
				vector( 0.5, -0.5+    j*spacing, -0.5+    i*spacing),
				vector( 0.5, -0.5+(j+1)*spacing, -0.5+    i*spacing));
			faces[1*level*level+(i*level+j)] = quad( // Left face
				vector( -0.5, -0.5+(j+1)*spacing, -0.5+    i*spacing),
				vector( -0.5, -0.5+    j*spacing, -0.5+    i*spacing),
				vector( -0.5, -0.5+    j*spacing, -0.5+(i+1)*spacing),
				vector( -0.5, -0.5+(j+1)*spacing, -0.5+(i+1)*spacing));
			faces[2*level*level+(i*level+j)] = quad( // Bottom face
				vector( -0.5+    j*spacing, -0.5, -0.5+(i+1)*spacing),
				vector( -0.5+    j*spacing, -0.5, -0.5+    i*spacing),
				vector( -0.5+(j+1)*spacing, -0.5, -0.5+    i*spacing),
				vector( -0.5+(j+1)*spacing, -0.5, -0.5+(i+1)*spacing));
			faces[3*level*level+(i*level+j)] = quad( // Front face
				vector( -0.5+(j+1)*spacing, -0.5+(i+1)*spacing, 0.5),
				vector( -0.5+    j*spacing, -0.5+(i+1)*spacing, 0.5),
				vector( -0.5+    j*spacing, -0.5+    i*spacing, 0.5),
				vector( -0.5+(j+1)*spacing, -0.5+    i*spacing, 0.5));
			faces[4*level*level+(i*level+j)] = quad( // Back face
				vector( -0.5+(j+1)*spacing, -0.5+(i+1)*spacing, -0.5),
				vector( -0.5+(j+1)*spacing, -0.5+    i*spacing, -0.5),
				vector( -0.5+    j*spacing, -0.5+    i*spacing, -0.5),
				vector( -0.5+    j*spacing, -0.5+(i+1)*spacing, -0.5));
			faces[5*level*level+(i*level+j)] = quad( // Top face
				vector( -0.5+(j+1)*spacing, 0.5, 0.5-    i*spacing),
				vector( -0.5+(j+1)*spacing, 0.5, 0.5-(i+1)*spacing),
				vector( -0.5+    j*spacing, 0.5, 0.5-(i+1)*spacing),
				vector( -0.5+    j*spacing, 0.5, 0.5-    i*spacing));
		}
	}
}

void
box::calc_textured_model(tquad *faces, int level)
{
	// Calculate the textured, sorted model.
	// There are level*level squares on each of the 6 sides of the box.
	double spacing = 1.0/level;
	for(size_t i = 0; i < level; i++)
	{
		for(size_t j = 0; j < level; j++)
		{
			faces[0*level*level+(i*level+j)] = tquad( // Right face
				vector( 0.5, -0.5+(j+1)*spacing, -0.5+(i+1)*spacing), tcoord((j+1)*spacing, (i+1)*spacing),
				vector( 0.5, -0.5+    j*spacing, -0.5+(i+1)*spacing), tcoord(    j*spacing, (i+1)*spacing),
				vector( 0.5, -0.5+    j*spacing, -0.5+    i*spacing), tcoord(    j*spacing,     i*spacing),
				vector( 0.5, -0.5+(j+1)*spacing, -0.5+    i*spacing), tcoord((j+1)*spacing,     i*spacing));
			faces[1*level*level+(i*level+j)] = tquad( // Left face
				vector( -0.5, -0.5+(j+1)*spacing, -0.5+    i*spacing), tcoord((j+1)*spacing,     i*spacing),
				vector( -0.5, -0.5+    j*spacing, -0.5+    i*spacing), tcoord(    j*spacing,     i*spacing),
				vector( -0.5, -0.5+    j*spacing, -0.5+(i+1)*spacing), tcoord(    j*spacing, (i+1)*spacing),
				vector( -0.5, -0.5+(j+1)*spacing, -0.5+(i+1)*spacing), tcoord((j+1)*spacing, (i+1)*spacing));
			faces[2*level*level+(i*level+j)] = tquad( // Bottom face
				vector( -0.5+    j*spacing, -0.5, -0.5+(i+1)*spacing), tcoord(    j*spacing, (i+1)*spacing),
				vector( -0.5+    j*spacing, -0.5, -0.5+    i*spacing), tcoord(    j*spacing,     i*spacing),
				vector( -0.5+(j+1)*spacing, -0.5, -0.5+    i*spacing), tcoord((j+1)*spacing,     i*spacing),
				vector( -0.5+(j+1)*spacing, -0.5, -0.5+(i+1)*spacing), tcoord((j+1)*spacing, (i+1)*spacing));
			faces[3*level*level+(i*level+j)] = tquad( // Front face
				vector( -0.5+(j+1)*spacing, -0.5+(i+1)*spacing, 0.5), tcoord((j+1)*spacing, (i+1)*spacing),
				vector( -0.5+    j*spacing, -0.5+(i+1)*spacing, 0.5), tcoord(    j*spacing, (i+1)*spacing),
				vector( -0.5+    j*spacing, -0.5+    i*spacing, 0.5), tcoord(    j*spacing,     i*spacing),
				vector( -0.5+(j+1)*spacing, -0.5+    i*spacing, 0.5), tcoord((j+1)*spacing,     i*spacing));
			faces[4*level*level+(i*level+j)] = tquad( // Back face
				vector( -0.5+(j+1)*spacing, -0.5+(i+1)*spacing, -0.5), tcoord((j+1)*spacing, (i+1)*spacing),
				vector( -0.5+(j+1)*spacing, -0.5+    i*spacing, -0.5), tcoord((j+1)*spacing,     i*spacing),
				vector( -0.5+    j*spacing, -0.5+    i*spacing, -0.5), tcoord(    j*spacing,     i*spacing),
				vector( -0.5+    j*spacing, -0.5+(i+1)*spacing, -0.5), tcoord(    j*spacing, (i+1)*spacing));
			faces[5*level*level+(i*level+j)] = tquad( // Top face
				vector( -0.5+(j+1)*spacing, 0.5, 0.5-    i*spacing), tcoord((j+1)*spacing,     i*spacing),
				vector( -0.5+(j+1)*spacing, 0.5, 0.5-(i+1)*spacing), tcoord((j+1)*spacing, (i+1)*spacing),
				vector( -0.5+    j*spacing, 0.5, 0.5-(i+1)*spacing), tcoord(    j*spacing, (i+1)*spacing),
				vector( -0.5+    j*spacing, 0.5, 0.5-    i*spacing), tcoord(    j*spacing,     i*spacing));
		}
	}
}

void
box::get_material_matrix(const view&, tmatrix& out) { 
	out.translate( vector(.5,.5,.5) );
	vector scale( axis.mag(), height, width );
	out.scale( scale * (1.0 / std::max(scale.x, std::max(scale.y, scale.z))) );
}

PRIMITIVE_TYPEINFO_IMPL(box)

} // !namespace cvisual
