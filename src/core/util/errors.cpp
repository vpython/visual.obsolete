#include "util/errors.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <GL/gl.h>
#include <GL/glu.h>

void
write_critical( 
	std::string file, 
	int line, 
	std::string function, 
	std::string message)
{
	std::cerr << "VPython ***CRITICAL ERROR***: " << file << ": " << line << ": " 
	<< function << ": " << message << "\n";
	return;
}

void
write_warning( 
	std::string file, 
	int line, 
	std::string function, 
	std::string message)
{
	std::cerr << "VPython WARNING: " << file << ": " << line << ": " 
	<< function << ": " << message << "\n";
	return;
}

void
dump_glmatrix()
{
	// TODO: set this up to write out a matrix with the same format for all of
	// the members.
	float M[4][4];
	glGetFloatv( GL_MODELVIEW_MATRIX, M[0]);
	std::cout << "Modelview matrix status:\n";
	std::cout << "| " << M[0][0] << " " << M[1][0] << " " << M[2][0] << " " << M[3][0] << "|\n";
	std::cout << "| " << M[0][1] << " " << M[1][1] << " " << M[2][1] << " " << M[3][1] << "|\n";
	std::cout << "| " << M[0][2] << " " << M[1][2] << " " << M[2][2] << " " << M[3][2] << "|\n";
	std::cout << "| " << M[0][3] << " " << M[1][3] << " " << M[2][3] << " " << M[3][3] << "|\n";
	
	glGetFloatv( GL_PROJECTION_MATRIX, M[0]);
	std::cout << "Projection matrix status:\n";
	std::cout << "| " << M[0][0] << " " << M[1][0] << " " << M[2][0] << " " << M[3][0] << "|\n";
	std::cout << "| " << M[0][1] << " " << M[1][1] << " " << M[2][1] << " " << M[3][1] << "|\n";
	std::cout << "| " << M[0][2] << " " << M[1][2] << " " << M[2][2] << " " << M[3][2] << "|\n";
	std::cout << "| " << M[0][3] << " " << M[1][3] << " " << M[2][3] << " " << M[3][3] << "|\n";	
}

void
clear_gl_error_real()
{
	#ifndef NDEBUG
	glGetError();
	#endif
}

void
check_gl_error_real( const char* file, int line)
{
	#ifndef NDEBUG
	GLenum err_code = glGetError();
	// Insert the manual cast from the unsigned char pointer to signed char
	// pointer type.
	if (err_code != GL_NO_ERROR) {
		std::ostringstream err;
		err << file << ":" << line << " " << (const char*)gluErrorString(err_code);
		throw gl_error( err.str().c_str(), err_code);
		
	}
	#endif
}

gl_error::gl_error( const char* msg, const GLenum err)
	: std::runtime_error(msg), error( err)
{
}

gl_error::gl_error( const char* msg)
	: std::runtime_error(msg), error( GL_NO_ERROR)
{
}
