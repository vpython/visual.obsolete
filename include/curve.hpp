#ifndef VPYTHON_CURVE_HPP
#define VPYTHON_CURVE_HPP

#include "renderable.hpp"
#include "util/displaylist.hpp"

#include <vector>

class curve : public renderable
{
 private:
	std::vector<vector> pos;
	std::vector<rgba> color;
	double radius;
	bool monochrome;
 
 	bool degenerate();
	displaylist cache;
	long checksum();
	long last_checksum;

 public:
	curve();
	void append( vector pos, rgba color);
	void append( vector pos);
	void set_radius( double r) { radius = r; }
 
 protected:
	virtual void gl_render( const view&);
	virtual vector get_center() const;
	virtual void gl_pick_render( const view&);
	virtual void grow_extent( extent&);
	
	void thinline( const view&);
	void thickline( const view&);
};


#endif // !defined VPYTHON_CURVE_HPP
