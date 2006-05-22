#ifndef VPYTHON_PYTHON_NUMERIC_TEXTURE_HPP
#define VPYTHON_PYTHON_NUMERIC_TEXTURE_HPP

#include <boost/python/numeric.hpp>
#include "util/texture.hpp"
#include "python/num_util.hpp"
#include <inttypes.h>

namespace cvisual { namespace python {

/**
 * Python users can specify a texture as NxMxC, where N and M are preferred to
 * be powers of 2.  C is the number of color channels, and must be one of 
 * 1, 2, 3, or 4.  The meaning of the texture is determined by its channels:
 * 1: luminance map
 * 2: luminance-alpha
 * 3: RGB
 * 4: RGBA
 * 
 * 
 */
class numeric_texture
{
 private:
	boost::python::numeric::array texdata;
	
	size_t data_width;
	size_t data_height;
	size_t data_channels;
	array_types data_type;
	
	size_t tex_width;
	size_t tex_height;
	size_t tex_channels;
	// Types of damage: those that change the array's type and/or dimensions,
	// and those that change its contents
	uint32_t checksum;
	
	bool mipmapped;
	bool degenerate();
	void damage_check(void);
	void gl_transform(void);

 public:
	numeric_texture();
	virtual ~numeric_texture();
	virtual void gl_init(void);
	virtual void transform(void);
	
	void set_data( boost::python::numeric::array data);
	boost::python::numeric::array get_data();
};

} } // !namespace cvisual::python

#endif /* VPYTHON_PYTHON_NUMERIC_TEXTURE_HPP */
