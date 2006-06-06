#include "win32/text.hpp"

namespace cvisual {

font::font( const std::string& desc, int size)
	: font_handle(0), listbase(-1)
{
	HDC dev_context = render_surface::current->dev_context;
	font_handle = CreateFont( -size, 0,
		0, 0,
		0,
		0, 0, 0,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		PROOF_QUALITY,
		VARIABLE_PITCH | FF_SWISS,
		desc != std::string() ? desc.c_str() : 0
	);
	if (!font_handle) {
		VPYTHON_WARNING( "Could not allocate requested font, "
			"falling back to system default");
		font_handle = (HFONT)GetStockObject( SYSTEM_FONT);
	}
	listbase = glGenLists(256);
	SelectObject( dev_context, font);
	wglUseFontBitmaps( dev_context, 0, 256, listbase);
}

font::~font()
{
	if (listbase >= 0) {
		glDeleteLists( listbase);
		listbase = -1;
	}
}

boost::shared_ptr<layout> 
font::lay_out( const std::string& text )
{
	SIZE size;
	GetTextExtentPoint32(
		render_surface::current->dev_context, text.c_str(), text.size(), &size);
	return boost::shared_ptr<layout>( new layout( size.cx, size.cy, listbase, text));
}

namespace {
typedef std::map< std::pair<std::string, int>, boost::shared_ptr<font> >
	fontcache_t;
fontcache_t font_cache;
}

boost::shared_ptr<font>
font::find_font( const std::string& desc, int height)
{
	fontcache_t::iterator i = font_cache.find( std::make_pair( desc, height));
	if (i != font_cache.end()) {
		return i->second;;
	}
	else {
		VPYTHON_NOTE( "Created new font and added to cache: " + desc + ";" + 
			boost::lexical_cast<std::string>(height));
		boost::shared_ptr<font> ret( new font( desc, height));
		font_cache[std::make_pair(desc, height)] = ret;
		return ret;
	}
}

void
layout::gl_render( const vector& pos)
{
	glRasterPos3dv( &pos.x);
	glListBase( listbase);
	glCallLists( text.size(), GL_UNSIGNED_BYTE, text.c_str());
};
	
} // !namespace cvisual
