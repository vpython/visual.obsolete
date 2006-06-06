#ifndef VPYTHON_GTK2_TEXT_HPP
#define VPYTHON_GTK2_TEXT_HPP

#include <boost/shared_ptr.hpp>

#include <glibmm/refptr.h>
#include <glibmm/ustring.h>

#include "util/texture.hpp"
#include "util/vector.hpp"

namespace Pango {
	class Context;
}

namespace cvisual {

typedef Glib::ustring string_t;

/* There will be a platform-specifc font class, as well as layout and
 * provider classes.  The layout and provider implementations will be invisible
 * from the headers.  However, the provider will provide at least
 * boost::shared_ptr<font> create_font( const std::string& desc, int size);
 * in its implementation header.
 */
class layout;

class font
{
 private:
	Glib::RefPtr<Pango::Context> ft2_context;
	font( const Glib::ustring&, int);

 public:
	~font();
	
	boost::shared_ptr<layout> 
	lay_out( const Glib::ustring& text );
	
	static boost::shared_ptr<font> 
	find_font( const Glib::ustring& desc = Glib::ustring(), int height = -1);
};

class ft2_texture;

class layout
{
 private:
 	friend class font;

 	float width;
 	float height;
 	boost::shared_ptr<ft2_texture> tex;
 	vector coord[4];
 	vector tcoord[4];
 	
	layout( float w, float h, boost::shared_ptr<ft2_texture> t);

 public:
	void gl_render( const vector& pos);
	// The extent of an empty string is zero width, and ascent+descent height
	inline vector extent(void) { return vector( width, height, 0); }
};

} // !namespace cvisual

#endif // !defined VPYTHON_UTIL_TEXT_HPP
