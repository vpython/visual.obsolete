#include "gtk2/file_texture.hpp"
#include "util/errors.hpp"

#include <iostream>
#include <GL/gl.h>
#include <GL/glu.h>

// This file is a part of libvpython-gtk2.so

shared_ptr<texture>
file_texture::create( const std::string& filename)
{
	return shared_ptr<texture>( new file_texture( filename));
}

file_texture::~file_texture()
{
}

file_texture::file_texture( const std::string& file)
	: filename(file)
{
	try {
		image = Gdk::Pixbuf::create_from_file(filename);
	
#ifdef DEBUG
		// Print out the properties of this image.
		std::cout << "New texture loaded from file " << filename << " with these"
			" properties:";
		std::cout << "\tn_channels: " << texture_img->get_n_channels() << "\n";
		std::cout << "\thas_alpha: " << texture_img->get_has_alpha() << "\n";
		std::cout << "\tbits_per_sample: " << texture_img->get_bits_per_sample() << "\n";
		std::cout << "\twidth: " << texture_img->get_width() << "\n";
		std::cout << "\theight: " << texture_img->get_height() << "\n";	
		std::cout << "\trowstride: " << texture_img->get_rowstride() << "\n";
#endif
	}
	catch (Glib::FileError e) {
		std::cerr << "Texture loading failed due to Glib::FileError: " << e.what() 
			<< "\n";
		throw;
	}
	catch (Gdk::PixbufError e) {
		std::cerr << "Texture loading failed due to Glib::PixbufError: " << e.what()
			<< "\n";
		throw;
	}
	catch (...) {
		std::cerr << "Texture loading failed due to unknown exception.\n";
		throw;
	}
	// TODO: add code to transform the image file format as needed.
}

void
file_texture::gl_init()
{
	glEnable( GL_TEXTURE_2D);
	glGenTextures(1, &handle);
	glBindTexture(GL_TEXTURE_2D, handle);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (image->get_has_alpha()) {
		gluBuild2DMipmaps( GL_TEXTURE_2D, GL_RGBA, image->get_width(),
			image->get_height(), GL_RGBA, GL_UNSIGNED_BYTE, 
			image->get_pixels());
	}
	else {
		gluBuild2DMipmaps( GL_TEXTURE_2D, GL_RGB, image->get_width(),
			image->get_height(), GL_RGB, GL_UNSIGNED_BYTE, 
			image->get_pixels());
	}
	glDisable( GL_TEXTURE_2D);
}

void
file_texture::gl_transform()
{
	glMatrixMode( GL_TEXTURE);
	// Gdk::Pixbuf images are in row-reverse storage order wrt to OpenGL,
	// so it has to be flipped across the y-axis.
	// TODO: This should be handled at load time.
	glLoadIdentity();
	glScalef( 1.0, -1.0, 1.0);
	glMatrixMode( GL_MODELVIEW);
}
