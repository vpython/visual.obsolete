#ifndef VPYTHON_UTIL_GL_FREE_HPP
#define VPYTHON_UTIL_GL_FREE_HPP

#include <sigc++/signal.h>
#include <sigc++/object.h>

namespace cvisual {

// When the last open OpenGL window is closed, this signal will be emitted to
// allow any texture and displaylist resources to be freed.
extern SigC::Signal0<void> on_gl_free;
	
} // !namespace cvisual

#endif // !defined VPYTHON_UTIL_GL_FREE_HPP
