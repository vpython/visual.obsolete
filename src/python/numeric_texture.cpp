// Copyright (c) 2006 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "python/numeric_texture.hpp"
#include "util/gl_enable.hpp"
#include "util/errors.hpp"

#include <boost/crc.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>
#include <iostream>

namespace cvisual { namespace python {


namespace {

GLenum 
gl_type_name( array_types t)
{
	switch (t) {
	    case char_t:
	    	return GL_BYTE;
	    case uchar_t:
	    	return GL_UNSIGNED_BYTE;
	    case schar_t:
	    	return GL_BYTE;
	    case short_t:
	    	return GL_SHORT;
	    case int_t:
	    	return GL_INT;
	    case float_t:
	    	return GL_FLOAT;
	    default:
	    	return 1;
	}
}

boost::crc_32_type engine;
} // !namespace (anonymous)

numeric_texture::numeric_texture()
	: texdata(0), 
	data_width(0), data_height(0), data_channels(0), data_type( notype_t), 
		data_textype( 0), data_mipmapped(true), data_antialias(false),
	tex_width(0), tex_height(0), tex_channels(0), tex_type(notype_t),
		tex_textype( 0), tex_mipmapped(false), tex_antialias(false),
	checksum(0)
{
}

numeric_texture::~numeric_texture()
{
}

bool
numeric_texture::degenerate() const
{
	return data_width == 0 || data_height == 0 || data_channels == 0 || data_type == notype_t;
}

bool
numeric_texture::should_reinitialize(void) const
{
	return (
		data_channels != tex_channels ||
		data_mipmapped != tex_mipmapped ||
		data_type != tex_type ||
		(
			tex_mipmapped &&
			next_power_of_two(data_width) != tex_width ||
			next_power_of_two(data_height) != tex_height 
		) ||
		(
			!tex_mipmapped &&
			data_width != tex_width ||
			data_height != tex_height
		)
	);
}

void 
numeric_texture::gl_init(void)
{
	if (degenerate())
		return;
	
	gl_enable tex2D( GL_TEXTURE_2D);
	if (!handle) {
		glGenTextures(1, &handle);
		on_gl_free.connect( sigc::mem_fun(*this, &texture::gl_free));
		VPYTHON_NOTE( "Allocated texture number " + boost::lexical_cast<std::string>(handle));
	}
	glBindTexture(GL_TEXTURE_2D, handle);
	
	if (data_mipmapped) {
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
			data_antialias ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
			data_antialias ? GL_LINEAR : GL_NEAREST);
	}
	else {
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
			data_antialias ? GL_LINEAR : GL_NEAREST);
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
			data_antialias ? GL_LINEAR : GL_NEAREST);
	}
	tex_antialias = data_antialias;
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP);
	
	
	// Something is damaged.  Either the texture must be reinitialized
	// or just its data has changed.
	bool reinitialize = should_reinitialize();
	
	GLenum internal_format;
	if (!data_textype) {
		switch (data_channels) {
			case 1:
				internal_format = GL_LUMINANCE;
				break;
			case 2:
				internal_format = GL_LUMINANCE_ALPHA;
				break;
			case 3:
				internal_format = GL_RGB;
				break;
			case 4:
				internal_format = GL_RGBA;
				break;
			default: // Won't ever happen
				internal_format = GL_RGB;
		}
	}
	else {
		internal_format = data_textype;
		
		switch (data_textype) {
			case GL_LUMINANCE:
				if (data_channels != 1)
					throw std::invalid_argument( 
						"Specify luminance data with single values.");
				break;
			case GL_ALPHA:
				if (data_channels != 1)
					throw std::invalid_argument( 
						"Specify opacity data with single values.");
				break;
			case GL_LUMINANCE_ALPHA:
				if (data_channels != 2)
					throw std::invalid_argument( 
						"Specify luminance and opacity data with double values, [luminance,opacity].");
				break;
			case GL_RGB:
				if (data_channels != 3)
					throw std::invalid_argument( 
						"Specify RGB data with triple values, [r,g,b].");
				break;
			case GL_RGBA:
				if (data_channels != 4)
					throw std::invalid_argument( 
						"Specify RGB_opacity data with quadruple values, [r,g,b,opacity].");
				break;
			case 0: default: // Won't ever happen
				break;
		}
	}
	tex_textype = internal_format;
	
	if (reinitialize) {
		// Compute a fresh checksum
		engine.reset();
		engine.process_block( data(texdata), 
			data(texdata) + data_width*data_height*data_channels * typesize(data_type));
		checksum = engine.checksum();
	}
	int saved_alignment = -1;
	glGetIntegerv( GL_UNPACK_ALIGNMENT, &saved_alignment);
	int alignment = data_width % 4;
	if (!alignment)
		alignment = 4;
	if (alignment == 3)
		alignment = 1;
	glPixelStorei( GL_UNPACK_ALIGNMENT, alignment);	
	
	
	if (reinitialize && !data_mipmapped) {
		tex_width = next_power_of_two(data_width);
		tex_height = next_power_of_two(data_height);
		tex_channels = data_channels;
		tex_textype = data_textype;
		tex_type = data_type;
		tex_mipmapped = false;
		
		size_t data_size = tex_width*tex_height*typesize(data_type)*data_channels;
		boost::scoped_array<unsigned char> dummy_data( new unsigned char[data_size]);
		memset( dummy_data.get(), 0, data_size);
		
		glTexImage2D( GL_TEXTURE_2D, 0, internal_format, tex_width, tex_height,
			0, internal_format, gl_type_name( tex_type), dummy_data.get());
		glTexSubImage2D( GL_TEXTURE_2D, 0, 
			0, 0, data_width, data_height, 
			internal_format, gl_type_name( tex_type), data(texdata));
	}
	else if (data_mipmapped) {
		tex_width = data_width;
		tex_height = data_height;
		tex_channels = data_channels;
		tex_type = data_type;
		tex_textype = data_textype;
		tex_mipmapped = true;
		
		gluBuild2DMipmaps( GL_TEXTURE_2D, internal_format, tex_width, tex_height,
			internal_format, gl_type_name(tex_type), data(texdata));
	}
	else // Only upload new texture subimage
		glTexSubImage2D( GL_TEXTURE_2D, 0, 
			0, 0, data_width, data_height, 
			internal_format, gl_type_name(tex_type), data(texdata));
	glPixelStorei( GL_UNPACK_ALIGNMENT, saved_alignment);	
}

