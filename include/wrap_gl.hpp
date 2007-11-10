
#ifndef VPYTHON_WRAP_GL_HPP
#define VPYTHON_WRAP_GL_HPP

// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

//A header file to wrap around GL/gl.h on *nix and Windows.
// The MSC_VER stuff refers to an effort in early 2007 to use MS Visual Studio
#if defined(_MSC_VER)
    #define NOMINMAX
    #include <GL/glaux.h>
	#include "win32/win_glext.hpp"
	#define GL_GLEXT_PROTOTYPES 1
	#include <GL/gl.h>
#elif defined(_WIN32)
	#define WIN32_LEAN_AND_MEAN 1
	#include <windows.h>
	#include <GL/gl.h>
	#include <GL/glext.h>
#else
	#define GL_GLEXT_PROTOTYPES 1
	#include <GL/gl.h>
	#include <GL/glext.h>
#endif

#include <GL/glu.h>
#endif // !defined VPYTHON_WRAP_GL_HPP
