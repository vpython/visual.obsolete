#ifndef VPYTHON_UTIL_ERRORS_HPP
#define VPYTHON_UTIL_ERRORS_HPP

#include <string>
#include <stdexcept>
#include <GL/gl.h>

#define VPYTHON_CRITICAL_ERROR(msg) write_critical( __FILE__, __LINE__, \
	__PRETTY_FUNCTION__, msg)
#define VPYTHON_WARNING(msg) write_warning( __FILE__, __LINE__, \
	__PRETTY_FUNCTION__, msg)
	
void
write_critical( 
	std::string file, int line, std::string function, std::string message);
	
void
write_warning(
	std::string file, int line, std::string function, std::string message);
	
void
dump_glmatrix();

// Clears the OpenGL error state.  If NDEBUG is set, this function is a no-op.
void
clear_gl_error_real( void);

// Checks the OpenGL error state and throws gl_error if it is anything other
// than GL_NO_ERROR.  If NDEBUG is set, this function is a no-op.
void
check_gl_error_real(const char* file, int line);

// Forward the call to the real function.
#ifdef NDEBUG
# define check_gl_error() do {} while (false)
# define clear_gl_error() do {} while (false)
#else
# define check_gl_error() check_gl_error_real(__FILE__, __LINE__)
# define clear_gl_error() clear_gl_error_real()
#endif

class gl_error : public std::runtime_error
{
 private:
	GLenum error;
 public:
	inline GLenum 
	get_error_code() const 
	{ return error; }
	
	gl_error( const char* msg, const GLenum code);
	gl_error( const char* msg);
};

#endif // !defined VPYTHON_UTIL_ERRORS_HPP
