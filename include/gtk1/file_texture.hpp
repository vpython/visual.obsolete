#ifndef VPYTHON_GTK1_FILE_TEXTURE_HPP
#define VPTYHON_GTK1_FILE_TEXTURE_HPP

#include "util/texture.hpp"
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <string>

#ifndef GDK_PIXBUF_H
 struct GdkPixbuf;
#endif

class file_texture : public texture
{
 public:
	std::string filename;
	GdkPixbuf* image;
	
 public:
 	// TODO: perhaps this should use Glib::ustring instead?
	static shared_ptr<texture> create( const std::string& filename);
	virtual ~file_texture();
 
 private:
	file_texture( const std::string& filename);	
	virtual void gl_init();
	virtual void gl_transform();
};

#endif
