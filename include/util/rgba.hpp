#ifndef VPYTHON_UTIL_RGBA_HPP
#define VPYTHON_UTIL_RGBA_HPP

#include <GL/gl.h>

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
	{ glColor4f( red, green, blue, alpha); }
	
	/** Make this the active OpenGL material property for front and back ambient
		and diffuse color.
	*/
	inline void gl_material() const
	{ glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, &red); }
};

#endif // !defined VPYTHON_UTIL_RGBA_HPP
