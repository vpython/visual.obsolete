#ifndef VPYTHON_UTIL_RGBA_HPP
#define VPYTHON_UTIL_RGBA_HPP

#include <GL/gl.h>

class rgba
{
 public:
	float red;
	float green;
	float blue;
    float alpha;
 
	inline rgba() : red(1.0), green(1.0), blue(1.0), alpha(1.0) {}
	inline rgba( float r, float g, float b, float a = 1.0)
		: red(r), green(g), blue(b), alpha(a) {}
			
	// Convert to HSVA, lower saturation by 50%, convert back to RGBA.
	rgba desaturate() const;
	// Convert to greyscale.
	rgba grayscale() const;
			
	inline void gl_set() const
	{ glColor4f( red, green, blue, alpha); }
	
	inline void gl_material() const
	{ glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, &red); }
};

#endif // !defined VPYTHON_UTIL_RGBA_HPP