void
numeric_texture::gl_transform(void)
{
	if (degenerate())
		return;
	glMatrixMode( GL_TEXTURE);
	glLoadIdentity();
	if (data_width != tex_width || data_height != tex_height) {
		float x_scale = float(data_width) / tex_width;
		float y_scale = float(data_height) / tex_height;
		glScalef( x_scale, y_scale, 1);
	}
	glMatrixMode( GL_MODELVIEW);
}

void
numeric_texture::damage_check(void)
{
	if (degenerate())
		return;
	if (should_reinitialize() || data_antialias != tex_antialias) {
		damage();
		return;
	}
	
	engine.reset();
	engine.process_block( data(texdata), 
		data(texdata) + data_width*data_height*data_channels * typesize(data_type));
	uint32_t result = engine.checksum();
	if (result != checksum) {
		checksum = result;
		damage();
	}
}
	
void
numeric_texture::set_data( boost::python::numeric::array data)
{
	namespace py = boost::python;
	if (data == py::object() && texdata != py::object()) {
		throw std::invalid_argument( 
			"Cannot nullify a texture by assigning its data to None");
	}
	
	if (!iscontiguous(data))
		throw std::invalid_argument( "Texture data must be contiguous");
	array_types t = type(data);
	if (t == cfloat_t || t == cdouble_t || t == object_t || t == notype_t)
		throw std::invalid_argument( "Invalid texture data type");
	std::vector<int> dims = shape( data);
	if (dims.size() != 3 && dims.size() != 2) {
		throw std::invalid_argument( "Texture data must be NxMxC or NxM");
	}
	if (dims.size() == 3)
		if (dims[2] < 1 || dims[2] > 4) {
		throw std::invalid_argument( 
			"Texture data must be NxMxC, where C is between 1 and 4 (inclusive)");
	}
	
	if (t == double_t) {
		data = astype( data, float_t);
		t = float_t;
	}
	if (t == long_t) {
		data = astype( data, int_t);
		t = int_t;
	}

	lock L(mtx);
	damage();
	texdata = data;
	data_height = dims[0];
	data_width = dims[1];
	data_channels = dims.size() == 3 ? dims[2] : 1;
	have_opacity = (
		data_channels == 2 || 
		data_channels == 4 ||
		(data_channels == 1 && data_textype == GL_ALPHA)
	);

	data_type = t;
}

boost::python::numeric::array
numeric_texture::get_data()
{
	return texdata;
}

void
numeric_texture::set_type( std::string requested_type)
{
	GLenum req_type = 0;
	if (requested_type == "luminance")
		req_type = GL_LUMINANCE;
	else if (requested_type == "opacity")
		req_type = GL_ALPHA;
	else if (requested_type == "luminance_opacity")
		req_type = GL_LUMINANCE_ALPHA;
	else if (requested_type == "alpha")
		req_type = GL_ALPHA;
	else if (requested_type == "luminance_alpha")
		req_type = GL_LUMINANCE_ALPHA;
	else if (requested_type == "rgb")
		req_type = GL_RGB;
	else if (requested_type == "rgbo")
		req_type = GL_RGBA;
	else if (requested_type == "rgba")
		req_type = GL_RGBA;
	else if (requested_type == "auto")
		req_type = 0;
	else
		throw std::invalid_argument( "Unknown texture type");
	lock L(mtx);
	data_textype = req_type;
	if (req_type == GL_RGBA || req_type == GL_ALPHA || req_type == GL_LUMINANCE_ALPHA)
		have_opacity = true;
	damage();
}

std::string
numeric_texture::get_type() const
{
	switch (data_textype) {
		case GL_LUMINANCE:
			return std::string( "luminance");
		case GL_ALPHA:
			return std::string( "opacity");
		case GL_LUMINANCE_ALPHA:
			return std::string( "luminance_opacity");
		case GL_RGB:
			return std::string( "rgb");
		case GL_RGBA:
			return std::string( "rgbo");
		case GL_RGBA:
			return std::string( "rgba");
		case 0: default:
			return std::string( "auto");
	}
}

void
numeric_texture::set_mipmapped( bool m)
{
	lock L(mtx);
	damage();
	data_mipmapped = m;
}

bool
numeric_texture::is_mipmapped(void)
{
	return data_mipmapped;
}

void
numeric_texture::set_antialias( bool aa)
{
	lock L(mtx);
	data_antialias = aa;
}

bool 
numeric_texture::is_antialiased( void)
{
	return data_antialias;
}

} } // !namespace cvisual::python
