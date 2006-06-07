#include "win32/text.hpp"
#include "win32/render_surface.hpp"
#include "util/errors.hpp"
#include "util/gl_free.hpp"

#include <boost/lexical_cast.hpp>

namespace cvisual {

void
font::gl_free(void)
{
	if (listbase > 0) {
		VPYTHON_NOTE( "Releasing 256 displaylists, starting with number " 
			+ boost::lexical_cast<std::string>(listbase));
		glDeleteLists( listbase, 256);
		listbase = 0;
	}
}

font::font( const std::string& desc, int size)
	: font_handle(0), listbase(0)
{
	HDC dev_context = render_surface::current->dev_context;
	if (desc == std::string("") && size < 0) {
		VPYTHON_NOTE( "Allocating default system font");
		font_handle = (HFONT)GetStockObject( SYSTEM_FONT);
	}
	else {
		VPYTHON_NOTE( "Allocating custom font: " + desc 
			+ boost::lexical_cast<std::string>( size));
		font_handle = CreateFont( 
			size > 0 ? -size : 0, 
			0, 0, 0, 0, 0, 0, 0, // width, angle, underline, bold, etc
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
	}
	listbase = glGenLists(256);
	if (!listbase) {
		VPYTHON_WARNING( "Failed to allocate displaylists for text rendering");
	}
	else {
		VPYTHON_NOTE( "Allocated 256 displaylists starting with number "
			+ boost::lexical_cast<std::string>( listbase));
	}
	on_gl_free.connect( sigc::mem_fun( *this, &font::gl_free));
	SelectObject( dev_context, font_handle);
	wglUseFontBitmaps( dev_context, 0, 256, listbase);
}

font::~font()
{
	// TODO: THis is not the correct action.  The correct action is to set a 
	// pending delete call.
	// gl_free();
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
		boost::shared_ptr<font> ret( new font( desc, height));
		font_cache[std::make_pair(desc, height)] = ret;
		VPYTHON_NOTE( "Created new font and added to cache: " + desc + ";" + 
			boost::lexical_cast<std::string>(height));
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
