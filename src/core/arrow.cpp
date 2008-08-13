// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "arrow.hpp"
#include "util/errors.hpp"
#include "util/gl_enable.hpp"
#include "box.hpp"
#include "pyramid.hpp"

namespace cvisual {
	
bool
arrow::degenerate()
{
	return axis.mag() == 0.0;
}

arrow::arrow()
	: fixedwidth(false), headwidth(0), headlength(0), shaftwidth(0)
{
}

arrow::arrow( const arrow& other)
	: primitive(other), fixedwidth( other.fixedwidth), 
	headwidth( other.headwidth), headlength( other.headlength), 
	shaftwidth( other.shaftwidth)
{	
}

arrow::~arrow()
{
}

void
arrow::set_headwidth( double hw)
{
	headwidth = hw;
}

double
arrow::get_headwidth()
{
	return headwidth;
}
	
void
arrow::set_headlength( double hl)
{
	headlength = hl;
}

double
arrow::get_headlength()
{
	return headlength;
}
	
void
arrow::set_shaftwidth( double sw)
{
	shaftwidth = sw;
}

double
arrow::get_shaftwidth()
{
	return shaftwidth;
}
	
void
arrow::set_fixedwidth( bool fixed)
{
	fixedwidth = fixed;
}

bool
arrow::is_fixedwidth()
{
	return fixedwidth;
}
	
void
arrow::set_length( double l)
{
	axis = axis.norm() * l;
}

double
arrow::get_length()
{
	return axis.mag();
}

vector
arrow::get_center() const
{
	return (pos + axis)/2.0;
}

void 
arrow::gl_pick_render( const view& scene)
{
	gl_render(scene);
}

void
arrow::gl_render( const view& scene)
{
	if (degenerate()) return;

	init_model();

	color.gl_set(opacity);

	double hl,hw,len,sw;
	effective_geometry( hw, sw, len, hl, 1.0 );

	// Render the shaft and the head in back to front order (the shaft is in front
	// of the head if axis points away from the camera)
	int shaft = axis.dot( scene.camera - (pos + axis * (1-hl/len)) ) < 0;
	for(int part=0; part<2; part++) {
		gl_matrix_stackguard guard;
		model_world_transform( scene.gcf ).gl_mult();
		if (part == shaft) {
			glScaled( len - hl, sw, sw );
			glTranslated( 0.5, 0, 0 );
			shaft_model.gl_render();
		} else {
			glTranslated( len - hl, 0, 0 );
			glScaled( hl, hw, hw );
			pyramid::model.gl_render();
		}
	}
}

void
arrow::grow_extent( extent& world)
{
	if (degenerate())
		return;
#if 1
	// Hmm.  A more accurate, but much slower calc of the extent of this body.
	double hl;
	double hw;
	double len;
	double sw;
	effective_geometry( hw, sw, len, hl, 1.0);
	// A sphere large enough to contain all four corners of the head.
	world.add_sphere( pos + axis.norm()*(len-hl), hw * 1.414213562);
	// A sphere large enough to contain all four corners of the base.
	world.add_sphere( pos, sw * 1.414213562);
#else
	world.add_point( pos);
#endif
	// The tip.
	world.add_point( pos + axis);
	world.add_body();
}

void
arrow::get_material_matrix(const view& v, tmatrix& out)
{
	double hl, hw, len, sw;
	effective_geometry( hw, sw, len, hl, v.gcf);

	out.translate( vector(0,.5,.5) );
	out.scale( vector(1,1,1) * (1 / std::max( axis.mag()*v.gcf, hw )) );
}

void arrow::init_model()
{
	if (!shaft_model) box::init_model(shaft_model, true);
	if (!pyramid::model) pyramid::init_model();
}

PRIMITIVE_TYPEINFO_IMPL(arrow)

void
arrow::effective_geometry( 
	double& eff_headwidth, double& eff_shaftwidth, double& eff_length, 
	double& eff_headlength, double gcf)
{
	// First calculate the actual geometry based on the specs for headwidth,
	// shaftwidth, shaftlength, and fixedwidth.  This geometry is calculated
	// in world space and multiplied
	static const double min_sw = 0.02; // minimum shaftwidth
	static const double def_sw = 0.1; // default shaftwidth
	static const double def_hw = 2.0; // default headwidth multiplier. (x shaftwidth)
	static const double def_hl = 3.0; // default headlength multiplier. (x shaftwidth)
	// maximum fraction of the total arrow length allocated to the head.
	static const double max_headlength = 0.5;
	
	eff_length = axis.mag() * gcf;
	if (shaftwidth)
		eff_shaftwidth = shaftwidth * gcf;
	else
		eff_shaftwidth = eff_length * def_sw;

	if (headwidth)
		eff_headwidth = headwidth * gcf;
	else
		eff_headwidth = eff_shaftwidth * def_hw;
	
	if (headlength)
		eff_headlength = headlength * gcf;
	else
		eff_headlength = eff_shaftwidth * def_hl;
	
	if (fixedwidth) {
		if (eff_headlength > max_headlength * eff_length)
			eff_headlength = max_headlength * eff_length;
	}
	else {
		if (eff_shaftwidth < eff_length * min_sw) {
			double scale = eff_length * min_sw / eff_shaftwidth;
			eff_shaftwidth = eff_length * min_sw;
			eff_headwidth *= scale;
			eff_headlength *= scale;
		}
		if (eff_headlength > eff_length * max_headlength) {
			double scale = eff_length * max_headlength / eff_headlength;
			eff_headlength = eff_length * max_headlength;
			eff_headwidth *= scale;
			eff_shaftwidth *= scale;
		}
	}
}

} // !namespace cvisual
