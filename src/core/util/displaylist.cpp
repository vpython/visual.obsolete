#include "util/displaylist.hpp"

#include <cassert>
#include <GL/gl.h>

// Don't draw a number until it is time to compile the thing.
displaylist::displaylist()
	: handle(0)
{
}

displaylist::~displaylist()
{
	if (handle != 0) {
		// Deallocate OpenGL resources associated with this handle, save the number
		// for later use.
		glDeleteLists(handle, 1);
	}
}

void 
displaylist::gl_compile_begin()
{
	if (handle == 0) {
		handle = glGenLists(1);
	}
	glNewList( handle, GL_COMPILE);
}
	
void 
displaylist::gl_compile_end()
{
	glEndList();
}

void
displaylist::gl_render() const
{
	assert( handle != 0);
	glCallList( handle);
}
