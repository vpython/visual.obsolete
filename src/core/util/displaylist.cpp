// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "wrap_gl.hpp"

#include "util/displaylist.hpp"
#include <cassert>

namespace cvisual {

void 
displaylist::deleter( unsigned int* handle)
{
	glDeleteLists(*handle, 1);
	delete handle;
}

// Don't draw a number until it is time to compile the thing.
displaylist::displaylist()
{
}

displaylist::~displaylist()
{
}

void 
displaylist::gl_compile_begin()
{
	if (!handle) {
		handle = shared_ptr<unsigned int>( 
			new unsigned int, &displaylist::deleter);
		*handle = glGenLists(1);
	}
	glNewList( *handle, GL_COMPILE);
}
	
void 
displaylist::gl_compile_end()
{
	glEndList();
}

void
displaylist::gl_render() const
{
	assert( handle);
	glCallList( *handle);
}

} // !namespace cvisual
