#ifndef VPYTHON_CONE_HPP
#define VPYTHON_CONE_HPP

#include "simple_displayobject.hpp"

class cone : public simple_displayobject
{
 private:
	// The radius of the base of the cone.
	double radius;
	static bool first;
	bool degenerate();
	
 public:
	cone();
	void set_radius( double r);
	
 protected:
	virtual void gl_pick_render( const view&);
	virtual void gl_render( const view&);
	virtual void update_cache( const view&);
	virtual void grow_extent( extent&);
	virtual vector get_center() const;
	SIMPLE_DISPLAYOBJECT_TYPEINFO_DECL;
};

#endif // !defined VPYTHON_CONE_HPP
