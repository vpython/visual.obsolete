#ifndef VPYTHON_UTIL_GL_FREE_HPP
#define VPYTHON_UTIL_GL_FREE_HPP

#include <boost/signals.hpp>

namespace cvisual {

// When the last open OpenGL window is closed, this signal will be emitted to
// allow any texture and displaylist resources to be freed.
extern boost::signal<void()> on_gl_free;
	
} // !namespace cvisual

#endif // !defined VPYTHON_UTIL_GL_FREE_HPP
