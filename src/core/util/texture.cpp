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
	: damaged(false), handle(0)
{
}

texture::~texture()
{
	if (handle) {
		VPYTHON_NOTE( "Deleting texture number " 
			+ lexical_cast<std::string>(handle));
		glDeleteTextures(1, &handle);
	}
}

texture::operator bool() const
{
	return handle != 0;
}

void
texture::gl_activate()
{
	if (!handle)
		on_gl_free.connect( SigC::slot(*this, &texture::gl_free));
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
texture::damage()
{
	damaged = false;
}

} // !namespace cvisual
