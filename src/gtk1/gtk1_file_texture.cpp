#include <glib.h>
#include "gtk1/file_texture.hpp"

#include <iostream>
#include <cstdlib>

#include <GL/gl.h>
#include <GL/glu.h>


file_texture::~file_texture()
{	
}

shared_ptr<texture>
file_texture::create( const std::string& filename)
{
	return shared_ptr<texture>( new file_texture( filename));
}


file_texture::file_texture( const std::string& file)
	: filename(file), image( gdk_pixbuf_new_from_file(filename.c_str()))
{
	// image = gdk_pixbuf_new_from_file( file.c_str());
	// Check for errors.
	if (!image) {
		std::cerr << "Image loading failed due to unknown error.  Aborting.\n";
		std::exit(1);	
	}
}

void
file_texture::gl_init()
{
	glEnable( GL_TEXTURE_2D);
	glGenTextures(1, &handle);
	glBindTexture(GL_TEXTURE_2D, handle);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (gdk_pixbuf_get_has_opacity(image)) {
		gluBuild2DMipmaps( GL_TEXTURE_2D, GL_RGBA, gdk_pixbuf_get_width(image),
			gdk_pixbuf_get_height(image), GL_RGBA, GL_UNSIGNED_BYTE, 
			gdk_pixbuf_get_pixels(image));
	}
	else {
		gluBuild2DMipmaps( GL_TEXTURE_2D, GL_RGB, gdk_pixbuf_get_width(image),
			gdk_pixbuf_get_height(image), GL_RGB, GL_UNSIGNED_BYTE, 
			gdk_pixbuf_get_pixels(image));
	}
	glDisable( GL_TEXTURE_2D);
	gdk_pixbuf_unref( image);
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
