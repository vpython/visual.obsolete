#ifndef VPYTHON_BOX_HPP
#define VPYTHON_BOX_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "rectangular.hpp"
#include "util/sorted_model.hpp"
#include "util/displaylist.hpp"

namespace cvisual {

class box : public rectangular
{
 private:
	// A model to be used for rendering transparent objects.
	static z_sorted_model<quad, 600> sorted_model;
	// A model to be used for transparent and textured objects.
	static z_sorted_model<tquad, 600> textured_sorted_model;
	// The global model to use for simple quad objects.
	static displaylist simple_model;
	// The global model to use for opaque textured objects.
	static displaylist textured_model;
	// A flag to determine if the static models have been initialized.
	static bool first;
	
	// Calculates the sorted_model object (only done once).
	void calc_sorted_model();
	// Calculates the sorted_model, based on the subdivision level
	// (only done once, hopefully).
	void calc_sorted_model(quad *faces, int level);
	
	
	// Calculates the textured_sorted_model (only done once, hopefully).
	void calc_textured_sorted_model();
	// Calculates the textured_sorted_model, based on the subdivision level
	// (only done once, hopefully).
	void calc_textured_sorted_model(tquad *faces, int level);
	
	// True if the box should not be rendered.
	bool degenerate();
	
 public:
	box();
	box( const box& other);
	virtual ~box();
	
 protected:
	virtual void gl_pick_render( const view&);
	virtual void update_cache( const view&);
	virtual void gl_render( const view&);
	virtual void grow_extent( extent&);
	PRIMITIVE_TYPEINFO_DECL;
};

} // !namespace cvisual

#endif // !defined VPYTHON_BOX_HPP
