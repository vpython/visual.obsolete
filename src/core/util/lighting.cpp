#include "util/lighting.hpp"
#include "util/errors.hpp"
#include <cassert>


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

# if 0


light::light( vertex v, rgba c)
	: x( static_cast<float>(v.x)),
	y( static_cast<float>(v.y)),
	z( static_cast<float>(v.z)),
	w( static_cast<float>(v.w)),
	color(c)
{
}

lighting::lighting()
	: ambient( .2, .2, .2), n_lights(2)
{
	// Default: two white lights at infinity of varying brightness.
	lights[0] = 
		light( vertex(vector(0.25, 0.5, 1.0).norm(), 0), rgba(0.8, 0.8, 0.8));
	lights[1] = 
		light( vertex(vector(-1.0, -0.25, -0.5).norm(), 0), rgba(.3, .3, .3));
}

void
lighting::gl_begin() const
{
	clear_gl_error();
	assert( n_lights <= 8);
	glLightModelfv( GL_LIGHT_MODEL_AMBIENT, &ambient.red);
	switch (n_lights) {
		// Each of these falls through.
		case 8:
			glLightfv( GL_LIGHT7, GL_POSITION, &lights[7].x );
			glLightfv( GL_LIGHT7, GL_DIFFUSE, &lights[7].color.red);
			glLightfv( GL_LIGHT7, GL_SPECULAR, &lights[7].color.red);
			glEnable( GL_LIGHT7);
		case 7:
			glLightfv( GL_LIGHT6, GL_POSITION, &lights[6].x);
			glLightfv( GL_LIGHT6, GL_DIFFUSE, &lights[6].color.red);
			glLightfv( GL_LIGHT6, GL_SPECULAR, &lights[6].color.red);
			glEnable( GL_LIGHT6);
		case 6:
			glLightfv( GL_LIGHT5, GL_POSITION, &lights[5].x);
			glLightfv( GL_LIGHT5, GL_DIFFUSE, &lights[5].color.red);
			glLightfv( GL_LIGHT5, GL_SPECULAR, &lights[5].color.red);
			glEnable( GL_LIGHT5);
		case 5:
			glLightfv( GL_LIGHT4, GL_POSITION, &lights[4].x);
			glLightfv( GL_LIGHT4, GL_DIFFUSE, &lights[4].color.red);
			glLightfv( GL_LIGHT4, GL_SPECULAR, &lights[4].color.red);
			glEnable( GL_LIGHT4);
		case 4:
			glLightfv( GL_LIGHT3, GL_POSITION, &lights[3].x);
			glLightfv( GL_LIGHT3, GL_DIFFUSE, &lights[3].color.red);
			glLightfv( GL_LIGHT3, GL_SPECULAR, &lights[3].color.red);
			glEnable( GL_LIGHT3);
		case 3:
			glLightfv( GL_LIGHT2, GL_POSITION, &lights[2].x);
			glLightfv( GL_LIGHT2, GL_DIFFUSE, &lights[2].color.red);
			glLightfv( GL_LIGHT2, GL_SPECULAR, &lights[2].color.red);
			glEnable( GL_LIGHT2);
		case 2:
			glLightfv( GL_LIGHT1, GL_POSITION, &lights[1].x);
			glLightfv( GL_LIGHT1, GL_DIFFUSE, &lights[1].color.red);
			glLightfv( GL_LIGHT1, GL_SPECULAR, &lights[1].color.red);
			glEnable( GL_LIGHT1);
		case 1:
			glLightfv( GL_LIGHT0, GL_POSITION, &lights[0].x);
			glLightfv( GL_LIGHT0, GL_DIFFUSE, &lights[0].color.red);
			glLightfv( GL_LIGHT0, GL_SPECULAR, &lights[0].color.red);
			glEnable( GL_LIGHT0);
			break;
		default:
			// Greater than eight lights???
			bool number_of_lights_less_than_eight = false;
			assert( number_of_lights_less_than_eight == true);
			return;
	}
	check_gl_error();
}

void
lighting::gl_end() const
{
	clear_gl_error();
	switch (n_lights) {
		case 8:
			glDisable( GL_LIGHT7);
		case 7:
			glDisable( GL_LIGHT6);
		case 6:
			glDisable( GL_LIGHT5);
		case 5:
			glDisable( GL_LIGHT4);
		case 4:
			glDisable( GL_LIGHT3);
		case 3:
			glDisable( GL_LIGHT2);
		case 2:
			glDisable( GL_LIGHT1);
		case 1:
			glDisable( GL_LIGHT0);
		default:
			return;
	}
	check_gl_error();
}

#endif
