#ifndef VPYTHON_WRAP_GL_HPP
#define VPYTHON_WRAP_GL_HPP

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
