
#include "gtk2/font.hpp"

#include <FTGLPixmapFont.h>
#include <fontconfig/fontconfig.h>
#include <pangomm/fontdescription.h>
#include <gtkmm/style.h>
#include <gtkmm/settings.h>
#include <gdkmm/screen.h>

bitmap_font::_init* bitmap_font::init = 0;

std::map<std::string, FTGLPixmapFont*> bitmap_font::font_cache;

bitmap_font::_init::_init()
{
	FcInit();
}

// TODO: Figure out an appropriate error handling policy and apply it.
bitmap_font::bitmap_font()
	: font(0), name("default")
{
	using namespace Pango;
	
	font_cache_iterator i = font_cache.find( std::string("default"));
	if (i != font_cache.end()) {
		font = i->second;
	}
	else {
		if (!init)
			init = new _init();
		
		// Load up info about the font description.
		FontDescription font_desc = Glib::wrap(gtk_style_new())->get_font();
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
					FcPatternAddInteger( pattern, FC_WEIGHT, FC_WEIGHT_ULTRALIGHT);
					break;
				case WEIGHT_LIGHT:
					FcPatternAddInteger( pattern, FC_WEIGHT, FC_WEIGHT_LIGHT);
					break;
				case WEIGHT_NORMAL:
					FcPatternAddInteger( pattern, FC_WEIGHT, FC_WEIGHT_NORMAL);
					break;
				case WEIGHT_BOLD:
					FcPatternAddInteger( pattern, FC_WEIGHT, FC_WEIGHT_BOLD);
					break;
				case WEIGHT_ULTRABOLD:
					FcPatternAddInteger( pattern, FC_WEIGHT, FC_WEIGHT_ULTRABOLD);
					break;
				case WEIGHT_HEAVY:
					FcPatternAddInteger( pattern, FC_WEIGHT, FC_WEIGHT_HEAVY);
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
		g_object_get( 
			Gtk::Settings::get_for_screen( Gdk::Screen::get_default())->gobj(),
			"gtk-xft-dpi", &dpi);
		font->FaceSize( font_desc.get_size() / SCALE, 
			(dpi > 0) ? (dpi / 1024) : 100);
			
		// Load the font into the font cache.
		font_cache[name] = font;
	}
}

bitmap_font::~bitmap_font()
{
	
}

// Renders the given string.
void 
bitmap_font::gl_render( const std::string&)
{
}
	
// The rise of the font above the position given with glRasterPos().
double 
bitmap_font::ascent()
{
	return 0.0;
}

	
// The drop of the font below the position given with glRasterPos().
double 
bitmap_font::descent()
{
	return 0.0;
}
 
// Estimates the width of a string of text, in pixels.
double 
bitmap_font::width( const std::string&)
{
	return 0.0;
}
