#ifndef VPYTHON_UTIL_LIGHTING_HPP
#define VPYTHON_UTIL_LIGHTING_HPP

#include "util/tmatrix.hpp"
#include "util/rgba.hpp"
#include "wrap_gl.hpp"

class light
{
 private:
	rgba diffuse;
	rgba specular;
	vector position;
	bool local;
	vector spot_direction; // defaults to <0,0,0>
	float spot_exponent; // defaults to 0
	float spot_cutoff; //< defaults to 180.
	double constant_attenuation; //< defaults to 1
	double linear_attenuation; //< defaults to 0
	double quadratic_attenuation; //< defaults to 0
	
 public:
	light( vector position, rgba color, bool local=false);
	void set_attenuation( double constant=1.0, double linear=0.0, double quadratic=0.0);
	void disable_attenuation();
	void set_spot( vector direction, float exponent, float cutoff);
	void disable_spot();
	void set_diffuse( const rgba& color) { diffuse = color; }
	void set_specular( const rgba& color) { specular = color; }
	void set_color( const rgba& color) { set_diffuse(color), set_specular(color); }
	void gl_begin( GLenum id, double gcf) const;
	void gl_end( GLenum id) const;
};

#endif // !defined VPYTHON_UTIL_LIGHTING_HPP
