#ifndef VPYTHON_WRAP_GL_HPP
#define VPYTHON_WRAP_GL_HPP

/* A header file to wrap around GL/gl.h on *nix and Windows.
 */ 
 
#if defined(_WIN32) || defined(_MSC_VER)
# define WIN32_LEAN_AND_MEAN 1
# include <windows.h>
#endif

#include <GL/gl.h>
#include <GL/glu.h>

#endif // !defined VPYTHON_WRAP_GL_HPP
