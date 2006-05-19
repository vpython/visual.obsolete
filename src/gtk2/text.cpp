#include "gtk2/text.hpp"
#include "util/gl_free.hpp"
#include "util/errors.hpp"

#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>

#include <gtkmm/style.h>
#include <gtkmm/settings.h>
#include <pangomm.h>
#include <pango/pangoft2.h>
#include <gtk/gtkstyle.h>

#include <map>
#include <utility>
#include <iostream>
#include <GL/gl.h>
#include <inttypes.h>

namespace cvisual {

/******************************************************************************/
// ft2_texture implementation
namespace {
int
next_power_of_two(const int arg) 
{
	int ret = 2;
	// upper bound of 28 chosen to limit memory growth to about 256MB, which is
	// _much_ larger than most supported textures
	while (ret < arg && ret < (1 << 28))
		ret <<= 1;
	return ret;
}
}

class ft2_texture : public sigc::trackable, public boost::noncopyable
{
 private:
	unsigned int handle;
	void gl_free();

 public:
 	float width;
 	float height;
	~ft2_texture();
	ft2_texture( FT_Bitmap& bitmap);
	void gl_activate();
};

ft2_texture::ft2_texture( FT_Bitmap& bitmap)
	: handle( 0)
{	
	glEnable( GL_TEXTURE_2D);
	glGenTextures(1, &handle);
	VPYTHON_NOTE( "Allocated texture number " 
		+ boost::lexical_cast<std::string>(handle));
	on_gl_free.connect( sigc::mem_fun( *this, &ft2_texture::gl_free));
	
	glBindTexture( GL_TEXTURE_2D, handle);
	int texw = next_power_of_two( bitmap.width);
	int texh = next_power_of_two( bitmap.rows);
	boost::scoped_array<uint8_t> texdata( new uint8_t[texw*texh]);
	memset( texdata.get(), 0, texw*texh);
	
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_ALPHA, texw, texh, 0, GL_ALPHA, 
		GL_UNSIGNED_BYTE, texdata.get());

	int saved_alignment = -1;
	glGetIntegerv( GL_UNPACK_ALIGNMENT, &saved_alignment);
	
	int alignment = bitmap.width % 4;
	if (!alignment)
		alignment = 4;
	if (alignment == 3)
		alignment = 1;
	glPixelStorei( GL_UNPACK_ALIGNMENT, alignment);	

	glTexSubImage2D( GL_TEXTURE_2D, 0, 
		0, 0, bitmap.width, bitmap.rows, 
		GL_ALPHA, GL_UNSIGNED_BYTE, bitmap.buffer);
	glPixelStorei( GL_UNPACK_ALIGNMENT, saved_alignment);
	
	width = bitmap.width / float(texw);
	height = bitmap.rows /float(texh);
	glDisable( GL_TEXTURE_2D);
}

void
ft2_texture::gl_free()
{
	if (handle) {
		VPYTHON_NOTE( "Deleting texture number " 
			+ boost::lexical_cast<std::string>(handle));
		glEnable( GL_TEXTURE_2D);
		glDeleteTextures(1, &handle);
		glDisable( GL_TEXTURE_2D);
		handle = 0;
	}
}

ft2_texture::~ft2_texture()
{
	gl_free();
}

void
ft2_texture::gl_activate()
{
	assert( handle != 0);
	glBindTexture( GL_TEXTURE_2D, handle);
}

/******************************************************************************/
// font and layout implemenation

namespace {
// The shared fontmap that all of the ft2_context's use.
PangoFT2FontMap* fontmap = 0;
// The set of all fonts that have been loaded so far, indexed by their
// description
typedef std::map< std::pair<Glib::ustring, int>, boost::shared_ptr<font> >
	fontcache_t;
fontcache_t font_cache;
}

