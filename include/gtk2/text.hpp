#ifndef VPYTHON_GTK2_TEXT_HPP
#define VPYTHON_GTK2_TEXT_HPP

#include <boost/shared_ptr.hpp>
#include <string>

#include <glibmm/refptr.h>

#include "util/texture.hpp"
#include "util/vector.hpp"

namespace Pango {
	class Context;
}

namespace cvisual {

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
	font( const std::string&, int);

 public:
	~font();
	
	boost::shared_ptr<layout> 
	lay_out( const std::string& text );
	
	static boost::shared_ptr<font> 
	find_font( const std::string& desc = std::string(), int height = -1);
};


class ft2_texture;

class layout
{
 private:
 	friend class font;

 	float width;
 	float height;
 	boost::shared_ptr<ft2_texture> tex;
 	
	layout( float w, float h, boost::shared_ptr<ft2_texture> t);

 public:
	void gl_render( const vector& pos);
	inline vector extent(void) { return vector( width, height, 0); }
};

} // !namespace cvisual

#endif // !defined VPYTHON_UTIL_TEXT_HPP
