#ifndef VPYTHON_UTIL_DISPLAYLIST_HPP
#define VPYTHON_UTIL_DISPLAYLIST_HPP

// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include <boost/shared_ptr.hpp>
#include "util/gl_free.hpp"

namespace cvisual {

using boost::shared_ptr;

/** A manager for OpenGL displaylists */
class displaylist : public SigC::Object
{
 private:
	/// A unique identifier for objects of this type
	shared_ptr<unsigned int> handle;
	static void deleter( unsigned int*);
 
	/** Release any OpenGL resources associated with this object, even though
	 * it has not been deleted.  Postcondition: operator bool() returns false.
	 */
	void gl_free();

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
	inline operator bool() const { return (handle && *handle);  }
};

} // !namespace cvisual

#endif // !defined VPYTHON_UTIL_DISPLAYLIST_HPP
