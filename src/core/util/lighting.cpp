// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "util/lighting.hpp"
#include "util/errors.hpp"
#include <cassert>

namespace cvisual {

light::light( vector position, rgba color, bool _local)
	: diffuse( color), specular( color), position( position), local(_local),
		spot_exponent(0), spot_cutoff(180.0), constant_attenuation(1.0),
		linear_attenuation( 0.0), quadratic_attenuation(0.0)
{
	if (!local)
		position = position.norm();
}

void
light::set_attenuation( double constant, double linear, double quadratic)
{
	if (!local)
		throw std::invalid_argument( "Only local lights may be attenuated.");
	constant_attenuation = constant;
	linear_attenuation = linear;
	quadratic_attenuation = quadratic;
}

void
light::disable_attenuation()
{
	constant_attenuation = 1.0;
	linear_attenuation = 0.0;
	quadratic_attenuation = 0.0;
}

void
light::set_spot( vector direction, float exponent, float cutoff)
{
	spot_direction = direction;
	spot_exponent = exponent;
	spot_cutoff = cutoff;
}

void
light::disable_spot()
{
	spot_exponent = 0.0;
	spot_cutoff = 180.0;
}

void
light::gl_begin( GLenum id, double gcf) const
{
	glEnable( id);
	glLightf( id, GL_CONSTANT_ATTENUATION, constant_attenuation*gcf);
	glLightf( id, GL_LINEAR_ATTENUATION, linear_attenuation*gcf);
	glLightf( id, GL_QUADRATIC_ATTENUATION, quadratic_attenuation*gcf);
	
	glLightfv( id, GL_DIFFUSE, &diffuse.red);
	glLightfv( id, GL_SPECULAR, &specular.red);
	
	float pos[] = { position.x * gcf, position.y*gcf, position.z*gcf, 
		local ? 1.0f : 0.0f
	};
	glLightfv( id, GL_POSITION, pos);
	glLightf( id, GL_SPOT_CUTOFF, spot_cutoff);
	glLightf( id, GL_SPOT_EXPONENT, spot_exponent);
	if (spot_cutoff != 180) {
		float spot_dir[] = { 
			spot_direction.x * gcf,
			spot_direction.y * gcf,
			spot_direction.z * gcf,
			1.0f
		};
		glLightfv( id, GL_SPOT_DIRECTION, spot_dir);
	}
}

void
light::gl_end( GLenum id) const
{
	glDisable( id);
}

} // !namespace cvisual
