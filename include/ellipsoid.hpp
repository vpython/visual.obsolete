#ifndef VPYTHON_ELLIPSOID_HPP
#define VPYTHON_ELLIPSOID_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "sphere.hpp"

namespace cvisual {

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
	PRIMITIVE_TYPEINFO_DECL;
};

} // !namespace cvisual

#endif // !defined VPYTHON_ELLIPSOID_HPp
