#ifndef VISUAL_GLCONTEXT_H
#define VISUAL_GLCONTEXT_H

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "util/vector.hpp"
#include <utility>

namespace cvisual {
/*
 * These classes are canonically defined here.  The actual implementations
 * are in the wgl, xgl, agl .h and .cpp files. 
 */


class glFont 
{
	virtual double getWidth(const char* string) = 0;
	// returns the horizontal extent of the given string in
	//   viewport coordinates (0.0 to 1.0)

	virtual double ascent() = 0;
	// returns the maximum distance from the baseline to the top
	//   of a glyph (e.g. "M")

	virtual double descent() = 0;
	// returns the maximum distance from the baseline to the bottom
	//   of a glyph (e.g. "g")

	virtual void draw(const char* string) = 0;
	// draws string with the current glRasterPos at its left baseline

	virtual void release() = 0;
	// call once for each call to glContext::getFont()
	
	// Defined in glDevice.cpp
	virtual ~glFont();
};

class glContext 
{
public:
	virtual void lockMouse() = 0;
	virtual void unlockMouse() = 0;
	virtual int  getMouseButtons() = 0;
	virtual int  getMouseButtonsChanged() = 0;
	virtual int  getShiftKey() = 0;
	virtual int  getAltKey() = 0;
	virtual int  getCtrlKey() = 0;
	virtual vector  getMouseDelta() = 0;
	virtual vector  getMousePos() = 0;
	virtual std::string  getKeys() = 0;

	glContext() {};
	
	// Do not delete contexts directly, as there can be
	// nasty interactions with the event loop thread.
	// Use the destroy_context function instead.
	virtual ~glContext() {};
	friend void destroy_context (glContext * cx);

	virtual bool initWindow( const char* title, int x, int y, int width, int height, int flags ) = 0;
	virtual bool changeWindow( const char* title, int x, int y, int width, int height, int flags ) = 0;
	virtual bool isOpen() = 0;
 
	virtual void makeCurrent() = 0;
	virtual void makeNotCurrent() = 0;
	virtual void swapBuffers() = 0;

	virtual int winX() = 0;
	virtual int winY() = 0;
	virtual int width() = 0;       // of GL area
	virtual int height() = 0;      // of GL area

	virtual std::string lastError() = 0;

	virtual glFont* getFont(const char* description, double size) = 0;
	// xxx need to document parameters!
		
	enum {
		DEFAULT    = 0,
		FULLSCREEN = 0x1,
		QB_STEREO  = 0x2
	} WindowFlags;
	
	void add_pending_glDeleteList(int base, int howmany);
	
 private:
    // This is a list of glDeleteLists calls that should be made the next time
    // the context is made active.
    mutex list_lock;
	std::vector<std::pair<int, int> > pending_glDeleteLists;

 protected:
	// Implementors of this class should call this function in their implementation
	// of makeCurrent();
 	void delete_pending_lists();
};

// Implemented by platform
void destroy_context (glContext * cx);

} // !namespace visual

#endif // !VISUAL_GLCONTEXT_H
