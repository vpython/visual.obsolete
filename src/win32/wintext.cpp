#include "text.hpp"
#include "render_surface.hpp"
#include "util/errors.hpp"
#include "util/gl_free.hpp"
#include "wrap_gl.hpp"

#include <boost/lexical_cast.hpp>
#include <sstream>

namespace cvisual {

void
font::gl_free(void)
{
	if (listbase > 0) {
		VPYTHON_NOTE( "Releasing 255 displaylists, starting with number " 
			+ boost::lexical_cast<std::string>(listbase));
		glDeleteLists( listbase, 255);
		listbase = 0;
	}
}

font::font( const std::string& desc, int size)
	: ascent(0), font_handle(0), listbase(0)
{
	size = 12;
	HDC dev_context = render_surface::current->dev_context;
	if (desc == std::string() && size < 0) {
		VPYTHON_NOTE( "Allocating default system font");
		font_handle = (HFONT)GetStockObject( SYSTEM_FONT);
	}
	else {
		VPYTHON_NOTE( "Allocating custom font: " + desc + " "
			+ boost::lexical_cast<std::string>( size));
		font_handle = CreateFont( 
			size > 0 ? -size : 0, 
			0, 0, 0, 0, 0, 0, 0, // width, angle, underline, bold, etc
			DEFAULT_CHARSET,
			OUT_TT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE,
			desc != std::string() ? desc.c_str() : 0
		);
		if (!font_handle) {
			VPYTHON_WARNING( "Could not allocate requested font, "
				"falling back to system default");
			font_handle = (HFONT)GetStockObject( SYSTEM_FONT);
		}
	}
	listbase = glGenLists(255);
	if (!listbase) {
		VPYTHON_WARNING( "Failed to allocate displaylists for text rendering");
	}
	else {
		VPYTHON_NOTE( "Allocated 255 displaylists, starting with number "
			+ boost::lexical_cast<std::string>( listbase));
	}
	on_gl_free.connect( sigc::mem_fun( *this, &font::gl_free));
	SelectObject( dev_context, font_handle);
	TEXTMETRIC tm;
	GetTextMetrics( dev_context, &tm);
	ascent = tm.tmAscent;
	height = tm.tmAscent + tm.tmDescent;
	wglUseFontBitmaps( dev_context, 0, 255, listbase);
}

font::~font()
{
	// TODO: THis is not the correct action.  The correct action is to set a 
	// pending delete call.
	// gl_free();
}

boost::shared_ptr<layout> 
font::lay_out( const std::string& new_text )
{
	std::vector<std::string> text;
	
	// Parse the 
	std::istringstream buf( new_text);
	std::string line;
	while (std::getline( buf, line))
		text.push_back( line);
	
	float maxwidth = 0;
	float total_height = 0;
	for (layout::text_iter i = text.begin(); i != text.end(); ++i) {
		SIZE size;
		GetTextExtentPoint32(
			render_surface::current->dev_context, 
			i->c_str(), i->size(), &size);
		total_height += std::min( float(size.cy), height);
		maxwidth = std::max( float(size.cx), maxwidth);
	}
	
	return boost::shared_ptr<layout>( 
		new layout( maxwidth, total_height, height, ascent, listbase, text));
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
	vector raster_pos = pos - vector(0, ascent);
	glListBase( listbase);
	for (text_iter i = text.begin(); i != text.end(); ++i) {
		glRasterPos3dv( &raster_pos.x);
		glCallLists( i->size(), GL_UNSIGNED_BYTE, i->c_str());
		raster_pos.y -= line_height;
	}
};
	
} // !namespace cvisual
