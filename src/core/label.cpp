#include "label.hpp"
#include "gtk2/font.hpp"
#include "util/errors.hpp"
#include <sstream>

label::label()
	: space(0),
	xoffset(0),
	yoffset(0),
	border(5),
	font_description("default"),
	box_enabled(true),
	line_enabled(true),
	linecolor( color),
	opacity(0.66)
{
}

void
label::set_pos( const vector& n_pos)
{
	pos = n_pos;
}

void
label::set_text( std::string t)
{
	text.clear();
	std::istringstream buf( t);
	std::string line;
	while (std::getline( buf, line)) {;
		text.push_back( line);
	}
}

void 
label::set_space( double n_space)
{
	space = n_space;
}

void 
label::set_xoffset( double n_xoffset)
{
	xoffset = n_xoffset;
}

void 
label::set_yoffset( double n_yoffset)
{
	yoffset = n_yoffset;
}

void 
label::set_border( double n_border)
{
	border = n_border;
}

void 
label::set_font_family( std::string name)
{
	font_description = name;
}

void 
label::render_box( bool enable)
{
	box_enabled = enable;
}

void 
label::render_line( bool enable)
{
	line_enabled = enable;
}

void
label::gl_render( const view& scene)
{
	// Compute the width of the text box.
	double box_width = 0.0;
	color.gl_set();
	bitmap_font font( font_description);
	for (std::vector<std::string>::iterator i = text.begin(); i != text.end(); ++i) {
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
	
	vector text_pos[text.size()];
	vector* tpos_i = text_pos;
	vector* tpos_end = text_pos + text.size();
	vector next_tpos( border, box_height - border - font.ascent());
	while (tpos_i != tpos_end) {
		*tpos_i = next_tpos; // (tpos_i - 1) - vector(0, font.ascent() - font.descent());
		next_tpos.y -= (font.ascent() - font.descent());
		++tpos_i;
	}

	clear_gl_error();
	glDisable( GL_DEPTH_TEST);
	glDisable( GL_LIGHTING);
	{
		gl_matrix_stackguard guard;
		vector label_pos = pos * scene.gcf;
		glTranslated( label_pos.x, label_pos.y, label_pos.z);
		// The label to world orientation transform.
		tmatrix lwt;
		vector x_axis = scene.forward.cross( scene.up).norm();
		lwt.x_column( x_axis);
		lwt.y_column( scene.up);
		lwt.z_column( -scene.forward);
		lwt.w_column();
		lwt.w_row();
		lwt.gl_mult();
		// At this point, all furthur translations are in direction of label
		//  space.
		if (space && (xoffset || yoffset)) {
			// Move the origin away from the body.
			vector space_offset = vector(xoffset, yoffset).norm() * std::fabs(space);
			glTranslated( space_offset.x, space_offset.y, 0);
		}
		// Scale the dimentions such that all further operations are in units
		// of pixels.
		double eye_dist = ((pos - scene.camera)*scene.gcf).dot( scene.forward);
		glScaled( 
			scene.tan_hfov_x * eye_dist * 2.0 / scene.window_width,
			scene.tan_hfov_y * eye_dist * 2.0 / scene.window_height,
			1);

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
			glTranslated(-box_width*0.5, -box_height*0.5, 0);
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
		// appearance is as if, for the first line, glRasterPos was far away in
		// the depth direction.  I have only observed this issue with the ATI
		// proprietary fglrx driver at this time - MESA's software renderer is
		// fine.
		glRasterPos3d( 0, 0, 0);
		glPushAttrib( GL_ENABLE_BIT);
		glPopAttrib();

		// Render the text iteself.
		tpos_i = text_pos;		
		std::vector<std::string>::iterator text_i = text.begin();
		std::vector<std::string>::iterator text_end = text.end();
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
	}
	glEnable( GL_LIGHTING);
	glEnable( GL_DEPTH_TEST);
	check_gl_error();
}

vector
label::get_center() const
{
	return pos;
}
