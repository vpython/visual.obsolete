#ifndef VPYTHON_RING_HPP
#define VPYTHON_RING_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "axial.hpp"

namespace cvisual {

class ring : public axial
{
 private:
	// The radius of the ring's body.  If not specified, it is set to 1/10 of
	// the radius of the body.
	double thickness;
	PRIMITIVE_TYPEINFO_DECL;
	bool degenerate();

 public:
	ring();
	ring( const ring& other);
	virtual ~ring();
	void set_thickness( double t);
	double get_thickness();
	
 protected:
	virtual void gl_pick_render( const view&);
	virtual void gl_render( const view&);
	virtual void grow_extent( extent&);
	
	void do_render_opaque( const view&, size_t rings, size_t bands);
	void do_render_translucent( const view&, size_t rings, size_t bands);
	
	void band_prepare( const view&, size_t, size_t);
	void gl_draw( const view&, size_t, size_t);
};

} // !namespace cvisual

#endif
