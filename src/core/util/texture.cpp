// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "wrap_gl.hpp"
#include "util/texture.hpp"

namespace cvisual {

texture::texture()
	: damaged(false), handle(0)
{
}

texture::~texture()
{
	if (handle)
		glDeleteTextures(1, &handle);
}

texture::operator bool() const
{
	return handle != 0;
}

void
texture::gl_activate()
{
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

void
texture::gl_transform()
{
}

void
texture::damage()
{
	damaged = false;
}

} // !namespace cvisual
