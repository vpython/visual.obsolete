#ifndef VPYTHON_WIN32_FONT_HPP
#define VPYTHON_WIN32_FONT_HPP

#include "wrap_gl.h"

#include <map>
#include <string>
#include <utility>

class bitmap_font
{
 private:		
	// A mapping from a font description+size pair to a Windows font.
	static std::map<std::pair<std::string, int>, std::pair<HFONT, int> > cache;
	typedef std::map<std::pair<std::string, int>, std::pair<HFONT, int> > cache_iterator;
	
	std::string font_family;
	int font_size;
	HFONT font;
	double m_ascent;
	double m_descent;
	int listbase;

 public:
	// TODO: Perhaps these constructors should take a view reference?
	bitmap_font( int size=10);
	bitmap_font( const std::string& name, int size=10);
	virtual ~bitmap_font();

	double ascent() const;
	double descent() const;
	virtual void gl_render( const std::string& text) const;
	virtual double width( const std::string& text) const;
};

#endif // !defined VPYTHON_WIN32_FONT_HPP
