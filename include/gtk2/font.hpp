#ifndef VPYTHON_GTK2_FONT_HPP
#define VPYTHON_GTK2_FONT_HPP

#include <string>

class bitmap_font
{
 protected:
	int displaylist_base;
	std::string name;
	// A function called to initialize the buggar on first use.
	void init();
	int ascent;
	int descent;
 
 public:
	// Get the default application font.
	bitmap_font();
	// Allocate a particular font using the abstract family identifier.
	bitmap_font( const std::string& name);
	// Frees OpenGL and Gtk resources.
	~bitmap_font();

	// Renders the given string.
	void gl_render( const char* text);
	void gl_render( const std::string&);
 
	// Estimates the dimentions of a particular string as a (width, height) 
	// pair.
	std::pair<double, double> get_bounds(const char* text);
};


#endif // !defined VPYTHON_GTK2_FONT_HPP
