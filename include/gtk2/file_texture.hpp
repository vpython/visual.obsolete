#ifndef VPYTHON_GTK2_FILE_TEXTURE_HPP
#define VPYTHON_GTK2_FILE_TEXTURE_HPP

#include "util/texture.hpp"
#include <gdkmm/pixbuf.h>
#include <string>

// Loads a texture from a file on disk.
class file_texture : public texture
{
 public:
	const std::string filename;
	// TODO: perhaps this should use Glib::ustring instead?
	static shared_ptr<texture> create( const std::string& filename);
	virtual ~file_texture();
	
 private:
	file_texture( const std::string&);
	virtual void gl_init();
	virtual void gl_transform();
	Glib::RefPtr<Gdk::Pixbuf> image;
};

#endif // VPYTHON_GTK2_FILE_TEXTURE_HPP
