#ifndef VPYTHON_RING_HPP
#define VPYTHON_RING_HPP

#include "simple_displayobject.hpp"

class ring : public simple_displayobject
{
 private:
	// The radius of the ring's body.  If not specified, it is set to 1/10 of
	// the radius of the body.
	double thickness;
	// The radius of the ring's hoop.
	double radius;

 public:
	ring();
	void set_radius( double r);
	void set_thickness( double t);
	
 protected:
	virtual void gl_render( const view&);
	virtual void grow_extent( extent&);
};

#endif
