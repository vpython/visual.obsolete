#ifndef VPYTHON_WRAP_GL_HPP
#define VPYTHON_WRAP_GL_HPP

// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

/* A header file to wrap around GL/gl.h on *nix and Windows.
 */ 
 
#if defined(_WIN32) || defined(_MSC_VER)
# define WIN32_LEAN_AND_MEAN 1
# include <windows.h>
# include <GL/gl.h>
# include <GL/glext.h>
#else
# include <GL/gl.h>
#endif

#include <GL/glu.h>

#endif // !defined VPYTHON_WRAP_GL_HPP
