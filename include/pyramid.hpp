#ifndef VPYTHON_PYRAMID_HPP
#define VPYTHON_PYRAMID_HPP

#include "simple_displayobject.hpp"
#include "util/displaylist.hpp"
#include "util/sorted_model.hpp"

#include <boost/scoped_ptr.hpp>
using boost::scoped_ptr;

class pyramid : public simple_displayobject
{
 private:
	double width; // size along the z axis
	double height; // size along the y axis.
	static displaylist simple_model;
	static scoped_ptr< z_sorted_model<triangle, 6> > sorted_model;
	SIMPLE_DISPLAYOBJECT_TYPEINFO_DECL;
	
 public:
	pyramid();
	void set_width( double width);
	void set_height( double height);
	void set_length( double length);
	
 protected:
	virtual void update_cache( const view&);
	virtual void gl_render( const view&);
	virtual void grow_extent( extent&);
	virtual vector get_center() const;
};

#endif // !defined VPYTHON_PYRAMID_HPP