font::font( const Glib::ustring& desc, int height)
	: ft2_context( Glib::wrap( pango_ft2_font_map_create_context( fontmap)))
{
	Pango::FontDescription font_desc = Glib::wrap(gtk_style_new())->get_font();
	if (height > 0)
		font_desc.set_size( height * Pango::SCALE);
	if (desc != Glib::ustring())
		font_desc.set_family( desc);
	font_desc.set_style( Pango::STYLE_NORMAL);
	
	ft2_context->set_font_description(font_desc);
}

font::~font()
{
}

boost::shared_ptr<font>
font::find_font( const Glib::ustring& desc, int height)
{
	if (!fontmap) {
		int dpi = -1;
		// TODO: Possible memory leak here:
		Glib::RefPtr<Gdk::Screen> default_screen = Gdk::Screen::get_default();
		// TODO: Possible memory leak here:
		Glib::RefPtr<Gtk::Settings> settings = 
			Gtk::Settings::get_for_screen( default_screen);
		g_object_get( settings->gobj(), "gtk-xft-dpi", &dpi, NULL);
		if (dpi > 0)
			dpi /= 1024;
		else
			dpi = 90;
		fontmap = (PangoFT2FontMap*)pango_ft2_font_map_new();
		pango_ft2_font_map_set_resolution( fontmap, dpi, dpi);
	}
	
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

boost::shared_ptr<layout>
font::lay_out( const Glib::ustring& text)
{
	Glib::RefPtr<Pango::Layout> pango_layout = Pango::Layout::create( ft2_context);

	pango_layout->set_alignment( Pango::ALIGN_LEFT);
	pango_layout->set_width( -1);
	pango_layout->set_text( text);
	Pango::Rectangle extents = pango_layout->get_logical_extents();
	
	const int saved_width = PANGO_PIXELS(extents.get_width());
	if (extents.get_width() == 0 || extents.get_height() == 0) {
    	return boost::shared_ptr<layout>();
	}
	
	// Allocate a greyscale buffer
	FT_Bitmap bitmap;
	bitmap.rows = PANGO_PIXELS(extents.get_height());
	bitmap.width = PANGO_PIXELS(extents.get_width());
	assert( bitmap.width == saved_width);
	bitmap.pitch = bitmap.width;
	boost::scoped_array<uint8_t> pix_buf( new uint8_t[bitmap.rows * bitmap.width]);
	bitmap.buffer = pix_buf.get();
	memset (bitmap.buffer, 0, bitmap.rows * bitmap.width);
	bitmap.num_grays = 256;
	bitmap.pixel_mode = ft_pixel_mode_grays;

	// Render the text to the buffer
	pango_ft2_render_layout_subpixel( &bitmap, pango_layout->gobj(), 
		-PANGO_PIXELS(extents.get_x()), -PANGO_PIXELS(extents.get_y()));
	
	boost::shared_ptr<ft2_texture> tex( new ft2_texture( bitmap));
	return boost::shared_ptr<layout>( new layout( bitmap.width, bitmap.rows, tex));
}

layout::layout( float w, float h, boost::shared_ptr<ft2_texture> t)
	: width(w), height(h), tex(t)
{
	coord[0] = vector();
	coord[1] = vector(0, -height);
	coord[2] = vector(width, -height);
	coord[3] = vector(width, 0);
	
	tcoord[0] = vector();
	tcoord[1] = vector(0, tex->height);
	tcoord[2] = vector(tex->width, tex->height);
	tcoord[3] = vector(tex->width, 0);
}


void
layout::gl_render( const vector& pos)
{
	glEnable( GL_TEXTURE_2D);
	glEnable( GL_BLEND);
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	tex->gl_activate();
	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	
	glTranslated( pos.x, pos.y, pos.z);
	glBegin( GL_QUADS);
	for (size_t i = 0; i < 4; ++i) {
		glTexCoord2d( tcoord[i].x, tcoord[i].y);
		coord[i].gl_render();
	}
	glEnd();
	
	glDisable( GL_BLEND);
	glDisable( GL_TEXTURE_2D);
}

} // !namespace cvisual
