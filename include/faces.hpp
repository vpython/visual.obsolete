#ifndef VPYTHON_FACES_HPP
#define VPYTHON_FACES_HPP

#include "renderable.hpp"

#include <vector>

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

#endif // !defined VPYTHON_FACES_HPP
