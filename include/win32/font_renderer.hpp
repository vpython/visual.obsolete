#ifndef VPYTHON_WIN32_FONT_RENDERER_HPP
#define VPYTHON_WIN32_FONT_RENDERER_HPP
#pragma once

// See text.hpp for public interface

#include "text.hpp"

namespace cvisual {

class font_renderer {
 public:
	// Create a font_renderer for the requested font.  If the requested font
	//   is not available, returns a similar or default font with the given height.
	font_renderer( const std::wstring& description, int height );
	
	// Render text and call tx.set_image()
	void gl_render_to_texture( const struct view&, const std::wstring& text, layout_texture& tx );

	~font_renderer();

 private:
	void* font_handle;
	bool isClearType;
};

} // namespace cvisual

#endif