#ifndef VPYTHON_WIN32_FONT_HPP
#define VPYTHON_WIN32_FONT_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "wrap_gl.hpp"

#include <map>
#include <string>
#include <utility>

namespace cvisual {

class bitmap_font
{
 private:		
	// A mapping from a font description+size pair to a Windows font.
	static std::map<std::pair<std::string, int>, std::pair<HFONT, int> > cache;
	typedef std::map<std::pair<std::string, int>, 
		std::pair<HFONT, int> >::iterator cache_iterator;
	
	std::string font_family;
	int font_size;
	HFONT font;
	double m_ascent;
	double m_descent;
	int listbase;

 public:
	// TODO: Perhaps these constructors should take a view reference?
	bitmap_font();
	bitmap_font( const std::string& name, int size=10);
	virtual ~bitmap_font();

	double ascent() const;
	double descent() const;
	virtual void gl_render( const std::string& text) const;
	virtual double width( const std::string& text) const;
};

} // !namespace cvisual;

#endif // !defined VPYTHON_WIN32_FONT_HPP
