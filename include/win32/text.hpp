#ifndef VPYTHON_WIN32_TEXT_HPP
#define VPYTHON_WIN32_TEXT_HPP

#include "win32/render_surface.hpp"
#include <boost/shared_ptr.hpp>

namespace cvisual {

// label uses string_t to avoid using Glib::ustring on Windows
typedef std::string string_t;

class layout;

class font : public sigc::trackable
{
 private:
	font( const std::string&, int);
	HFONT font_handle;
	int listbase;

 public:
	~font();
	
	boost::shared_ptr<layout> 
	lay_out( const std::string& text );
	
	static boost::shared_ptr<font>
	find_font( const std::string& desc = std::string(), int height = -1);
};

class layout
{
 private:
 	friend class font;

 	float width;
 	float height;
 	int listbase;
 	std::string text;
	layout(float w, float h, int l_base, const std::string& t )
		: width(w), height(h), listbase( l_base), text(t)
	{}

 public:
	void gl_render( const vector& pos);
	// The extent of an empty string is zero width, and ascent+descent height
	inline vector extent(void) { return vector( width, height, 0); }
};

} // !namespace cvisual

#endif /*VPYTHON_WIN32_TEXT_HPP*/
