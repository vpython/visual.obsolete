// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "wrap_gl.hpp"

#include "util/displaylist.hpp"
#include "util/errors.hpp"
#include <cassert>
#include <sigc++/object_slot.h>
#include <boost/lexical_cast.hpp>
using boost::lexical_cast;

namespace cvisual {

void 
displaylist::deleter( unsigned int* handle)
{
	if (*handle) {
		VPYTHON_NOTE( "Deleting displaylist number " 
			+ lexical_cast<std::string>(*handle));
		glDeleteLists(*handle, 1);
		*handle = 0;
	}
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
		VPYTHON_NOTE( "Allocated displaylist number " 
			+ lexical_cast<std::string>(*handle));
		on_gl_free.connect( SigC::slot(*this, &displaylist::gl_free));
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

void
displaylist::gl_free()
{
	if (handle && *handle) {
		VPYTHON_NOTE( "Deleting displaylist number " 
			+ lexical_cast<std::string>(handle));
		glDeleteLists( *handle, 1);
	}
	*handle = 0;
}

} // !namespace cvisual
