#include "curve.hpp"
#include "util/errors.hpp"

#include <GL/gle.h>

bool
curve::degenerate()
{
	return pos.size() != color.size()+1 || pos.size() < 3;
}

namespace {

inline void
checksum_double( long& sum, const double* d)
{
	const unsigned char* p = (const unsigned char*)d;
	for (unsigned int i = 0; i < sizeof(double); i++) {
		sum ^= *p;
		p++;
		if (sum < 0)
			sum = (sum << 1) | 1;
		else
			sum = sum << 1;
	}
}

inline void
checksum_float( long& sum, const float* f)
{
	const unsigned char* p = (const unsigned char*)f;
	for (unsigned int i = 0; i < sizeof(float); i++) {
		sum ^= *p;
		p++;
 		if (sum < 0)
 			sum = (sum << 1) | 1;
		else
			sum = sum << 1;
	}

}

} // !namespace (unnamed)

long
curve::checksum( size_t begin, size_t end)
{
	long ret = 0;
	
	checksum_double( ret, &radius);
	for (std::vector<rgba>::const_iterator i = color.begin()+begin; 
			i < color.begin()+end; ++i) {
		checksum_float( ret, &i->red);
		checksum_float( ret, &i->green);
		checksum_float( ret, &i->blue);
		checksum_float( ret, &i->alpha);
	}
	for (std::vector<vector>::const_iterator i = pos.begin()+begin; 
			i < pos.begin()+end+1; ++i) {
		checksum_double( ret, &i->x);
		checksum_double( ret, &i->y);
		checksum_double( ret, &i->z);
	}
	if (end != pos.size()+1) {
		checksum_double( ret, &(pos.begin()+end+2)->x);
		checksum_double( ret, &(pos.begin()+end+2)->y);
		checksum_double( ret, &(pos.begin()+end+2)->z);
	}
	return ret;
}

curve::curve()
	: pos(1), radius(0), monochrome(true)
{
}

void
curve::append( vector n_pos, rgba n_color)
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
curve::gl_render( const view& scene)
{
	clear_gl_error();
	static bool first = true;
	if (first) {
		gleSetNumSides(6);
		gleSetJoinStyle( TUBE_JN_ANGLE | TUBE_NORM_PATH_EDGE);
		first = false;
	}
	if (degenerate())
		return;
	
	size_t next_size = std::min(257u, color.size());
	size_t current_begin = 0;
	cache_iterator c = cache.begin();
	const bool do_thinline = (radius == 0.0);
	if (do_thinline) {
		glEnableClientState( GL_VERTEX_ARRAY);
		glDisable( GL_LIGHTING);
		if (!monochrome)
			glEnableClientState( GL_COLOR_ARRAY);		
	}
	while (next_size > 1) {
		long check = checksum( current_begin, current_begin+next_size);
		if (check == c->checksum)
			c->gl_cache.gl_render();
		else {
			c->gl_cache.gl_compile_begin();
			if (do_thinline)
				thinline( scene, current_begin, current_begin+next_size);
			else
				thickline( scene, current_begin, current_begin+next_size);
			c->gl_cache.gl_compile_end();
			c->checksum = check;
			c->gl_cache.gl_render();
		}
		current_begin += next_size-1;
		next_size = (current_begin + 256 < color.size()) 
			? 256 : color.size()-current_begin;
		++c;
	}
	if (do_thinline) {
		glDisableClientState( GL_VERTEX_ARRAY);
		glDisableClientState( GL_COLOR_ARRAY);
		glEnable( GL_LIGHTING);
	}
	check_gl_error();
}

vector
curve::get_center() const
{
	vector ret;
	for (std::vector<vector>::const_iterator i = pos.begin()+1; 
			i != pos.end(); ++i) {
		ret += *i;
	}
	ret /= pos.empty() ? 1 : pos.size();
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
	std::vector<rgba> tcolor;
	
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
				color[0].desaturate().gl_set();
			else
				color[0].grayscale().gl_set();
		}
		else
			color[0].gl_set();
	}
	else {
		if (scene.anaglyph) {
			// Must desaturate or grayscale the color.
			std::vector<rgba> _tmp(color.begin()+begin, color.begin()+end);
			tcolor.swap( _tmp);
			for (std::vector<rgba>::iterator i = tcolor.begin(); 
					i < tcolor.end(); ++i) {
				if (scene.coloranaglyph)
					*i = i->desaturate();
				else
					*i = i->grayscale();
			}
			glColorPointer( 4, GL_FLOAT, 0, &*tcolor.begin());
		}
		else
			glColorPointer( 4, GL_FLOAT, 0, &*(color.begin()+begin));
	}
	glDrawArrays( GL_LINE_STRIP, 0, end-begin);
}

void
curve::thickline( const view& scene, size_t begin, size_t end)
{
	std::vector<rgba> tcolor;
	std::vector<vector> spos;
	float (*used_color)[4] = &((color.begin()+begin)->data);
	double (*used_pos)[3] = &((pos.begin()+begin)->data);
	
	// Set up the leading and trailing points for the joins.  See 
	// glePolyCylinder() for details
	if (begin == 0)
		pos[0] = pos[1] - (pos[2]-pos[1]);
	if (end == pos.size()) {
		pos.push_back( pos.back() + pos.back() - pos[pos.size()-2]);
	}
	
	if (scene.gcf != 1.0) {
		// Must scale the pos data.
		std::vector<vector> tmp(pos.begin()+begin, pos.begin()+end+2);
		spos.swap( tmp);
		for (std::vector<vector>::iterator i = spos.begin(); 
				i < spos.end(); ++i) {
			*i *= scene.gcf;
		}
		used_pos = &(spos.begin()->data);
	}
	if (monochrome) {
		// Can get away without using a color array.
		if (scene.anaglyph) {
			if (scene.coloranaglyph)
				color[0].desaturate().gl_set();
			else
				color[0].grayscale().gl_set();
		}
		else
			color[0].gl_set();
		
		glePolyCylinder( 
			end-begin+2, used_pos,
			0, radius * scene.gcf);
	}
	else {
		if (scene.anaglyph) {
			// Must desaturate or grayscale the color.
			std::vector<rgba> tmp( color.begin()+begin, color.end()+end);
			tcolor.swap(tmp);
			for (std::vector<rgba>::iterator i = tcolor.begin(); 
					i < tcolor.end(); ++i) {
				if (scene.coloranaglyph)
					*i = i->desaturate();
				else
					*i = i->grayscale();
			}
			used_color = &tcolor.begin()->data;
		}
		
		glePolyCylinder_c4f( 
			end-begin+2, used_pos,
			used_color, radius * scene.gcf);
		
	}
	if (end == pos.size())
		pos.pop_back();
}
