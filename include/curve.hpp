#ifndef VPYTHON_CURVE_HPP
#define VPYTHON_CURVE_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "renderable.hpp"
#include "util/displaylist.hpp"

#include <vector>

namespace cvisual {

class curve : public renderable
{
 private:
	std::vector<vector> pos;
	std::vector<rgb> color;
	double radius;
	bool monochrome;
 
 	bool degenerate() const;
 	bool closed_path() const;
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
	void append( vector pos, rgb color);
	void append( vector pos);
	void set_radius( double r);
 
 protected:
	virtual void gl_render( const view&);
	virtual vector get_center() const;
	virtual void gl_pick_render( const view&);
	virtual void grow_extent( extent&);
	
	void thinline( const view&, size_t begin, size_t end);
	void thickline( const view&, size_t begin, size_t end);
};

} // !namespace cvisual

#endif // !defined VPYTHON_CURVE_HPP
