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
curve::checksum()
{
	long ret = 0;
	
	checksum_double( ret, &radius);
	for (std::vector<rgba>::const_iterator i = color.begin(); i < color.end(); ++i) {
		checksum_float( ret, &i->red);
		checksum_float( ret, &i->green);
		checksum_float( ret, &i->blue);
		checksum_float( ret, &i->alpha);
	}
	for (std::vector<vector>::const_iterator i = pos.begin(); i < pos.end(); ++i) {
		checksum_double( ret, &i->x);
		checksum_double( ret, &i->y);
		checksum_double( ret, &i->z);
	}
	return ret;
}

curve::curve()
	: pos(1), radius(0), monochrome(true), last_checksum(0)
{
}

void
curve::append( vector n_pos, rgba n_color)
{
	monochrome = false;
	pos.push_back( n_pos);
	color.push_back( n_color);
}

void
curve::append( vector n_pos)
{
	pos.push_back( n_pos);
	color.push_back( *color.end());
}

// TODO: Change gle to use per-vertex normals rather than per-face normals for
// glePolyCylinder().

void 
curve::gl_render( const view& scene)
{
	static bool first = true;
	if (first) {
		gleSetNumSides(7);
		first = false;
	}
	if (degenerate())
		return;
	
	clear_gl_error();
	long check = checksum();
	
	if (check == last_checksum) {
		cache.gl_render();
	}
	else {
		cache.gl_compile_begin();
		if (radius)
			thickline(scene);
		else
			thinline(scene);
		cache.gl_compile_end();
		cache.gl_render();
		last_checksum = check;
	}
	check_gl_error();
}

vector
curve::get_center() const
{
	vector ret;
	for (std::vector<vector>::const_iterator i = pos.begin()+1; i != pos.end(); ++i) {
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
curve::thinline( const view& scene)
{
	// The following may be empty, but they are kept at this scope to ensure
	// that their memory will be valid when rendering the body below.
	std::vector<vector> spos;
	std::vector<rgba> tcolor;
	glEnableClientState( GL_VERTEX_ARRAY);
	glDisable( GL_LIGHTING);
	
	if (scene.gcf != 1.0) {
		// Must scale the pos data.
		spos = pos;
		for (std::vector<vector>::iterator i = spos.begin(); i < spos.end(); ++i) {
			*i *= scene.gcf;
		}
		glVertexPointer( 3, GL_DOUBLE, 0, &*(spos.begin()+1));
	}
	else {
		glVertexPointer( 3, GL_DOUBLE, 0, &*(pos.begin()+1));
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
	}
	else {
		glEnableClientState( GL_COLOR_ARRAY);
		if (scene.anaglyph) {
			// Must desaturate or grayscale the color.
			tcolor = color;
			for (std::vector<rgba>::iterator i = tcolor.begin(); i < tcolor.end(); ++i) {
				if (scene.coloranaglyph)
					*i = i->desaturate();
				else
					*i = i->grayscale();
			}
			glColorPointer( 4, GL_FLOAT, 0, &*tcolor.begin());
		}
		else
			glColorPointer( 4, GL_FLOAT, 0, &*color.begin());
	}
	
	size_t points = pos.size()-1;
	size_t index = 0;
	while (points > 1024) {
		glDrawArrays( GL_LINE_STRIP, index, 1024);
		index += 1024;
		points -= 1024;
	}
	if (points)
		glDrawArrays( GL_LINE_STRIP, index, points);
	glDisableClientState( GL_VERTEX_ARRAY);
	glDisableClientState( GL_COLOR_ARRAY);
	glEnable( GL_LIGHTING);
}

void
curve::thickline( const view& scene)
{
	std::vector<rgba> tcolor;
	std::vector<vector> spos;
	std::vector<rgba>& used_color = color;
	std::vector<vector>& used_pos = pos;
	
	// Set up the leading and trailing points for the joins.  See 
	// glePolyCylinder() for details
	pos[0] = pos[1] - (pos[2]-pos[1]);
	size_t end = pos.size()-1;
	pos.push_back( pos[end] + pos[end] - pos[end-1]);
	
	if (scene.gcf != 1.0) {
		// Must scale the pos data.
		spos = pos;
		for (std::vector<vector>::iterator i = spos.begin(); i < spos.end(); ++i) {
			*i *= scene.gcf;
		}
		used_pos = spos;
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
			used_pos.size(), &(used_pos.begin()->data),
			0, radius * scene.gcf);
	}
	else {
		if (scene.anaglyph) {
			// Must desaturate or grayscale the color.
			tcolor = color;
			for (std::vector<rgba>::iterator i = tcolor.begin(); i < tcolor.end(); ++i) {
				if (scene.coloranaglyph)
					*i = i->desaturate();
				else
					*i = i->grayscale();
			}
			used_color = tcolor;
		}
		
		glePolyCylinder_c4f( 
			used_pos.size(), &(used_pos.begin()->data),
			&(used_color.begin()->data), radius * scene.gcf);
		
	}
	pos.pop_back();
}
