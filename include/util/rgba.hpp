#ifndef VPYTHON_UTIL_RGBA_HPP
#define VPYTHON_UTIL_RGBA_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "wrap_gl.hpp"

namespace cvisual {

/** A helper class to manage OpenGL color attributes.  The data is layout
	compatable with OpenGL's needs for the various vector forms of commands,
	like glColor4fv(), and glColorPointer().
*/
class rgba
{
 public:
	/** Red channel intensity, clamped to [0,1] */
	float red;
	/** Green channel intensity, clamped to [0,1] */
	float green;
	/** Blue channel intensity, clamped to [0,1] */
	float blue;
	/** Alpha channel intensity, clamped to [0,1] */
	float alpha;


	/** Defaults to opaque white. */
	inline rgba() : red(1.0), green(1.0), blue(1.0), alpha(1.0) {}
	/** Allocate a new color. */
	inline rgba( float r, float g, float b, float a = 1.0)
		: red(r), green(g), blue(b), alpha(a) {}
	inline rgba( float bw)
		: red(bw), green(bw), blue(bw), alpha(1) {}
	inline explicit rgba( const float* c)
		: red(c[0]), green(c[1]), blue(c[2]), alpha( c[3]) {}
			
	/** Convert to HSVA, lower saturation by 50%, convert back to RGBA. 
		@return The desaturated color.
	*/
	rgba desaturate() const;
	/** Convert to greyscale, accounting for differences in perception.  This
		function makes 4 calls to std::pow(), and is very slow.
		@return The scaled color.
	*/
	rgba grayscale() const;
	
	/** Make this the active OpenGL color using glColor(). */
	inline void gl_set() const
	{ glColor4fv( &red); }
	
	/** Make this the active OpenGL material property for front and back ambient
		and diffuse color.
	*/
	inline void gl_material() const
	{ glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, &red); }
};


class rgb
{
 public:
	float red;
	float green;
	float blue;
	
	inline rgb() : red(1.0f), green(1.0f), blue(1.0f) {}
	
	inline rgb( float r, float g, float b)
		: red(r), green(g), blue(b)
	{}
	
	inline rgb( float bw)
		: red(bw), green(bw), blue(bw)
	{}
	
	inline rgb( const rgba& other)
		: red( other.red), green( other.green), blue(other.blue)
	{}
	
	inline operator rgba() const { return rgba( red, green, blue, 1.0f); }
	
	rgb desaturate() const;
	rgb grayscale() const;
	
	inline void gl_set() const
	{ glColor3fv( &red); }
};

} // !namespace cvisual

#endif // !defined VPYTHON_UTIL_RGBA_HPP
