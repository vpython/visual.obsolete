#ifndef VPYTHON_BOX_HPP
#define VPYTHON_BOX_HPP

#include "simple_displayobject.hpp"
#include "util/sorted_model.hpp"
#include "util/texture.hpp"
#include "util/displaylist.hpp"

#include <boost/scoped_ptr.hpp>
using boost::scoped_ptr;

class box : public simple_displayobject
{
 private:
	double width;
	double height;
	// A model to be used for rendering transparent objects.
	scoped_ptr<z_sorted_model<quad, 6> > sorted_model;
	
	// This object may be textured.
	shared_ptr<texture> tex;
	// A model to be used for transparent and textured objects.
	scoped_ptr<z_sorted_model<tquad, 6> > textured_sorted_model;
	
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
	
 public:
	box();
	void set_width( const double&);
	void set_height( const double&);
	void set_length( const double&);
	inline void set_texture( shared_ptr<texture> t)
	{ tex = t; }
	
 protected:
	virtual vector get_scale() const;
	virtual void update_cache( const view&);
	virtual void gl_render( const view&);
	virtual void grow_extent( extent&);
	virtual void update_z_sort( const view&);
};

#endif // !defined VPYTHON_BOX_HPP
