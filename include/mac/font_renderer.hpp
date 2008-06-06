#ifndef VPYTHON_WIN32_FONT_RENDERER_HPP
#define VPYTHON_WIN32_FONT_RENDERER_HPP
#pragma once

// See text.hpp for public interface

#include "text.hpp"
#include <Carbon/Carbon.h>
#include <AGL/agl.h>

namespace cvisual {

class aglFont 
{
public:
	double getWidth(const std::wstring& string);
	// returns the horizontal extent of the given string in
	//   viewport coordinates (0.0 to 1.0)

	double ascent();
	// returns the maximum distance from the baseline to the top
	//   of a glyph (e.g. "M")

	double descent();
	// returns the maximum distance from the baseline to the bottom
	//   of a glyph (e.g. "g")

	void draw(const std::wstring& string);
	// draws string with the current glRasterPos at its left baseline

	void release();
	// call once for each call to glContext::getFont()
	  
	//used by aglContext
	aglFont(struct display& cx, 
		const char *name, 
		double size);
	~aglFont();
	void addref() {refcount++;};
  
 private:
	struct display& cx;
	SInt16		fID;
	int			fSize;
	FontInfo	fInfo;
	int			listBase;
	int			refcount;
};

class font_renderer {
 public:
	// Create a font_renderer for the requested font.  
	// Must support 'verdana' or 'sans-serif'
	// Should support 'times new roman' or 'serif', and 'courier new' or 'monospace'
	font_renderer( const std::wstring& description, int height );
	
	// Returns true if the requested font was available.
	bool ok();
	
	// Render text and call tx.set_image()
	void gl_render_to_texture( const struct view&, const std::wstring& text, layout_texture& tx );

	~font_renderer();

 private:
	void* font_handle;
	bool isClearType;
};

} // namespace cvisual

#endif