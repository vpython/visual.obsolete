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
	long checksum( size_t begin, size_t end);
	
	struct c_cache 
	{
		static const size_t items = 256;
		displaylist gl_cache;
		long checksum;
	};
	std::vector<c_cache> cache;
	typedef std::vector<c_cache>::iterator cache_iterator;

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
	
	void thinline( const view&, size_t begin, size_t end);
	void thickline( const view&, size_t begin, size_t end);
};


#endif // !defined VPYTHON_CURVE_HPP
