#include "faces.hpp"

bool
faces::degenerate() const
{
	return pos.size() < 3 
		// && pos.size() % 3 == 0
		|| pos.size() != color.size()
		|| pos.size() != normal.size();
}

faces::faces()
{
}

void
faces::append( const vector& p, const vector& n)
{
	pos.push_back( p);
	normal.push_back(n);
	color.push_back( color.back());
}

void
faces::append( const vector& p, const vector& n, const rgba& c)
{
	pos.push_back( p);
	normal.push_back( n);
	color.push_back( c);
}

// TODO: Finish bsp_tree and support transparency for this object.
void 
faces::gl_render( const view& scene)
{
	if (degenerate())
		return;
	
	std::vector<vector> spos;
	std::vector<rgba> tcolor;
	
	glEnableClientState( GL_VERTEX_ARRAY);
	glEnableClientState( GL_NORMAL_ARRAY);
	glEnableClientState( GL_COLOR_ARRAY);
	
	glNormalPointer( GL_DOUBLE, 0, &*normal.begin());
	
	if (scene.gcf != 1.0) {
		spos = pos;
		for (std::vector<vector>::iterator i = spos.begin(); i != spos.end(); ++i) {
			*i *= scene.gcf;
		}
		glVertexPointer( 3, GL_DOUBLE, 0, &*spos.begin());
	}
	else
		glVertexPointer( 3, GL_DOUBLE, 0, &*pos.begin());
		
	if (scene.anaglyph) {
		tcolor = color;
		for (std::vector<rgba>::iterator i = tcolor.begin(); i != tcolor.end(); ++i) {
			if (scene.coloranaglyph)
				*i = i->desaturate();
			else
				*i = i->grayscale();
		}
		glColorPointer( 3, GL_FLOAT, sizeof(float)*4, &*tcolor.begin());
	}
	else
		glColorPointer( 3, GL_FLOAT, sizeof(float)*4, &*color.begin());
		
	glDrawArrays( GL_TRIANGLES, 0, pos.size());
	
	glDisableClientState( GL_COLOR_ARRAY);
	glDisableClientState( GL_NORMAL_ARRAY);
	glDisableClientState( GL_VERTEX_ARRAY);
}

void 
faces::gl_pick_render( const view& scene)
{
	gl_render( scene);
}

vector 
faces::get_center() const
{
	vector ret;
	for (std::vector<vector>::const_iterator i = pos.begin(); i != pos.end(); ++i) {
		ret += *i;
	}
	if (!pos.empty())
		ret /= pos.size();
	return ret;
}

void 
faces::grow_extent( extent& world)
{
	for (std::vector<vector>::const_iterator i = pos.begin(); i != pos.end(); ++i) {
		world.add_point( *i);
	}
}
