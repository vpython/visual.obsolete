#ifndef VPYTHON_UTIL_GL_EXTENSIONS_HPP
#define VPYTHON_UTIL_GL_EXTENSIONS_HPP
#pragma once

#include "wrap_gl.hpp"

namespace cvisual {

// GL extension functions wrapper - just the functions we currently need
// This could be replaced by a library like GLEW, if it becomes a hassle to maintain.

class gl_extensions {
 public:
	// All extensions will be unavailable until init() is called.
	gl_extensions();

	// Must be initialized and used with the same OpenGL context current
	void init( class display& d );

	// Extension: ARB_shader_objects
	bool ARB_shader_objects;
	PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB;
	PFNGLLINKPROGRAMARBPROC			glLinkProgramARB;
	PFNGLUSEPROGRAMOBJECTARBPROC    glUseProgramObjectARB;
	PFNGLCREATESHADEROBJECTARBPROC	glCreateShaderObjectARB;
	PFNGLSHADERSOURCEARBPROC		glShaderSourceARB;
	PFNGLCOMPILESHADERARBPROC		glCompileShaderARB;
	PFNGLATTACHOBJECTARBPROC		glAttachObjectARB;
	PFNGLDELETEOBJECTARBPROC		glDeleteObjectARB;
	PFNGLGETHANDLEARBPROC			glGetHandleARB;
	PFNGLUNIFORM1IARBPROC			glUniform1i;
	PFNGLUNIFORMMATRIX4FVARBPROC	glUniformMatrix4fv;
	PFNGLGETUNIFORMLOCATIONARBPROC	glGetUniformLocation;
	
	// Extension: EXT_texture3D
	bool EXT_texture3D;
	PFNGLTEXIMAGE3DEXTPROC			glTexImage3D;
	PFNGLTEXSUBIMAGE3DEXTPROC		glTexSubImage3D;
	
	// Extension: ARB_multitexture
	bool ARB_multitexture;
	PFNGLACTIVETEXTUREARBPROC		glActiveTexture;
	
};

}

#endif