#include "font_renderer.hpp"
#include "util/errors.hpp"
//#include "mac/display.hpp"
#include <ApplicationServices/ApplicationServices.h>

namespace cvisual {

bool ucs4_to_utf16( const std::wstring& in, std::vector<unsigned short>& out ) {
	out.clear();
	for(int i=0; i<in.size(); i++) {
		if (in[i] >= 0x110000) return false;
		if (in[i] >= 0x10000) {
			out.push_back( 0xD800 + ((in[i] - 0x10000) >> 10) );
			out.push_back( 0xDC00 + ((in[i] - 0x10000) & 1023) );
		} else
			out.push_back( in[i] );
	}
	return true;
}

bool ucs4_to_ascii( const std::wstring& in, std::vector<unsigned char>& out ) {
	out.clear();
	for(int i=0; i<in.size(); i++) {
		if (in[i] >= 0x80) return false;
		out.push_back( in[i] );
	}
	return true;
}

font_renderer::font_renderer( const std::wstring& description, int height ) {
	fontID = kATSUInvalidFontID;
	this->height = height;

	// TODO: support generic "sans-serif", "serif", "monospace" families.
	// Doesn't matter much if Macs always have "verdana", "times new roman", and "courier new".

	// TODO: research suggests kFontUnicodePlatform might not work for all fonts!  Try
	// kFontMacintoshPlatform and ascii description as well?
	std::vector<unsigned char> desc;
	if (!ucs4_to_ascii( description, desc )) return;
	ATSUFontID fid;
	if (ATSUFindFontFromName( &desc[0], desc.size()*sizeof(desc[0]), 
			kFontFamilyName,  //< ?
			kFontMacintoshPlatform,//kFontUnicodePlatform,
			kFontNoScriptCode,
			kFontNoLanguageCode,
			&fid))
		return;

	fontID = fid;
}

bool font_renderer::ok() {
	return fontID != kATSUInvalidFontID;
}

font_renderer::~font_renderer() {
}

void font_renderer::gl_render_to_texture( const view&, const std::wstring& text, layout_texture& tx ) {
	std::vector< unsigned short > text_16;
	static int pfactor = 65536;
	if (!ucs4_to_utf16( text, text_16 ))
		throw std::runtime_error("Encoding conversion failed.");

	// Compute correct point size. height is in pixels, not "points", and I
	// don't know what part of the character is measured by point_size.
	Fixed point_size = (height+1)* pfactor;

	ATSUStyle style;
	if (ATSUCreateStyle(&style)) throw std::runtime_error("ATSUCreateStyle failed.");

	ATSUAttributeTag attr_tag[] = { kATSUFontTag, kATSUSizeTag };
	ByteCount attr_size[] = { sizeof(fontID), sizeof(point_size) };
	ATSUAttributeValuePtr attr_ptr[] = { &fontID, &point_size };
	int a;
	if (a=ATSUSetAttributes(style, sizeof(attr_tag)/sizeof(attr_tag[0]), attr_tag, attr_size, attr_ptr)) {
		throw std::runtime_error("ATSUSetAttributes failed.");
	}
	
	UniCharCount toEnd = kATSUToTextEnd;
	ATSUTextLayout layout;
	if (ATSUCreateTextLayoutWithTextPtr( &text_16[0], 0, kATSUToTextEnd, text_16.size(), 
										 1, &toEnd, &style, &layout ))
		throw std::runtime_error("ATSUCreateTextLayoutWithTextPtr failed.");

	// TODO: support multiple lines with ATSUBatchBreakLines, ATSUGetSoftLineBreaks
	
	Rect text_rect;
	if (ATSUMeasureTextImage( layout, 0, kATSUToTextEnd, 0, 0, &text_rect ))
		throw std::runtime_error("ATSUMeasureTextImage failed.");
	int ww = text_rect.right - text_rect.left+2, 
		hh = text_rect.bottom - text_rect.top;

	CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
	boost::scoped_array<uint8_t> pix_buf( new uint8_t[ww*4*hh] );
	memset( pix_buf.get(), 0x80, ww*4*hh );
	CGContextRef cx = CGBitmapContextCreate(pix_buf.get(),ww,hh,8,ww*4,colorspace,kCGImageAlphaPremultipliedLast);
	float rect[4] = {0,0,ww,hh};
	float transparent[] = {0,0,0,1}, text_color[] = {1,1,1,1};
	CGContextSetFillColorSpace( cx, colorspace );
	CGContextSetFillColor( cx, transparent );
	CGContextFillRect(cx, *(CGRect*)rect);
	CGContextSetFillColor( cx, text_color );
	CGContextSetShouldSmoothFonts( cx, true );
	CGContextSetShouldAntialias( cx, true );
	CGContextSetAllowsAntialiasing( cx, true );
	
	{
		ATSUAttributeTag attr_tag[] = { kATSUCGContextTag };
		ByteCount attr_size[] = { sizeof(CGContextRef) };
		ATSUAttributeValuePtr attr_ptr[] = { &cx };
		if (ATSUSetLayoutControls (layout, sizeof(attr_tag)/sizeof(attr_tag[0]), attr_tag, attr_size, attr_ptr)) {
			throw std::runtime_error("ATSUSetLayoutControls failed.");
		}
	}
	
	if (ATSUDrawText( layout, 0, kATSUToTextEnd, -(text_rect.left-1)*pfactor, text_rect.bottom*pfactor ))
		throw std::runtime_error("ATSUDrawText failed.");

	CGContextRelease( cx );
	
	tx.set_image( ww, hh, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, 4, pix_buf.get() );
	
	// Cleanup (TODO: needed in exception cases also!)
	CGColorSpaceRelease( colorspace );
	
	ATSUDisposeTextLayout( layout );
	ATSUDisposeStyle( style );
}

} // namespace cvisual
