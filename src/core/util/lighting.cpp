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
	position( mtx, position), 
	local(true),
	spot_direction( mtx, vector()),
	spot_exponent(0), 
	spot_cutoff(180.0), 
	constant_attenuation(1.0),
	linear_attenuation( 0.0), 
	quadratic_attenuation(0.0)
{
}

light::light( const light& other)
	: diffuse( other.diffuse),
	specular( other.specular),
	position( mtx, other.position),
	local( other.local),
	spot_direction( mtx, other.spot_direction),
	spot_exponent( other.spot_exponent),
	spot_cutoff( other.spot_cutoff),
	constant_attenuation( other.constant_attenuation),
	linear_attenuation( other.linear_attenuation),
	quadratic_attenuation( other.quadratic_attenuation)
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
	lock L(mtx);
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
	lock L(mtx);
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
	lock L(mtx);
	spot_cutoff = e;
}

float 
light::get_spot_cutoff()
{
	return spot_cutoff;
}


void
light::set_attenuation( double constant, double linear, double quadratic)
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
	lock L(mtx);
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
	lock L(mtx);
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
	lock L(mtx);

	glEnable( id);
	if (attenuated()) {
		glLightf( id, GL_CONSTANT_ATTENUATION, constant_attenuation*gcf);
		glLightf( id, GL_LINEAR_ATTENUATION, linear_attenuation*gcf);
		glLightf( id, GL_QUADRATIC_ATTENUATION, quadratic_attenuation*gcf);
	}
	
	glLightfv( id, GL_DIFFUSE, &diffuse.red);
	glLightfv( id, GL_SPECULAR, &specular.red);
	
	vector _pos = position;
	if (local) {
		_pos = _pos.norm();
	}
	float pos[] = { _pos.x*gcf, _pos.y*gcf, _pos.z*gcf, 
		local ? 1.0f : 0.0f
	};
	glLightfv( id, GL_POSITION, pos);
	
	if (spotlight()) {
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
}

void
light::gl_end( GLenum id) const
{
	glDisable( id);
}

} // !namespace cvisual
