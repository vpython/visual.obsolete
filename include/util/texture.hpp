#ifndef VPYTHON_UTIL_TEXTURE_HPP
#define VPYTHON_UTIL_TEXTURE_HPP

#include <string>
#include <boost/shared_ptr.hpp>
using boost::shared_ptr;

// A class to assist in managing OpenGL texture resources.
// TODO: extend this class to represent the abstract interface that Visual
// objects need to render textures while providing facilities to base classes
// for implementation of various platform-specific texture loaders and 
// procedural texture-generators.

// This class (and its subclasses) should have initialize-on-first-use
// semantics.  Instantiation of one of the subclasses should perform first-stage
// initialization: decode the on-disk file, generate a procedural texture, 
// whatever.  The first time it is used (with gl_activate), the data is
// transfered to texture memory for subsequent use.  This class should be
// noncopyable due to its resource allocation/sharing model.  The base class 
// should be able to expose a damage and locking model for mutable textures as
// well as be immutable.  It is possible that the base class is so abstract as
// to be non-constructable from Python (simmilar to cvisual.displayobject now).
class texture
{
 private:
	bool damaged;

 public:
	// Release the handle to OpenGL.  Subclasses must not call glDeleteTextures()
	// on this class's handle.
	virtual ~texture();

	// True iff the texture object is managing something.
	operator bool() const;
 
	// Make this texture active.  This function constitutes use under the
	// "initialize on first use" rule.  Precondition: an OpenGL context must be
	// active.  
	void gl_activate();
 
 protected:
	// A unique identifier for the texture, to be obtained from glGenTextures().
	unsigned int handle;
 
	// Perform zero initialization of POD members.
	texture();
 
	// Make this class noncopyable.
	texture( const texture&);
	const texture& operator=( const texture&);
 
	// Called by gl_activate() on the first use and whenever damaged.
	// Postcondition: handle refers to an initialized OpenGL texture object.
	virtual void gl_init() = 0;
 
	// Perform any texture transformation matrix initialization that might be
	// required.  Default: do nothing.
	// This function must assume that the active matrix is GL_MODELVIEW and must
	// return in that state.
	virtual void gl_transform();
 
	// Mutable subclasses must call this function whenever their texture data
	// needs to be reloaded into OpenGL.
	void damage();
};

#endif
