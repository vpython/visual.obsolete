#include "python/numeric_texture.hpp"

#include <boost/crc.hpp>

namespace cvisual { namespace python {


namespace {
int
next_power_of_two(const int arg) 
{
	int ret = 2;
	// upper bound of 28 chosen to limit memory growth to about 256MB, which is
	// _much_ larger than most supported textures
	while (ret < arg && ret < (1 << 28))
		ret <<= 1;
	return ret;
}

size_t typesize( array_types t)
{
	switch (t) {
	    case char_t:
	    	return sizeof (char);
	    case uchar_t:
	    	return sizeof (unsigned char);
	    case schar_t:
	    	return sizeof (signed char);
	    case short_t:
	    	return sizeof (short);
	    case int_t:
	    	return sizeof (int);
	    case long_t:
	    	return sizeof (long);
	    case float_t:
	    	return sizeof (float);
	    case double_t:
	    	return sizeof (double);
	    case cfloat_t:
	    	return sizeof (float)*2;
	    case cdouble_t:
	    	return sizeof (double)*2;
		default:
			bool type_is_recognized = false;
			assert( type_is_recognized == true);
	}
}

GLenum gl_type_name( array_types t)
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
	    	return FLOAT;
	    default:
	    	return 1;
	}
}

boost::crc_32_type engine;
} // !namespace (anonymous)

numeric_texture::numeric_texture()
	: texdata(0), width(0), height(0), channels(0), type( notype_t), checksum(0),
	mipmapped(true)
{
}

numeric_texture::~numeric_texture()
{
}

bool
numeric_texture::degenerate()
{
	return width == 0 || height == 0 || channels == 0 || type == notype_t;
}

void 
numeric_texture::gl_init(void)
{
	if (degenerate())
		return;
	
	// Otherwise, something is damaged.  Either the texture must be reinitialized
	// or just its data has changed.
	bool reinitialize = (
		data_channels != tex_channels ||
		next_power_of_two(data_width) != tex_width ||
		next_power_of_two(data_height) != tex_height
	);
	
	GLenum format;
	switch (tex_channels) {
		case 1:
			format = GL_LUMINANCE;
			break;
		case 2:
			format = GL_LUMINANCE_ALPHA;
			break;
		case 3:
			format = GL_RGB;
			break;
		case 4:
			format = GL_RGBA;
			break;
		default: // Won't ever happen
			format = GL_RGB;
	}
	
	if (reinitialize) {
		tex_width = next_power_of_two(data_width);
		tex_height = next_power_of_two(data_height);
		tex_channels = data_channels;
		size_t data_size = tex_width*tex_height*typesize(data_type)*data_channels
		boost::scoped_array dummy_data( 
			new unsigned char[data_size]);
		memset( dummy_data.get(), 0, data_size);
		glTexImage2D( GL_TEXTURE_2D, 0, tex_channels, tex_width, tex_height,
			0, format, gl_type_name( type), dummy_data.get());
	}
	
	glTexSubImage2d( GL_TEXTURE_2D, 0, 0, 0,
		data_width, data_height, format, gl_type_name(type), data(texdata));
}

void
numeric_texture::transform(void)
{
	if (degenerate())
		return;
}

void
numeric_texture::damage_check(void)
{
	if (degenerate())
		return;
	if (tex_channels != data_channels)
		damage();
	engine.process_block( data(texdata), width*height*channels * typesize(type));
	uint32_t result = engine.checksum();
	engine.reset();
	if (result != checksum) {
		checksum = result;
		damage();
	}
}
	
void
numeric_texture::set_data( boost::python::numeric::array data)
{
	if (data == object() && texdata != object()) {
		throw std::invalid_argument( 
			"Cannot nullify a texture by assigning its data to None");
	}
	
	if (!iscontiguous(data))
		throw std::invalid_argument( "Texture data must be contiguous");
	array_types t = type(data);
	if (t == cfloat_t || t == cdouble_t || t == object_t || t == notype_t)
		throw std::invalid_argument( "Invalid texture data type");
	std::vector<int> dims = shape( data);
	if (dims.size() != 3) {
		throw std::invalid_argument( "Texture data must be NxMxC");
	}
	if (dims[2] < 1 || dims[2] > 4) {
		throw std::invalid_argument( 
			"Texture data must be NxMxC, where C is between 1 and 4 (inclusive)");
	}
	
	if (t == double_t)
		data = astype( data, float_t);
	if (t == long_t)
		data = astype( data, int_t);
	
	lock L(mtx);
	damage();
	texdata = data;
	data_width = dims[0];
	data_height = dims[1];
	data_channels = dims[2];
	if (data_channels == 2 || data_channels == 4) {
		have_alpha = true;
	}
	else
		have_alpha = false;
	data_type = t;
}

boost::python::numeric::array
numeric_texture::get_data()
{
	return texdata;
}

} } // !namespace cvisual::python
