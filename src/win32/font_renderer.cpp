#include "font_renderer.hpp"
#include "util/errors.hpp"

namespace cvisual {

using std::wstring;

class win32_exception : std::exception {
 public:
	win32_exception( const char* desc ) : std::exception(desc) {
		// todo: report GetLastError(), see win32_write_critical in windisplay.cpp
	}
};

static void * createFont( const wstring& desc, int height, bool isClearType ) {
	int quality = DEFAULT_QUALITY;
	
	if (isClearType) quality = 5;

	return CreateFontW( height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_TT_PRECIS,
						CLIP_DEFAULT_PRECIS, quality, DEFAULT_PITCH | FF_DONTCARE,
						desc.size() ? desc.c_str() : NULL );
}

font_renderer::font_renderer( const wstring& description, int height ) {
	if (height<0) height = 0; // default
	
	// Respect the users' preferences as to whether ClearType should be enabled.
	isClearType = false;
	UINT smoothType = 0;
	// On versions of Windows < XP, this call should fail
	if (SystemParametersInfo( 0x200a, 0, &smoothType, 0 )) {  // SPI_GETFONTSMOOTHINGTYPE
		if (smoothType == 2) // FE_FONTSMOOTHINGCLEARTYPE
			isClearType = true;
	}

	font_handle = createFont( description, height, isClearType );
	if (!font_handle) {
		VPYTHON_WARNING( "Could not allocate requested font, "
			"falling back to system default");
		font_handle = createFont( wstring(), height, isClearType );
	}
	
	// Work around KB306198
	HDC sic = CreateIC( "DISPLAY", NULL, NULL, NULL );
	SelectObject( sic, SelectObject( sic, font_handle ) );
	DeleteDC( sic );
}

font_renderer::~font_renderer() {
	if (font_handle)
		DeleteObject( font_handle );
}

void font_renderer::gl_render_to_texture( const view&, const wstring& text, layout_texture& tx ) {
	HDC dc = NULL;
	HBITMAP bmp = NULL;
	HFONT prevFont = NULL;

	try {
		dc = CreateCompatibleDC( NULL );

		prevFont = (HFONT)SelectObject( dc, font_handle );
		
		RECT rect;
		rect.left = 0;
		rect.top = 0;
		rect.right = 1024;
		rect.bottom = 1024;
		
		if (!DrawTextW( dc, text.c_str(), text.size(), &rect, DT_CALCRECT ))
			throw win32_exception("DrawText(DT_CALCRECT) failed.");
			
		if (!rect.right) rect.right = 1;
		if (!rect.bottom) rect.bottom = 1;

		BITMAPINFOHEADER hdr;
		memset(&hdr, 0, sizeof(hdr));
		hdr.biSize = sizeof(hdr);
		hdr.biWidth = rect.right;
		hdr.biHeight = rect.bottom;
		hdr.biPlanes = 1;
		hdr.biBitCount = 24;
		hdr.biCompression = BI_RGB;
		
		void* bits;
		bmp = CreateDIBSection( dc, (BITMAPINFO*)&hdr, DIB_RGB_COLORS, &bits, NULL, 0 );
		if (!bmp) throw win32_exception("CreateDIBSection failed.");
		
		memset(bits, 0, (hdr.biWidth*3 + (-hdr.biWidth*3)&3) * hdr.biHeight);
		
		SetTextColor(dc, 0xFFFFFF);
		SetBkColor(dc, 0);
		
		HBITMAP prevBmp = (HBITMAP)SelectObject( dc, bmp );
		DrawTextW( dc, text.c_str(), text.size(), &rect, 0 );
		SelectObject(dc, prevBmp);
		
		SelectObject(dc, prevFont);
		DeleteDC( dc ); dc = NULL;
		
		tx.set_image( rect.right, -rect.bottom, isClearType ? GL_RGB : GL_LUMINANCE, GL_BGR_EXT, GL_UNSIGNED_BYTE, 4, bits );

		DeleteObject( bmp ); bmp = NULL;
	} catch( ... ) {
		if (bmp) DeleteObject( bmp );
		if (dc) { SelectObject(dc, prevFont); DeleteDC( dc ); }
		
		throw;
	}
}

} // namespace cvisual