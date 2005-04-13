// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "curve.hpp"
#include "util/errors.hpp"

#include <boost/crc.hpp>

#include <iostream>

#include <GL/gle.h>

namespace cvisual {

bool
curve::degenerate() const
{
	return pos.size() != color.size() || pos.size() < 3;
}

namespace {
// Determines if two values differ by more than frac of either one.
bool
eq_frac( double lhs, double rhs, double frac)
{
	if (lhs == rhs)
		return true;
	lhs = fabs(lhs);
	rhs = fabs(rhs);
	double diff = fabs(lhs - rhs);
	return lhs*frac > diff && rhs*frac > diff;
}
} // !namespace (unnamed)

bool
curve::closed_path() const
{
	// Determines if the curve follows a closed path or not.  The case where 
	// this fails is when the beginning and end are:
	//   very close together.
	//   and very close to the origin.
	//   and the scope of the entire curve is large relative to the points' 
	//     proximity to the origin.
	// In this case, it returns false when it should be true.
	vector begin = pos[1];
	vector end = pos.back();
	return eq_frac(begin.x, end.x, 1e-5) 
		&& eq_frac(begin.y, end.y, 1e-5) 
		&& eq_frac(begin.z, end.z, 1e-5);
}


long
curve::checksum( size_t begin, size_t end)
{
	boost::crc_32_type engine;
	engine.process_block( &(color.begin()+1+begin)->red, &(color.begin()+1+end)->red);
	engine.process_block( &(pos.begin()+begin)->x, &(pos.begin()+end+2)->x);
	return engine.checksum();
}

curve::curve()
	: pos(1), color(1), radius(0), monochrome(true)
{
	renderable::color = rgba(0,0,0,.5);
}

void
curve::append( vector n_pos, rgb n_color)
{
	monochrome = false;
	if (color.size()%256 == 1)
		cache.push_back( c_cache());
	pos.push_back( n_pos);
	color.push_back( n_color);
}

void
curve::append( vector n_pos)
{
	if (color.size()%256 == 1)
		cache.push_back( c_cache());
	pos.push_back( n_pos);
	color.push_back( color.back());
}

void 
curve::set_radius( double r)
{ 
	radius = r;
	renderable::color = (radius == 0.0) ? rgba(0,0,0,.5) : rgba(0,0,0,1);
}


void 
curve::gl_render( const view& scene)
{
	if (degenerate())
		return;
	const size_t true_size = pos.size()-1;
	// Set up the leading and trailing points for the joins.  See 
	// glePolyCylinder() for details.  The intent is to create joins that are
	// perpendicular to the path at the last segment.  When the path appears
	// to be closed, it should be rendered that way on-screen.
	bool closed = closed_path();
	if (closed) {
		pos[0] = *(pos.end()-2);
		pos.push_back( pos[2]);
		color[0] = color.back();
		color.push_back( color[1]);
	}
	else {
		pos[0] = pos[1] - (pos[2]-pos[1]);
		pos.push_back( pos.back() + pos.back() - pos[pos.size()-2]);
		color[0] = color[1];
		color.push_back( color.back());		
	}

	clear_gl_error();
	static bool first = true;
	if (first) {
		gleSetNumSides(6);
		gleSetJoinStyle( TUBE_JN_ANGLE | TUBE_NORM_PATH_EDGE);
		first = false;
	}
	
	size_t size = std::min(256u, true_size);
	size_t begin = 0;
	cache_iterator c = cache.begin();
	const bool do_thinline = (radius == 0.0);
	if (do_thinline) {
		glEnableClientState( GL_VERTEX_ARRAY);
		glDisable( GL_LIGHTING);
		if (!monochrome)
			glEnableClientState( GL_COLOR_ARRAY);
		glEnable( GL_BLEND);
		glEnable( GL_LINE_SMOOTH);
		glBlendFunc( GL_SRC_ALPHA, GL_ONE);	
	}
	while (size > 1) {
		long check = checksum( begin, begin+size);
		if (check == c->checksum)
			c->gl_cache.gl_render();
		else {
			c->gl_cache.gl_compile_begin();
			if (do_thinline)
				thinline( scene, begin, begin+size);
			else
				thickline( scene, begin, begin+size);
			c->gl_cache.gl_compile_end();
			c->checksum = check;
			c->gl_cache.gl_render();
		}
		begin += size-1;
		size = (begin + 256 < true_size)
			? 256 : true_size-begin;
		++c;
	}
	if (do_thinline) {
		glDisableClientState( GL_VERTEX_ARRAY);
		glDisableClientState( GL_COLOR_ARRAY);
		glEnable( GL_LIGHTING);
		glDisable( GL_BLEND);
		glDisable( GL_LINE_SMOOTH);
	}
	check_gl_error();
	
	pos.pop_back();
	color.pop_back();
}

vector
curve::get_center() const
{
	if (pos.size() == 1)
		return vector();
	vector ret;
	for (std::vector<vector>::const_iterator i = pos.begin()+1; 
			i != pos.end(); ++i) {
		ret += *i;
	}
	ret /= pos.size()-1;
	return ret;
}

void
curve::gl_pick_render( const view& scene)
{
	// Aack, I can't think of any obvious optimizations here
	gl_render( scene);
}

void
curve::grow_extent( extent& world)
{
	for (std::vector<vector>::iterator i = pos.begin()+1; i != pos.end(); ++i) {
		world.add_point(*i);
	}
}

void
curve::thinline( const view& scene, size_t begin, size_t end)
{
	// The following may be empty, but they are kept at this scope to ensure
	// that their memory will be valid when rendering the body below.
	std::vector<vector> spos;
	std::vector<rgb> tcolor;
	
	if (scene.gcf != 1.0) {
		// Must scale the pos data.
		std::vector<vector> _tmp( pos.begin()+begin+1, pos.begin()+1+end);
		spos.swap( _tmp);
		for (std::vector<vector>::iterator i = spos.begin(); 
				i < spos.end(); ++i) {
			*i *= scene.gcf;
		}
		glVertexPointer( 3, GL_DOUBLE, 0, &*(spos.begin()));
	}
	else {
		glVertexPointer( 3, GL_DOUBLE, 0, &*(pos.begin()+begin+1));
	}
	
	if (monochrome) {
		// Can get away without using a color array?
		if (scene.anaglyph) {
			if (scene.coloranaglyph)
				color[1].desaturate().gl_set();
			else
				color[1].grayscale().gl_set();
		}
		else
			color[1].gl_set();
	}
	else {
		if (scene.anaglyph) {
			// Must desaturate or grayscale the color.
			std::vector<rgb> _tmp(color.begin()+begin+1, color.begin()+1+end);
			tcolor.swap( _tmp);
			for (std::vector<rgb>::iterator i = tcolor.begin(); 
					i < tcolor.end(); ++i) {
				if (scene.coloranaglyph)
					*i = i->desaturate();
				else
					*i = i->grayscale();
			}
			glColorPointer( 3, GL_FLOAT, 0, &*tcolor.begin());
		}
		else
			glColorPointer( 3, GL_FLOAT, 0, &*(color.begin()+begin+1));
	}
	glDrawArrays( GL_LINE_STRIP, 0, end-begin);
}

namespace {
template <typename T>
struct converter
{
	T data[3];
};
} // !namespace (anonymous)

void
curve::thickline( const view& scene, size_t begin, size_t end)
{
	std::vector<rgb> tcolor;
	std::vector<vector> spos;
	float (*used_color)[3] = &((converter<float>*)&*(color.begin()+begin))->data;
	double (*used_pos)[3] = &((converter<double>*)&*(pos.begin()+begin))->data;
	
	
	if (scene.gcf != 1.0) {
		// Must scale the pos data.
		std::vector<vector> tmp(pos.begin()+begin, pos.begin()+end+2);
		spos.swap( tmp);
		for (std::vector<vector>::iterator i = spos.begin(); 
				i < spos.end(); ++i) {
			*i *= scene.gcf;
		}
		used_pos = &((converter<double>*)&*spos.begin())->data;
	}
	if (monochrome) {
		// Can get away without using a color array.
		if (scene.anaglyph) {
			if (scene.coloranaglyph)
				color[1].desaturate().gl_set();
			else
				color[1].grayscale().gl_set();
		}
		else
			color[1].gl_set();
		
		glePolyCylinder( 
			end-begin+2, used_pos,
			0, radius * scene.gcf);
	}
	else {
		if (scene.anaglyph) {
			// Must desaturate or grayscale the color.
			std::vector<rgb> tmp( color.begin()+begin, color.end()+end+2);
			tcolor.swap(tmp);
			for (std::vector<rgb>::iterator i = tcolor.begin(); 
					i < tcolor.end(); ++i) {
				if (scene.coloranaglyph)
					*i = i->desaturate();
				else
					*i = i->grayscale();
			}
			used_color = &((converter<float>*)&*tcolor.begin())->data;
		}
		
		glePolyCylinder( 
			end-begin+2, used_pos,
			used_color, radius * scene.gcf);
	}
}

} // !namespace cvisual
