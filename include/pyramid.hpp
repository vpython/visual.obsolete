#ifndef VPYTHON_PYRAMID_HPP
#define VPYTHON_PYRAMID_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "rectangular.hpp"
#include "util/displaylist.hpp"
#include "util/sorted_model.hpp"

#include <boost/scoped_ptr.hpp>

namespace cvisual {

using boost::scoped_ptr;

class pyramid : public rectangular
{
 private:
	static displaylist simple_model;
	static scoped_ptr< z_sorted_model<triangle, 6> > sorted_model;
	PRIMITIVE_TYPEINFO_DECL;
	
 public:
	pyramid();
	pyramid( const pyramid& other);
	
 protected:
	virtual void gl_pick_render( const view&);
	virtual void update_cache( const view&);
	virtual void gl_render( const view&);
	virtual void grow_extent( extent&);
	virtual vector get_center() const;
};

} // !namespace cvisual

#endif // !defined VPYTHON_PYRAMID_HPP
