#ifndef VPYTHON_LABEL_HPP
#define VPYTHON_LABEL_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "renderable.hpp"
#include <string>
#include <vector>

namespace cvisual {

class bitmap_font;

class label : public renderable
{
 public:
	label();
	void set_pos( const vector& n_pos);
	void set_text( std::string t);
	void set_space( double space);
	void set_xoffset( double xoffset);
	void set_yoffset( double yoffset);
	void set_border( double border);
	void set_font_family( std::string name);
	void render_box( bool);
	void render_line( bool);

 protected:
	// In world space:
	vector pos;
	double space;
	
	// In pixels:
	double xoffset;   // offset from pos + space to the box
	double yoffset;
	double border;    // space between text and box
	
	// A common name for the font.
	std::string font_description;
	
	bool box_enabled; ///< True to draw a box around the text
	bool line_enabled; ///< True to draw a line to the text.
	
	// bitmap_font* font;
	rgba linecolor; ///< The color of the lines in the label. (color is for text)
	double opacity; ///< The opacity of the background for the text.
	
	// Text strings in python may be specified by the """ ... """ syntax in python.
	//   This means that we may recieve text strings that span multiple lines.
	//   Each element of this container contains a single continuous line for 
	//   the displayed text.
	std::vector<std::string> text;
    
	virtual void gl_render( const view&);
	virtual vector get_center() const;
};

} // !namespace cvisual

#endif // !defined VPYTHON_LABEL_HPP
