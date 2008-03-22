#include "font_renderer.hpp"
#include "util/errors.hpp"

#include <gtkmm/style.h>
#include <gtkmm/settings.h>
#include <pangomm.h>
#include <pango/pangoft2.h>
#include <gtk/gtkstyle.h>

namespace cvisual {

static PangoFT2FontMap* fontmap = 0;

using std::wstring;

Glib::ustring w2u( const wstring& w ) {
	// Glib uses UTF8 strings, whereas elsewhere in the program we use wstring, which
	//   is either UTF-16 or UCS-4.  Frankly, I prefer the UTF8 approach but we don't
	//   always have Glib around, and in any case Python uses wide encoding.

	gchar* utf8;
	if ( sizeof(wchar_t) == 2 )
		utf8 = g_utf16_to_utf8( reinterpret_cast<gunichar2*>(w.c_str()), -1, 0, 0, 0 );
	else if ( sizeof(wchar_t) == 4 )
		utf8 = g_ucs4_to_utf8( reinterpret_cast<guinchar*>(w.c_str()), -1, 0, 0, 0 );
	else
		throw std::exception("Unexpected wchar_t.");
	
	Glib::ustring us( utf8 );
	gfree( utf8 );
	
	return us;
}

font_renderer::font_renderer( const wstring& description, int height ) {
	if (!fontmap) {
		#ifdef G_OS_WIN32
		int dpi = 90;
		#else
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
		#endif
		fontmap = (PangoFT2FontMap*)pango_ft2_font_map_new();
		pango_ft2_font_map_set_resolution( fontmap, dpi, dpi);
	}

	ft2_context = Glib::wrap( pango_ft2_font_map_create_context( fontmap ) );

	Pango::FontDescription font_desc = Glib::wrap(gtk_style_new())->get_font();
	if (height > 0)
		font_desc.set_size( height * Pango::SCALE);
	if (desc.size())
		font_desc.set_family( w2u(description) );
	font_desc.set_style( Pango::STYLE_NORMAL);
	
	ft2_context->set_font_description(font_desc);
}

void font_renderer::gl_render_to_texture( const view&, const wstring& text, layout_texture& tx ) {
	// Lay out text
	Glib::RefPtr<Pango::Layout> pango_layout = Pango::Layout::create( ft2_context);

	pango_layout->set_alignment( Pango::ALIGN_LEFT);
	pango_layout->set_width( -1);
	pango_layout->set_text( w2u(text) );

	Pango::Rectangle extents = pango_layout->get_logical_extents();
	if (!extents.get_width()) extents.set_width(1);
	if (!extents.get_height()) extents.set_height(1);

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

	// And copy it into the texture
	tx.set_image( extents.get_width(), extents.get_height(), GL_ALPHA, GL_ALPHA, GL_UNSIGNED_BYTE, 1, bitmap.buffer );
}