#ifndef VPYTHON_FACES_HPP
#define VPYTHON_FACES_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "renderable.hpp"

#include <vector>

namespace cvisual {

class faces : public renderable
{
 private:
	std::vector<vector> pos;
	std::vector<vector> normal;
	std::vector<rgba> color;
	
	bool degenerate() const;
	
 public:
	faces();
	void append( const vector& p, const vector& n);
	void append( const vector& p, const vector& n, const rgba& c);
	
	
 protected:
	virtual void gl_render( const view&);
	virtual void gl_pick_render( const view&);
	virtual vector get_center() const;
	virtual void grow_extent( extent&);
};

} // !namespace cvisual

#endif // !defined VPYTHON_FACES_HPP
