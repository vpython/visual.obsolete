// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "win32/font.hpp"
#include "win32/render_surface.hpp"
#include "util/errors.hpp"

#include <GL/glext.h>

namespace cvisual {

std::map<std::pair<std::string, int>, std::pair<HFONT, int> > 
bitmap_font::cache;

bitmap_font::bitmap_font()
	: font_family("default"), font_size(0), font(0), 
	m_ascent(0), m_descent(0), listbase(0)
{
	assert( render_surface::current != 0);
	
	HDC dev_context = render_surface::current->dev_context;
	cache_iterator i = cache.find(std::make_pair(font_family, font_size));
	if (i != cache.end()) {
		font = i->second.first;
		listbase = i->second.second;
		SelectObject( dev_context, font);
	}
	else {
		font = (HFONT)GetStockObject( SYSTEM_FONT);
		listbase = glGenLists(256);
		SelectObject( dev_context, font);
		BOOL result = wglUseFontOutlines(
			dev_context,
			0,
			256,
			listbase,
			0.0f,
			0.0f,
			WGL_FONT_POLYGONS,
			0
		);
		
		if (!result) {
			WIN32_CRITICAL_ERROR("wglUseFontOutlines()");
		}
		cache[std::make_pair(font_family, font_size)] 
			= std::make_pair(font, listbase);
	}
	
	TEXTMETRIC metrics;
	GetTextMetrics( dev_context, &metrics);
	m_ascent = metrics.tmAscent * 2.0 / render_surface::current->window_width;
	m_descent = metrics.tmDescent * 2.0 / render_surface::current->window_width;
}

bitmap_font::bitmap_font( const std::string& name, int size)
	: font_family(name), font_size(size), font(0), 
	m_ascent(0), m_descent(0), listbase(0)
{
	assert( render_surface::current != 0);
	
	HDC dev_context = render_surface::current->dev_context;
	cache_iterator i = cache.find(std::make_pair(font_family, font_size));
	if (i != cache.end()) {
		font = i->second.first;
		listbase = i->second.second;
		SelectObject( dev_context, font);
	}
	else {
		font = (HFONT)CreateFont(
			static_cast<int>(-72.0 * size 
				/ GetDeviceCaps(dev_context, LOGPIXELSY)),
			0,
			0,0,
			0,
			0,0,0,
			DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			PROOF_QUALITY, 
			VARIABLE_PITCH | FF_SWISS, 
			font_family.c_str()
		);
		if (!font)
			WIN32_CRITICAL_ERROR("CreateFont()");
		SelectObject( dev_context, font);
		listbase = glGenLists(256);
		BOOL result = wglUseFontOutlines(
			dev_context,
			0,
			256,
			listbase,
			0.0f,
			0.0f,
			WGL_FONT_POLYGONS,
			0
		);
		
		if (!result) {
			WIN32_CRITICAL_ERROR("wglUseFontOutlines()");
		}
		cache[std::make_pair(font_family, font_size)] 
			= std::make_pair(font, listbase);
	}
	
	TEXTMETRIC metrics;
	GetTextMetrics( dev_context, &metrics);
	m_ascent = metrics.tmAscent * 2.0 / render_surface::current->window_height;
	m_descent = metrics.tmDescent * 2.0 
		/ render_surface::current->window_height;
}

bitmap_font::~bitmap_font()
{
}

double
bitmap_font::ascent() const
{
	return m_ascent;
}

double
bitmap_font::descent() const
{
	return m_descent;
}

void 
bitmap_font::gl_render( const std::string& text) const
{
	assert( listbase != 0);
	glListBase( listbase);
	glCallLists( text.size(), GL_UNSIGNED_BYTE, text.c_str());
}

double 
bitmap_font::width( const std::string& text) const
{
	assert( render_surface::current != 0);
	
	SIZE size;
	GetTextExtentPoint32(
		render_surface::current->dev_context, text.c_str(), text.size(), &size);
	return static_cast<double>(size.cx * 2) 
		/ render_surface::current->window_width;
}

} // !namespace cvisual
