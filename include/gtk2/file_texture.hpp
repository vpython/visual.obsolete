#ifndef VPYTHON_GTK2_FILE_TEXTURE_HPP
#define VPYTHON_GTK2_FILE_TEXTURE_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "util/texture.hpp"
#include <gdkmm/pixbuf.h>
#include <string>

namespace cvisual {

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

} // !namespace cvisual

#endif // VPYTHON_GTK2_FILE_TEXTURE_HPP
