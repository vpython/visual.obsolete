#ifndef VPYTHON_ELLIPSOID_HPP
#define VPYTHON_ELLIPSOID_HPP

#include "sphere.hpp"

class ellipsoid : public sphere
{
 private:
	double width;
	double height;
	
 public:
	ellipsoid();
	void set_width( double width);
	void set_height( double height);
	void set_length( double length);
	
 protected:
	virtual vector get_scale();
	virtual bool degenerate();
	SIMPLE_DISPLAYOBJECT_TYPEINFO_DECL;
};


#endif // !defined VPYTHON_ELLIPSOID_HPp
