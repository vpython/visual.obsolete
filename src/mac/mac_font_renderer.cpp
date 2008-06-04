#include "font_renderer.hpp"
#include "util/errors.hpp"
#include "mac/display.hpp"

namespace cvisual {

/**************** aglFont implementation *******************/


aglFont::aglFont(struct aglContext& _cx, 
		 const char *name, 
		 double size) 
	: cx(_cx), refcount(1)
{
		unsigned char pName[256];
		int ok;
		
		// Font names vary across platforms, so fall back if necessary
		strcpy((char *)(&(pName[1])), name);
		pName[0] = strlen(name);
		fID = FMGetFontFamilyFromName((unsigned char *)pName);
		if (fID <= 0)
			fID = GetAppFont();
		// No size means default
		if (size <= 0)
			size = GetDefFontSize();
		
		fSize = (int)size;
		FetchFontInfo(fID, fSize, 0, &fInfo);
		
		cx.makeCurrent();
		listBase = glGenLists(256);
		ok = aglUseFont(cx.getContext(), fID, 0, fSize, 0, 256, listBase);
		// if (! ok)	um, report something? Unlikely
		cx.makeNotCurrent();
}

aglFont::~aglFont()
{
	fID = -1;
}

void
aglFont::draw(const std::wstring& c)
{
	if (fID >= 0) {
		glListBase(listBase);
		//glCallLists(strlen(c), GL_UNSIGNED_BYTE, c);
	}
}

double
aglFont::getWidth(const std::wstring& c)
{
	int		tw;
	
	if (fID == 0)
		return 0;
	
	// Still using old QuickDraw text measurement because I can't
	// figure out how to use ATSUI
	cx.makeCurrent();
	TextFont(fID);
	TextFace(0);
	TextSize(fSize);
	
	//tw = TextWidth(c, 0, strlen(c));
	
	return (double)(tw  * 2) / cx.width();
}

double
aglFont::ascent()
{
	return (double)fInfo.ascent * 2 / cx.height();
}

double
aglFont::descent()
{
	return (double)fInfo.descent * 2 / cx.height();
}

void
aglFont::release()
{
	refcount--;
	if (refcount <= 0) {
		cx.add_pending_glDeleteList( listBase, 256);
		delete(this);
	}
}

/*
aglFont*
aglContext::getFont(const char* description, double size)
{
	return new aglFont(*this, description, size);
}
*/

font_renderer::font_renderer( const std::wstring& description, int height ) {
	/*
	font_handle = NULL;
	
	// TODO: support generic "sans-serif", "serif", "monospace" families using lfPitchAndFamily.
	// Doesn't matter much because Windows machines pretty much always have "verdana", 
	// "times new roman", and "courier new".

	// Respect the users' preferences as to whether ClearType should be enabled.
	isClearType = isClearTypeEnabled();
	int quality = DEFAULT_QUALITY;
	if (isClearType) quality = 5;
	
	HDC sic = CreateIC( "DISPLAY", NULL, NULL, NULL );
	
	LOGFONTW lf;
	memset(&lf, 0, sizeof(lf));
	lf.lfHeight = -height;
	lf.lfOutPrecision = OUT_TT_PRECIS;
	lf.lfQuality = quality;
	wcsncpy( lf.lfFaceName, description.c_str(), sizeof(lf.lfFaceName)/2-1 );
	lf.lfFaceName[ sizeof(lf.lfFaceName)/2-1 ] = 0;
	
	bool fontFound = false;
	EnumFontFamiliesExW( sic, &lf, (FONTENUMPROCW)ef_callback, (LPARAM)&fontFound, 0 );
	if (fontFound)
		font_handle = CreateFontIndirectW( &lf );

	if (font_handle)  
		SelectObject( sic, SelectObject( sic, font_handle ) ); //< Work around KB306198
	
	DeleteDC( sic );
	*/
}

bool font_renderer::ok() {
	return font_handle != 0;
}

font_renderer::~font_renderer() {
	/*
	if (font_handle)
		DeleteObject( font_handle );
	*/
}

void font_renderer::gl_render_to_texture( const view&, const std::wstring& text, layout_texture& tx ) {
	/*
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
		
		int biPitch = (hdr.biWidth*3 + ((-hdr.biWidth*3)&3));
		memset(bits, 0, biPitch * hdr.biHeight);
		
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
	*/
	}

} // namespace cvisual
