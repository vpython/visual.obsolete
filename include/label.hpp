#ifndef VPYTHON_LABEL_HPP
#define VPYTHON_LABEL_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "renderable.hpp"
#include "text.hpp"
#include <vector>

namespace cvisual {

class label : public renderable
{
 public:
	label();
	label( const label& other);
	virtual ~label();

	void set_pos( const vector& n_pos);
	shared_vector& get_pos();

	void set_x( double x);
	double get_x();

	void set_y( double y);
	double get_y();

	void set_z( double z);
	double get_z();

	void set_color( const rgba& n_color);
	rgba get_color();

	void set_red( float x);
	double get_red();

	void set_green( float x);
	double get_green();

	void set_blue( float x);
	double get_blue();

	void set_opacity( float);
	double get_opacity();
	
	void set_text( string_t t);
	string_t get_text();

	void set_space( double space);
	double get_space();

	void set_xoffset( double xoffset);
	double get_xoffset();

	void set_yoffset( double yoffset);
	double get_yoffset();

	void set_border( double border);
	double get_border();

	void set_font_family( string_t name);
	string_t get_font_family();

	void set_font_size(double);
	double get_font_size();

	void render_box( bool);
	bool has_box();

	void render_line( bool);
	bool has_line();

	void set_linecolor( const rgba& color);
	rgba get_linecolor();

 protected:
	// In world space:
	shared_vector pos;
	double space;

	// In pixels:
	double xoffset;   // offset from pos + space to the box
	double yoffset;
	double border;    // space between text and box

	/// A common name for the font.
	string_t font_description;
	/// The nominal size of the font, in pixels.
	double font_size;

	bool box_enabled; ///< True to draw a box around the text
	bool line_enabled; ///< True to draw a line to the text.

	// bitmap_font* font;
	rgba linecolor; ///< The color of the lines in the label. (color is for text)
	float opacity; ///< The opacity of the background for the text.

	// Text strings in python may be specified by the """ ... """ syntax in python.
	// This case is handled by the layout code
	string_t text;

	bool text_changed;
	boost::shared_ptr<layout> text_layout;

	virtual void gl_render( const view&);
	virtual vector get_center() const;
};

} // !namespace cvisual

#endif // !defined VPYTHON_LABEL_HPP
