#ifndef VPYTHON_WIN32_TEXT_HPP
#define VPYTHON_WIN32_TEXT_HPP

#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>
#include <sigc++/object.h>
#include "util/vector.hpp"
#include "wrap_gl.hpp"

namespace cvisual {

// label uses string_t to avoid using Glib::ustring on Windows
typedef std::string string_t;

class layout;

class font : public sigc::trackable
{
 private:
	float ascent;
	float height;
	font( const std::string&, int);
	HFONT font_handle;
	unsigned int listbase;
	void gl_free(void);

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
 	float line_height;
 	float ascent;
 	unsigned int listbase;
 	std::vector<std::string> text;
 	typedef std::vector<std::string>::iterator text_iter;
	layout(float w, float h, float l_h, float a, unsigned int l_base, const std::vector<std::string>& t )
		: width(w), height(h), line_height(l_h), ascent(a), listbase( l_base), text(t)
	{}

 public:
	void gl_render( const vector& pos);
	// The extent of an empty string is zero width, and ascent+descent height
	inline vector extent(void) { return vector( width, height, 0); }
};

} // !namespace cvisual

#endif /*VPYTHON_WIN32_TEXT_HPP*/
