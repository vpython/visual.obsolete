// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "util/clipping_plane.hpp"

#include "util/tmatrix.hpp"
#include "util/rgba.hpp"
#include "util/errors.hpp"

#include <utility>
namespace cvisual { namespace {

// A singleton class used to manage the six available OpenGL clipping planes.
class c_planes
{
 private:
	// The boolean half is true if the plane ID has been allocated by an
	// instance of clipping_plane, and false otherwise.  The GLenum is present
	// to allow algorithmic access to the named parameter rather than with
	// switch/case, or something like that.
	std::pair<GLenum, bool> allocated[6];
 
 public:	 
	c_planes()
	{
		using std::make_pair;
		allocated[0] = make_pair( GL_CLIP_PLANE0, false);
		allocated[1] = make_pair( GL_CLIP_PLANE1, false);
		allocated[2] = make_pair( GL_CLIP_PLANE2, false);
		allocated[3] = make_pair( GL_CLIP_PLANE3, false);
		allocated[4] = make_pair( GL_CLIP_PLANE4, false);
		allocated[5] = make_pair( GL_CLIP_PLANE5, false);
	}
	
	// Returns a unique identifier for a new clipping plane
	size_t get_handle()
	{
		for (size_t i = 0; i < 6; ++i) {
			if (allocated[i].second == false) {
				allocated[i].second = true;
				return i;
			}
		}
		throw gl_error( "Maxiumum number of clipping planes exceeded.");
	}
	
	// Looks up the appropriate GLenum that corrisponds to a particular handle
	GLenum operator[]( size_t handle) const
	{
		assert( handle < 6);
		assert( allocated[handle].second == true);
		return allocated[handle].first;
	}
	
	// Returns the unused handle to the pool.
	void destroy_handle( size_t handle)
	{
		assert( handle < 6);
		allocated[handle].second = false;
	}
	
} planes;

} // !namespace (unnamed)

clipping_plane::clipping_plane( vector point, vector normal)
	: id(planes.get_handle())
{
	equation[0] = normal.x;
	equation[1] = normal.y;
	equation[2] = normal.z;
	equation[3] = -(normal * point).sum();
	glClipPlane( planes[id], equation);
}

clipping_plane::~clipping_plane()
{
	planes.destroy_handle(id);
}

void
clipping_plane::gl_enable()
{
	glEnable( planes[id]);
}

void
clipping_plane::gl_disable()
{
	glDisable( planes[id]);
}

} // !namespace cvisual
