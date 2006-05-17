// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "label.hpp"
#include "font.hpp"
#include "util/errors.hpp"

#include <sstream>
#include <iostream>

#include <boost/scoped_array.hpp>
using boost::scoped_array;

namespace cvisual {

label::label()
	: pos(mtx, 0, 0, 0),
	space(0),
	xoffset(0),
	yoffset(0),
	border(5),
	font_description("default"),
	font_size(0),
	box_enabled(true),
	line_enabled(true),
	linecolor( color),
	opacity(0.66)
{
}

label::label( const label& other)
	: renderable( other),
	pos( mtx, other.pos.x, other.pos.y, other.pos.z),
	space( other.space),
	xoffset( other.xoffset),
	yoffset( other.yoffset),
	border( other.border),
	font_description( other.font_description),
	font_size( other.font_size),
	box_enabled( other.box_enabled),
	line_enabled( other.line_enabled),
	linecolor( other.linecolor),
	opacity( other.opacity),
	text( other.text)
{
}

label::~label()
{
}

void
label::set_pos( const vector& n_pos)
{
	pos = n_pos;
}

shared_vector&
label::get_pos()
{
	return pos;
}

void
label::set_x( double x)
{
	pos.set_x( x);
}

double
label::get_x()
{
	return pos.x;
}

void
label::set_y( double y)
{
	pos.set_y( y);
}

double
label::get_y()
{
	return pos.y;
}

void
label::set_z( double z)
{
	pos.set_z( z);
}

double
label::get_z()
{
	return pos.z;
}

void 
label::set_color( const rgba& n_color)
{
	lock L(mtx);
	color = n_color;
}

rgba
label::get_color()
{
	return color;
}

void
label::set_red( double r)
{
	lock L(mtx);
	color.red = r;
}

double
label::get_red()
{
	return color.red;
}

void
label::set_green( double g)
{
	lock L(mtx);
	color.green = g;
}

double
label::get_green()
{
	return color.green;
}

void
label::set_blue( double b)
{
	lock L(mtx);
	color.blue = b;
}

double
label::get_blue()
{
	return color.blue;
}

void
label::set_alpha( double a)
{
	lock L(mtx);
	color.alpha = a;
}

double
label::get_alpha()
{
	return color.alpha;
}

void
label::set_opacity( double o)
{
	lock L(mtx);
	opacity = o;
}

double
label::get_opacity()
{
	return opacity;
}

void
label::set_text( std::string t)
{
	lock L(mtx);
	text.clear();
	std::istringstream buf( t);
	std::string line;
	while (std::getline( buf, line)) {;
		text.push_back( line);
	}
}

std::string
label::get_text()
{
	std::string ret;
	if (text.empty())
		return ret;
	ret += text.front();
	for (text_iterator i = text.begin()+1; i != text.end(); ++i) {
		ret += '\n';
		ret += *i;
	}
	return ret;
}

void 
label::set_space( double n_space)
{
	lock L(mtx);
	space = n_space;
}

double
label::get_space()
{
	return space;
}

void 
label::set_xoffset( double n_xoffset)
{
	lock L(mtx);
	xoffset = n_xoffset;
}

double
label::get_xoffset()
{
	return xoffset;
}

void 
label::set_yoffset( double n_yoffset)
{
	lock L(mtx);
	yoffset = n_yoffset;
}

double
label::get_yoffset()
{
	return yoffset;
}

void 
label::set_border( double n_border)
{
	lock L(mtx);
	border = n_border;
}

double
label::get_border()
{
	return border;
}

void 
label::set_font_family( std::string name)
{
	lock L(mtx);
	font_description = name;
}

std::string
label::get_font_family()
{
	return font_description;
}

void
label::set_font_size( double n_size)
{
	lock L(mtx);
	font_size = n_size;
}

double
label::get_font_size()
{
	return font_size;
}

void 
label::render_box( bool enable)
{
	lock L(mtx);
	box_enabled = enable;
}

bool
label::has_box()
{
	return box_enabled;
}

void 
label::render_line( bool enable)
{
	lock L(mtx);
	line_enabled = enable;
}

bool
label::has_line()
{
	return line_enabled;
}

void
label::set_linecolor( const rgba& n_color)
{
	lock L(mtx);
	linecolor = n_color;
}

rgba
label::get_linecolor()
{
	return linecolor;
}
#define NEWMODE 1
void
label::gl_render( const view& scene)
{
	// Compute the width of the text box.
	double box_width = 0.0;
	color.gl_set();
	// bitmap_font font( font_description);
	bitmap_font font;
	for (text_iterator i = text.begin(); i != text.end(); ++i) {
		double str_width = font.width(*i);
		if (str_width > box_width)
			box_width = str_width;
	}
	box_width += 2.0 * border;

	// Compute the positions of the text in the text box, and the height of the
	// text box.  The text positions are relative to the lower left corner of 
	// the text box.
	double box_height = border * 2.0 
		+ ((text.empty()) ? 1 : text.size()) *( -font.descent() + font.ascent());
	
	scoped_array<vector> text_pos( new vector[text.size()]);
	// vector text_pos[text.size()];
	vector* tpos_i = text_pos.get();
	vector* tpos_end = text_pos.get() + text.size();
	vector next_tpos( border, box_height - border - font.ascent());
	while (tpos_i != tpos_end) {
		*tpos_i = next_tpos; // (tpos_i - 1) - vector(0, font.ascent() - font.descent());
		next_tpos.y -= (font.ascent() - font.descent());
		++tpos_i;
	}

	clear_gl_error();
	vector label_pos = pos * scene.gcf;
	tmatrix lst = tmatrix().gl_projection_get() * tmatrix().gl_modelview_get();
	{
		tmatrix translate;
		translate.w_column( label_pos);
		lst = lst * translate;
	}
	vector origin = lst * vertex(vector(), 1.0);

	displaylist list;
	list.gl_compile_begin();
	{
		// Zero out the existing matrices, rendering will be in screen coords.
		gl_matrix_stackguard guard;
		tmatrix identity;
		identity.gl_load();
		glMatrixMode( GL_PROJECTION); { //< Zero out the projection matrix, too
		gl_matrix_stackguard guard2;
		identity.gl_load();
		
		glTranslated( origin.x, origin.y, origin.z);
		glScaled( 2.0/scene.window_width, 2.0/scene.window_height, 1.0);
		// At this point, all furthur translations are in direction of label
		//  space.
		if (space && (xoffset || yoffset)) {
			// Move the origin away from the body.
			vector space_offset = vector(xoffset, yoffset).norm() * std::fabs(space);
			glTranslated( space_offset.x, space_offset.y, space_offset.z);
		}
		// Optionally draw the line, and move the origin to the bottom left
		// corner of the text box.		
		if (xoffset || yoffset) {
			if (line_enabled) {
				glBegin( GL_LINES);
					linecolor.gl_set();
					vector().gl_render();
					vector(xoffset, yoffset).gl_render();
				glEnd();
			}
			if (std::fabs(xoffset) > std::fabs(yoffset)) {
				glTranslated( 
					xoffset + ((xoffset > 0) ? 0 : -box_width), 
					yoffset - box_height*0.5,
					0);
			}
			else {
				glTranslated( 
					xoffset - box_width*0.5,
					yoffset + ((yoffset > 0) ? 0 : -box_height),
					0);
			}
		}
		else {
			glTranslated( -box_width*0.5, -box_height*0.5, 0.0);
		}
		if (opacity) {
			// Occlude objects behind the label.
			glEnable( GL_BLEND);
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			rgba( 0, 0, 0, opacity).gl_set();
			glBegin( GL_QUADS);
				vector().gl_render();
				vector( box_width, 0).gl_render();
				vector( box_width, box_height).gl_render();
				vector( 0, box_height).gl_render();
			glEnd();
			glDisable( GL_BLEND);
		}
		if (box_enabled) {
			// Draw a box around the text.
			linecolor.gl_set();
			glBegin( GL_LINE_LOOP);
				vector().gl_render();
				vector( box_width, 0).gl_render();
				vector( box_width, box_height).gl_render();
				vector( 0, box_height).gl_render();
			glEnd();
		}
		
		// The following three lines work around a wierd problem.  Without them,
		// the first line of text below will not be rendered properly.  The physical
		// appearance is as if, for the first line of text, glRasterPos was far 
		// away in the visual depth direction.  I have only observed this issue 
		// with the ATI proprietary fglrx driver at this time - MESA's software
		// renderer is fine.  So is NVIDIA's Linux GLX driver.
		glRasterPos3d( 0, 0, 0);
		glPushAttrib( GL_ENABLE_BIT);
		glPopAttrib();

		// Render the text iteself.
		tpos_i = text_pos.get();		
		text_iterator text_i = text.begin();
		text_iterator text_end = text.end();
		while (tpos_i != tpos_end && text_i != text_end) {
			glRasterPos3dv( &tpos_i->x);
			// float raster_pos[3];
			// GLboolean valid;
			// glGetBooleanv( GL_CURRENT_RASTER_POSITION_VALID, &valid);
			// glGetFloatv( GL_CURRENT_RASTER_POSITION, raster_pos);
			font.gl_render( *text_i);
			++tpos_i;
			++text_i;
		}
	} glMatrixMode( GL_MODELVIEW); } // Pops the matricies back off the stack
	list.gl_compile_end();
	check_gl_error();
	scene.screen_objects.insert( std::make_pair(pos, list));
}

vector
label::get_center() const
{
	return pos;
}

} // !namespace cvisual
