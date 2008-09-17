#ifndef VPYTHON_UTIL_LIGHTING_HPP
#define VPYTHON_UTIL_LIGHTING_HPP

// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "util/tmatrix.hpp"
#include "util/rgba.hpp"
namespace cvisual {

class light
{
 private:
	rgba diffuse;
	rgba specular;
	shared_vector position;
	bool local;

	shared_vector spot_direction; // defaults to <0,0,0>
	float spot_exponent; // defaults to 0
	float spot_cutoff; //< defaults to 180.

	double constant_attenuation; //< defaults to 1
	double linear_attenuation; //< defaults to 0
	double quadratic_attenuation; //< defaults to 0

	bool attenuated() const;
	bool spotlight() const;

 public:
	light( const vector& position, rgba color=rgba(1,1,1));\

	void set_pos( const vector& n_pos);
	shared_vector& get_pos();

	void set_local( bool);
	bool is_local();

	void set_spot_direction( const vector& n_dir);
	shared_vector& get_spot_direction();

	void set_spot_exponent( float e);
	float get_spot_exponent();

	void set_spot_cutoff( float e);
	float get_spot_cutoff();

	void set_attenuation( double constant=1.0, double linear=0.0, double quadratic=0.0);
	vector get_attentuation();

	void set_diffuse_color( const rgba& color);
	rgba get_diffuse_color();

	void set_specular_color( const rgba& color);
	rgba get_specular_color();

	// For internal use by the display_kernel object.
	void gl_begin( GLenum id, double gcf) const;
	void gl_end( GLenum id) const;
	vertex get_world_pos( const struct view& );
};

} // !namespace cvisual

#endif // !defined VPYTHON_UTIL_LIGHTING_HPP
