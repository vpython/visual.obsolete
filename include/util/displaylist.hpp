#ifndef VPYTHON_UTIL_DISPLAYLIST_HPP
#define VPYTHON_UTIL_DISPLAYLIST_HPP

#include <boost/noncopyable.hpp>

/** A manager for OpenGL displaylists */
class displaylist : boost::noncopyable
{
 private:
	/// A unique identifier for objects of this type
	unsigned int handle;
 
 public:
	displaylist();
	~displaylist();
	/** Begin compiling a new displaylist.  Nothing is drawn to the screen
 		when rendering commands into the displaylist.  Be sure to call 
 		gl_compile_end() when you are done.
 		*/
	void gl_compile_begin();
	/** Completes compiling the displaylist. */
	void gl_compile_end();
	/** Perform the OpenGL commands cached with gl_compile_begin() and 
		gl_compile_end(). */
	void gl_render() const;
	/** @return true iff this object contains a compiled OpenGL program. */
	inline operator bool() const { return handle != 0; }
};

#endif // !defined VPYTHON_UTIL_DISPLAYLIST_HPP
