#ifndef VPYTHON_GTK2_FONT_HPP
#define VPYTHON_GTK2_FONT_HPP

#include <string>
#include <map>

class FTFont;

// A helper class for rendering text.  It has a fast caching capability and is
// designed to be instantiated on-demand within the rendering loop.
class bitmap_font
{
 private:
	// A table mapping names to fonts.  Typically, only the first request for
	// a font will require actually loading it.  Subsequent renders will simply
	// look it up here.
	static std::map<std::string, std::pair<FTFont*, int> > font_cache;
	typedef std::map<std::string, std::pair<FTFont*, int> >::iterator font_cache_iterator;
	
	class _init {
	 public:
		_init();
	};
	static _init* init;
	
 protected:
	FTFont* font;
	std::string name;
	int font_height;
 
 public:
	// Get the default application font.
	bitmap_font();
	// Allocate a particular font using a font family name.
	bitmap_font( const std::string& name, int size = 10);
	// Frees OpenGL and/or Gtk resources.
	virtual ~bitmap_font();

	// Renders the given string at the particular coordinates on the screen.
	virtual void gl_render( const std::string& text) const;
	
	// The rise of the font above the position given with glRasterPos().
	double ascent() const;
	// The drop of the font below the position given with glRasterPos().
	double descent() const;
 
	// Estimates the width of a string of text, in pixels.
	virtual double width( const std::string& text) const;
	
	// Find out the height of the font, in pixels.
	int get_size() const;
};


#endif // !defined VPYTHON_GTK2_FONT_HPP
