// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "gtk2/font.hpp"

#include <FTGLPixmapFont.h>
#include <fontconfig/fontconfig.h>

#include <pangomm/fontdescription.h>
#include <gtkmm/style.h>
#include <gtkmm/settings.h>
#include <gdkmm/screen.h>

namespace cvisual {

bitmap_font::_init* bitmap_font::init = 0;
 
std::map<std::string, std::pair<FTFont*, int> > bitmap_font::font_cache;

bitmap_font::_init::_init()
{
	FcInit();
}

// TODO: Figure out an appropriate error handling policy and apply it.
bitmap_font::bitmap_font()
	: font(0), name("default"), font_height(0)
{
	using namespace Pango;
	
	font_cache_iterator i = font_cache.find( std::string("default"));
	if (i != font_cache.end()) {
		font = i->second.first;
		font_height = i->second.second;
	}
	else {
		if (!init)
			init = new _init();
		
		// glEnable( GL_TEXTURE_2D);
		// Load up info about the font description.
		FontDescription font_desc = Glib::wrap(gtk_style_new())->get_font();
		font_height = font_desc.get_size() / SCALE;
		FontMask set_properties = font_desc.get_set_fields();
		FcConfig* conf = FcConfigGetCurrent();
		
		// Initialize an FcPattern with info from the default style.
		FcPattern* pattern = FcPatternCreate();
		if (FONT_MASK_FAMILY & set_properties)
			FcPatternAddString( pattern, FC_FAMILY, 
				(const FcChar8*)font_desc.get_family().c_str());
		if (FONT_MASK_STYLE & set_properties) {
			switch (font_desc.get_style()) {
				case STYLE_NORMAL:
					FcPatternAddInteger( pattern, FC_SLANT, FC_SLANT_ROMAN);
					break;
				case STYLE_OBLIQUE:
					FcPatternAddInteger( pattern, FC_SLANT, FC_SLANT_OBLIQUE);
					break;
				case STYLE_ITALIC:
					FcPatternAddInteger( pattern, FC_SLANT, FC_SLANT_ITALIC);
					break;
			}
		}
		if (FONT_MASK_WEIGHT & set_properties) {
			switch (font_desc.get_weight()) {
				case WEIGHT_ULTRALIGHT:
					FcPatternAddInteger( pattern, FC_WEIGHT, FC_WEIGHT_EXTRALIGHT);
					break;
				case WEIGHT_LIGHT:
					FcPatternAddInteger( pattern, FC_WEIGHT, FC_WEIGHT_LIGHT);
					break;
				case WEIGHT_NORMAL:
					FcPatternAddInteger( pattern, FC_WEIGHT, FC_WEIGHT_MEDIUM);
					break;
				case WEIGHT_BOLD:
					FcPatternAddInteger( pattern, FC_WEIGHT, FC_WEIGHT_BOLD);
					break;
				case WEIGHT_ULTRABOLD:
					FcPatternAddInteger( pattern, FC_WEIGHT, FC_WEIGHT_EXTRABOLD);
					break;
				case WEIGHT_HEAVY:
					FcPatternAddInteger( pattern, FC_WEIGHT, FC_WEIGHT_BLACK);
					break;
			}
		}
		
		// Set default properties for everything not already covered.
		FcDefaultSubstitute( pattern);
		// Perform config-based pattern substitutions.
		FcConfigSubstitute( conf, pattern, FcMatchPattern);
		// Get the font most closely matching the requested font.
		FcResult result = FcResultMatch;
		FcPattern* match = FcFontMatch( conf, pattern, &result);
		FcPatternDestroy( pattern); //< Free the pattern now that it isn't required.
		
		// Find the font file and create the font renderer.
		char* filename = 0;
		FcPatternGetString( match, FC_FILE, 0, (FcChar8**)&filename);
		font = new FTGLPixmapFont( filename);
		
		// Set the font size.
		int dpi = -1;
		Glib::RefPtr<Gdk::Screen> default_screen = Gdk::Screen::get_default();
		Glib::RefPtr<Gtk::Settings> settings = Gtk::Settings::get_for_screen(
			default_screen);
		g_object_get( 
			settings->gobj(),
			"gtk-xft-dpi", &dpi, NULL);
		font->FaceSize( font_height, 
			(dpi > 0) ? (dpi / 1024) : 100);
		font->CharMap( ft_encoding_unicode);
		// glDisable( GL_TEXTURE_2D);
			
		// Load the font into the font cache.
		font_cache[name] = std::make_pair( font, font_height);
	}
}

bitmap_font::bitmap_font( const std::string& family, int size)
	: font(0), name(family), font_height(size)
{
	using namespace Pango;
	
	font_cache_iterator i = font_cache.find( family);
	if (i != font_cache.end()) {
		font = i->second.first;
		size = i->second.second;
	}
	else {
		// glEnable( GL_TEXTURE_2D);
		FcConfig* conf = FcConfigGetCurrent();
		
		// Initialize an FcPattern with info from the default style.
		FcPattern* pattern = FcPatternCreate();
		FcPatternAddString( pattern, FC_FAMILY, (const FcChar8*)family.c_str());
		// Set default properties for everything not already covered.
		FcDefaultSubstitute( pattern);
		// Perform config-based pattern substitutions.
		FcConfigSubstitute( conf, pattern, FcMatchPattern);
		// Get the font most closely matching the requested font.
		FcResult result = FcResultMatch;
		FcPattern* match = FcFontMatch( conf, pattern, &result);
		FcPatternDestroy( pattern); //< Free the pattern now that it isn't required.
		
		// Find the font file and create the font renderer.
		char* filename = 0;
		FcPatternGetString( match, FC_FILE, 0, (FcChar8**)&filename);
		font = new FTGLPixmapFont( filename);
		
		// Set the font size.
		int dpi = -1;
		g_object_get( 
			Gtk::Settings::get_for_screen( Gdk::Screen::get_default())->gobj(),
			"gtk-xft-dpi", &dpi, NULL);
		font->FaceSize( font_height, 
			(dpi > 0) ? (dpi / 1024) : 100);
		font->CharMap( ft_encoding_unicode);
			
		// Load the font into the font cache.
		font_cache[name] = std::make_pair(font, font_height);
		// glDisable( GL_TEXTURE_2D);
	}
}

bitmap_font::~bitmap_font()
{
	
}

// Renders the given string.
void 
bitmap_font::gl_render( const std::string& text) const
{
	// glEnable( GL_TEXTURE_2D);
	font->Render( text.c_str());
	// glDisable( GL_TEXTURE_2D);
}
	
// The rise of the font above the position given with glRasterPos().
double 
bitmap_font::ascent() const
{
	return font->Ascender();
}

	
// The drop of the font below the position given with glRasterPos().
double 
bitmap_font::descent() const
{
	return font->Descender();
}
 
// Estimates the width of a string of text, in pixels.
double 
bitmap_font::width( const std::string& text) const
{
	float bbox[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	font->BBox( 
		text.c_str(), bbox[0], bbox[1], bbox[2], bbox[3], bbox[4], bbox[5]);
	return bbox[3] - bbox[0];
}

int
bitmap_font::get_size() const
{
	return font_height;
}

} // !namespace cvisual
