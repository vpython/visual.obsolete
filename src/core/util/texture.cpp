// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "wrap_gl.hpp"
#include "util/texture.hpp"
#include "util/errors.hpp"
#include <sigc++/object_slot.h>
#include <boost/lexical_cast.hpp>
using boost::lexical_cast;

namespace cvisual {

texture::texture()
	: damaged(false), handle(0), have_alpha(false)
{
}

texture::~texture()
{
	gl_free();
}

#if 0
texture::operator bool() const
{
	return handle != 0;
}
#endif

void
texture::gl_activate()
{
	lock L(mtx);
	damage_check();
	if (!handle || damaged) {
		gl_init();
		damaged = false;
		// Verify gl_init()'s postcondition.
		assert(handle != 0);
	}
	// Verify execution flowpath
	assert( handle != 0);
	
	glBindTexture( GL_TEXTURE_2D, handle);
	this->gl_transform();
}

bool
texture::has_alpha() const
{
	return have_alpha;
}

void
texture::gl_free()
{
	if (handle) {
		VPYTHON_NOTE( "Deleting texture number " 
			+ lexical_cast<std::string>(handle));
		glDeleteTextures(1, &handle);
		handle = 0;
	}
}

void
texture::gl_transform()
{
}

void 
texture::damage_check()
{
}

void
texture::damage()
{
	damaged = false;
}

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

} // !namespace cvisual
