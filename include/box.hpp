#ifndef VPYTHON_BOX_HPP
#define VPYTHON_BOX_HPP

#include "simple_displayobject.hpp"
#include "util/sorted_model.hpp"
#include "util/texture.hpp"
#include "util/displaylist.hpp"

class box : public simple_displayobject
{
 private:
	double width;
	double height;
	// This object may be textured.
	shared_ptr<texture> tex;

	// A model to be used for rendering transparent objects.
	static z_sorted_model<quad, 6> sorted_model;
	// A model to be used for transparent and textured objects.
	static z_sorted_model<tquad, 6> textured_sorted_model;
	// The global model to use for simple quad objects.
	static displaylist simple_model;
	// The global model to use for opaque textured objects.
	static displaylist textured_model;
	// A flag to determine if the static models have been initialized.
	static bool first;
	// Calculates the sorted_model object (only done once).
	void calc_sorted_model();
	
	// Calculates the textured_sorted_model (only done once, hopefully).
	void calc_textured_sorted_model();
	
	// True if the box should not be rendered.
	bool degenerate();
	
 public:
	box();
	void set_width( const double&);
	void set_height( const double&);
	void set_length( const double&);
	inline void set_texture( shared_ptr<texture> t)
	{ tex = t; }
	
 protected:
	virtual void gl_pick_render( const view&);
	virtual void update_cache( const view&);
	virtual void gl_render( const view&);
	virtual void grow_extent( extent&);
	SIMPLE_DISPLAYOBJECT_TYPEINFO_DECL;
};

#endif // !defined VPYTHON_BOX_HPP
