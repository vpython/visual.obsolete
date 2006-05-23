#ifndef VPYTHON_PYTHON_NUMERIC_TEXTURE_HPP
#define VPYTHON_PYTHON_NUMERIC_TEXTURE_HPP

#include <boost/python/numeric.hpp>
#include "util/texture.hpp"
#include "python/num_util.hpp"
#include "wrap_gl.hpp"
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
class numeric_texture : public texture
{
 private:
	boost::python::numeric::array texdata;
	
	// A texture is data_width x data_height x data_channels
	size_t data_width;
	size_t data_height;
	size_t data_channels;
	array_types data_type; // The type of C data in the memory object
	GLenum data_textype; // The type of GL texture object (GL_ALPHA, GL_RGB, etc)
	bool data_mipmapped; // true if the data should be mipmapped
	
	size_t tex_width;
	size_t tex_height;
	size_t tex_channels;
	array_types tex_type;
	GLenum tex_textype;
	bool tex_mipmapped;
	// Types of damage: those that change the array's type and/or dimensions,
	// and those that change its contents
	uint32_t checksum;
	
	
	bool degenerate() const;
	bool should_reinitialize() const;
 protected:
	virtual void damage_check(void);
	virtual void gl_init(void);
	virtual void transform(void);

 public:
	numeric_texture();
	virtual ~numeric_texture();
	
	void set_data( boost::python::numeric::array data);
	boost::python::numeric::array get_data();
	
	void set_type( std::string requested_type);
	std::string get_type() const;
	
	void set_mipmapped( bool);
	bool is_mipmapped(void);
};

} } // !namespace cvisual::python

#endif /* VPYTHON_PYTHON_NUMERIC_TEXTURE_HPP */
