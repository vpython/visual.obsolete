#include <gtkmm.h>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <fontconfig/fontconfig.h>
using boost::shared_ptr;

// A demo that was used to figure out how I could get a font filename (for FTGL)
// from a Pango::FontDescription.
int
main( int argc, char** argv)
{
	Gtk::Main kit( &argc, &argv);
	
	// Load up info about the font description.
	Pango::FontDescription font_desc = Glib::wrap(gtk_style_new())->get_font();
	Pango::FontMask set_properties = font_desc.get_set_fields();
	std::cout << "Font name: " << font_desc.to_string() << std::endl;
	
	FcInit();
	
	FcConfig* conf = FcConfigGetCurrent();
	// Initialize an FcPattern with info from the default style.
	shared_ptr<FcPattern> pattern( FcPatternCreate(), FcPatternDestroy);
	// Set some of the essential properties for the font.
	if (Pango::FONT_MASK_FAMILY & set_properties)
		FcPatternAddString( pattern.get(), FC_FAMILY, 
			(const FcChar8*)font_desc.get_family().c_str());
	if (Pango::FONT_MASK_STYLE & set_properties) {
		switch (font_desc.get_style()) {
			case Pango::STYLE_NORMAL:
				FcPatternAddInteger( pattern.get(), FC_SLANT, FC_SLANT_ROMAN);
				break;
			case Pango::STYLE_OBLIQUE:
				FcPatternAddInteger( pattern.get(), FC_SLANT, FC_SLANT_OBLIQUE);
				break;
			case Pango::STYLE_ITALIC:
				FcPatternAddInteger( pattern.get(), FC_SLANT, FC_SLANT_ITALIC);
				break;
		}
	}
	if (Pango::FONT_MASK_WEIGHT & set_properties) {
		switch (font_desc.get_weight()) {
			case Pango::WEIGHT_ULTRALIGHT:
				FcPatternAddInteger( pattern.get(), FC_WEIGHT, FC_WEIGHT_ULTRALIGHT);
				break;
			case Pango::WEIGHT_LIGHT:
				FcPatternAddInteger( pattern.get(), FC_WEIGHT, FC_WEIGHT_LIGHT);
				break;
			case Pango::WEIGHT_NORMAL:
				FcPatternAddInteger( pattern.get(), FC_WEIGHT, FC_WEIGHT_NORMAL);
				break;
			case Pango::WEIGHT_BOLD:
				FcPatternAddInteger( pattern.get(), FC_WEIGHT, FC_WEIGHT_BOLD);
				break;
			case Pango::WEIGHT_ULTRABOLD:
				FcPatternAddInteger( pattern.get(), FC_WEIGHT, FC_WEIGHT_ULTRABOLD);
				break;
			case Pango::WEIGHT_HEAVY:
				FcPatternAddInteger( pattern.get(), FC_WEIGHT, FC_WEIGHT_HEAVY);
				break;
		}
	}
	
	// Set default properties for everything not already covered.
	FcDefaultSubstitute( pattern.get());
	// Perform config-based pattern substitutions.
	FcConfigSubstitute( conf, pattern.get(), FcMatchPattern);
	// Get the font most closely matching the one I want.
	FcResult result = FcResultMatch;
	FcPattern* match = FcFontMatch( conf, pattern.get(), &result);
	// Find its filename.
	char* filename = 0;
	FcPatternGetString( match, FC_FILE, 0, (FcChar8**)&filename);
	std::cout << "Font file: " << filename << std::endl;

	return 0;	
}
