#ifndef VPYTHON_UTIL_DISPLAYLIST_HPP
#define VPYTHON_UTIL_DISPLAYLIST_HPP

#include <boost/noncopyable.hpp>

// A manager for OpenGL displaylists
class displaylist : boost::noncopyable
{
 private:
	// A unique identifier for objects of this type
	unsigned int handle;
 
 public:
	displaylist();
	~displaylist();
	// Generate a new displaylist without rendering it.
	void gl_compile_begin();
	// All calls to gl_compile_begin() must box a block with gl_compile_end().
	void gl_compile_end();
	// Perform the OpenGL commands cached with gl_compile_begin() and 
	// gl_compile_end().
	void gl_render() const;
	inline operator bool() const { return handle != 0; }
};

#endif // !defined VPYTHON_UTIL_DISPLAYLIST_HPP
