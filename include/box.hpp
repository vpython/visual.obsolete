#ifndef VPYTHON_BOX_HPP
#define VPYTHON_BOX_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include "rectangular.hpp"
#include "util/sorted_model.hpp"
#include "util/displaylist.hpp"

// Levels of detail for boxes. box_Ln = 8 means a face has 8*8 subdivisions
#define box_L0 2
#define box_L1 3
#define box_L2 5
#define box_L3 10
#define box_L4 15
#define box_L5 20

namespace cvisual {

class box : public rectangular
{
 private:
	/** The level-of-detail caches.  They are stored for the life of the program,
		and initialized when the first box is rendered. 
	*/
 	static displaylist lod_cache[6];
 	static displaylist lod_textured_cache[6];
 	
 	// The following kludge is hopefully temporary,
 	// in order to try out the basic scheme. This
 	// needs to be cleaned up eventually. The problem
 	// is that z_sorted_model definitions require
 	// literal numeric values.
 	
 	// Models to be used for rendering opaque objects.
	static z_sorted_model<quad, 6*box_L0*box_L0> simple_model_0;
	static z_sorted_model<quad, 6*box_L1*box_L1> simple_model_1;
	static z_sorted_model<quad, 6*box_L2*box_L2> simple_model_2;
	static z_sorted_model<quad, 6*box_L3*box_L3> simple_model_3;
	static z_sorted_model<quad, 6*box_L4*box_L4> simple_model_4;
	static z_sorted_model<quad, 6*box_L5*box_L5> simple_model_5;
	// Models to be used for transparent and textured objects.
	static z_sorted_model<tquad, 6*box_L0*box_L0> textured_model_0;
	static z_sorted_model<tquad, 6*box_L1*box_L1> textured_model_1;
	static z_sorted_model<tquad, 6*box_L2*box_L2> textured_model_2;
	static z_sorted_model<tquad, 6*box_L3*box_L3> textured_model_3;
	static z_sorted_model<tquad, 6*box_L4*box_L4> textured_model_4;
	static z_sorted_model<tquad, 6*box_L5*box_L5> textured_model_5;
	
	// Calculates the sorted_model, based on the subdivision level
	// (only done once, hopefully).
	void calc_simple_model(quad *faces, int level);
	
	// Calculates the textured_sorted_model, based on the subdivision level
	// (only done once, hopefully).
	void calc_textured_model(tquad *faces, int level);
 	
	// A flag to determine if the static models have been initialized.
	static bool first;
	
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
