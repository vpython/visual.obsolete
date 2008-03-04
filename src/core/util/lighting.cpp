// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "util/lighting.hpp"
#include "util/errors.hpp"
#include <cassert>

namespace cvisual {

bool
light::attenuated() const
{
	return constant_attenuation != 1.0 
		|| linear_attenuation != 1.0 
		|| quadratic_attenuation != 0.0;
}

bool
light::spotlight() const
{
	return spot_cutoff != 0 || 
		(spot_cutoff != 180 && spot_direction != vector());
}

light::light( const vector& position, rgba color)
	: diffuse( color), 
	specular( color), 
	position( position ), 
	local(true),
	spot_direction(vector()),
	spot_exponent(0), 
	spot_cutoff(180.0), 
	constant_attenuation(1.0),
	linear_attenuation( 0.0), 
	quadratic_attenuation(0.0)
{
}

void 
light::set_pos( const vector& n_pos)
{
	position = n_pos;
}

shared_vector& 
light::get_pos()
{
	return position;
}

void 
light::set_local( bool n_local)
{
	local = n_local;
}

bool 
light::is_local()
{
	return local;
}

void 
light::set_spot_direction( const vector& n_dir)
{
	spot_direction = n_dir;
}

shared_vector& 
light::get_spot_direction()
{
	return spot_direction;
}

void 
light::set_spot_exponent( float e)
{
	if (e < 0 || e > 128)
		throw std::invalid_argument( 
			"spot exponent must be within the range [0, 128].");
	spot_exponent = e;
}

float 
light::get_spot_exponent()
{
	return spot_exponent;
}
 
void 
light::set_spot_cutoff( float e)
{
	if (e != 180 && (e < 0 || e > 90))
		throw std::invalid_argument( 
			"spot cutoff angle must be an angle between [0,90], or exactly 180 "
			"degrees.");
	spot_cutoff = e;
}

float 
light::get_spot_cutoff()
{
	return spot_cutoff;
}


void
light::set_attenuation( float constant, float linear, float quadratic)
{
	if (!local)
		throw std::invalid_argument( "Only local lights may be attenuated.");
	if (constant < 0 || linear < 0 || quadratic < 0)
		throw std::invalid_argument( 
			"Light attenuation factors must be non-negative.");
	constant_attenuation = constant;
	linear_attenuation = linear;
	quadratic_attenuation = quadratic;
}

vector
light::get_attentuation()
{
	return vector( 
		constant_attenuation, linear_attenuation, quadratic_attenuation);
}

void 
light::set_diffuse_color( const rgba& color)
{
	diffuse = color;
}

rgba 
light::get_diffuse_color()
{
	return diffuse;
}

void 
light::set_specular_color( const rgba& color)
{
	specular = color;
}

rgba 
light::get_specular_color()
{
	return specular;
}

void
light::gl_begin( GLenum id, double gcf) const
{
	glEnable( id);
	if (attenuated()) {
		glLightf( id, GL_CONSTANT_ATTENUATION, (GLfloat) (constant_attenuation*gcf));
		glLightf( id, GL_LINEAR_ATTENUATION, (GLfloat)(linear_attenuation*gcf));
		glLightf( id, GL_QUADRATIC_ATTENUATION, (GLfloat)(quadratic_attenuation*gcf));
	}
	
	glLightfv( id, GL_DIFFUSE, &diffuse.red);
	glLightfv( id, GL_SPECULAR, &specular.red);
	
	vector _pos = position;
	if (!local) {
		_pos = _pos.norm();
	}
	float pos[] = { (float) (_pos.x*gcf),(float) (_pos.y*gcf), (float) (_pos.z*gcf), 
		local ? 1.0f : 0.0f
	};
	glLightfv( id, GL_POSITION, pos);
	
	if (spotlight()) {
		glLightf( id, GL_SPOT_CUTOFF, spot_cutoff);
		glLightf( id, GL_SPOT_EXPONENT, spot_exponent);
		if (spot_cutoff != 180) {
			vector _spot_direction = spot_direction.norm();
			float spot_dir[] = { 
				(float) (_spot_direction.x * gcf),
				(float)(_spot_direction.y * gcf),
				(float)(_spot_direction.z * gcf),
				1.0f
			};
			glLightfv( id, GL_SPOT_DIRECTION, spot_dir);
		}
	}
}

void
light::gl_end( GLenum id) const
{
	glDisable( id);
}

} // !namespace cvisual
