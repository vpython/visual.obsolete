// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

#include <boost/python/detail/wrap_python.hpp>
#include <boost/crc.hpp>

#include "util/errors.hpp"
#include "util/gl_enable.hpp"

#include "python/slice.hpp"
#include "python/extrusion.hpp"

#include <stdexcept>
#include <cassert>
#include <sstream>
#include <iostream>

// Recall that the default constructor for object() is a reference to None.

namespace cvisual { namespace python {

using boost::python::object;
using boost::python::make_tuple;
using boost::python::tuple;

extrusion::extrusion()
	: antialias( true)
{
	contours.insert(contours.begin(), 0.0);
	strips.insert(strips.begin(), 0.0);
	pcontours.insert(pcontours.begin(), 0);
	pstrips.insert(pstrips.begin(), 0);
	indices.insert(indices.begin(), 0);
	normals2D.insert(normals2D.begin(), 0.0);
}

namespace numpy = boost::python::numeric;

//    Serious issue with 32bit vs 64bit machines, apparently,
//    with respect to extract/converting from an array (e.g. double <- int),
//    so for the time being, make sure that in primitives.py one builds
//    the contour arrays as double and int.

void check_array( const array& n_array )
{
	std::vector<npy_intp> dims = shape( n_array );
	if (!(dims.size() == 2 && dims[1] == 2)) {
		throw std::invalid_argument( "This must be an Nx2 array");
	}
}

template <typename T>
void build_contour(const numpy::array& _cont, std::vector<T>& cont)
{
	check_array(_cont);
	std::vector<npy_intp> dims = shape(_cont);
	size_t length = 2*dims[0];
	cont.resize(length);
	T* start = (T*)data(_cont);
	for(size_t i=0; i<length; i++, start++) {
		cont[i] = *start;
		//std::cout << i << "i: " << cont[i] << std::endl;
	}
}

void
extrusion::set_contours( const numpy::array& _contours,  const numpy::array& _pcontours,
		const numpy::array& _strips,  const numpy::array& _pstrips  )
{
	build_contour<double>(_contours, contours);
	build_contour<int>(_pcontours, pcontours);
	/*
	for (size_t i=0; i<contours.size(); i++) {
		std::cout << "contours[" << i << "]=" << contours[i] << std::endl;
	}
	for (size_t i=0; i<pcontours.size(); i++) {
		std::cout << "pcontours[" << i << "]=" << pcontours[i] << std::endl;
	}
	*/
	build_contour<double>(_strips, strips);
	build_contour<int>(_pstrips, pstrips);

	size_t ncontours = pcontours[0];
	if (ncontours == 0) return;
	size_t npoints = contours.size()/2; // total number of 2D points in all contours

	// Set up indices used in glDrawElements
	indices.resize(2*npoints+2*ncontours);
	size_t i=0;
	for (size_t c=0; c < ncontours; c++) {
		int np = pcontours[2*c+2]; // number of 2D points in this contour
		int base = pcontours[2*c+3]; // location of first 2D point of this contour
		for (int pt=0; pt < np; pt++) {
			indices[i] = base+pt+npoints; // location in 3D vector array cp
			indices[i+1] = base+pt;
			i += 2;
		}
		indices[i] = base+npoints;
		indices[i+1] = base;
		i += 2;
	}

	// Set up 2D normals in the shape plane; during rendering these are used to generate 3D normals
	normals2D.resize(2*npoints); // each normal is (x,y), two doubles, relative to origin of shape
	for (size_t c=0; c < ncontours; c++) {
		int np = 2*pcontours[2*c+2]; // number of doubles in this contour
		int base = 2*pcontours[2*c+3]; // location of first (x) member of 2D (x,y) point
		std::cout << "start, c=" << c << " np=" << np << " base=" << base << std::endl;
		vector N, Navg;
		vector Nprev = vector(contours[base+np-1]-contours[base+1],contours[base]-contours[base+np-2],0).norm();
		vector Nlast = Nprev;
		for (int pt=0; pt < np; pt+=2) {
			std::cout << "pt=" << pt << std::endl;
			if (pt < np-2) {
				N = vector(contours[base+pt+1]-contours[base+pt+3], contours[base+pt+2]-contours[base+pt], 0).norm();
			} else {
				N = Nlast;
			}
			std::cout << "calculated N" << std::endl;
			Navg = N;
			double cosangle = N.dot(Nprev);
			if (cosangle > 0.95) {
				Navg = (N+Nprev).norm();
			}
			Nprev = N;
			normals2D[base+pt] = Navg[0];
			normals2D[base+pt+1] = Navg[1];
			std::cout << "< " << Navg[0] << ", " << Navg[1] << " >" << std::endl;
		}
	}

}

void
extrusion::set_antialias( bool aa)
{
	antialias = aa;
}

bool
extrusion::degenerate() const
{
	return count < 2;
}

bool
extrusion::monochrome(float* tcolor, size_t pcount)
{
	rgb first_color( tcolor[0], tcolor[1], tcolor[2]);
	size_t nn;

	for(nn=0; nn<pcount; nn++)  {
		if (tcolor[nn*3] != first_color.red)
			return false;
		if (tcolor[nn*3+1] != first_color.green)
			return false;
		if (tcolor[nn*3+2] != first_color.blue)
			return false;
	}

	return true;
}

namespace {
// Determines if two values differ by more than frac of either one.
bool
eq_frac( double lhs, double rhs, double frac)
{
	if (lhs == rhs)
		return true;
	double diff = fabs(lhs - rhs);
	lhs = fabs(lhs);
	rhs = fabs(rhs);
	return lhs*frac > diff && rhs*frac > diff;
}
} // !namespace (unnamed)

vector
extrusion::get_center() const
{
	// TODO: Optimize this by only recomputing the center when the checksum of
	// the entire object has changed.
	// TODO: Only add the "optimization" if the checksum is actually faster than
	// computing the average value every time...
	if (degenerate())
		return vector();
	vector ret;
	const double* pos_i = pos.data();
	const double* pos_end = pos.end();
	while (pos_i < pos_end) {
		ret.x += pos_i[0];
		ret.y += pos_i[1];
		ret.z += pos_i[2];
		pos_i += 3;
	}
	ret /= count;
	return ret;
}

void
extrusion::gl_pick_render( const view& scene)
{
	// Aack, I can't think of any obvious optimizations here.
	// But since Visual 3 didn't permit picking of curves, omit for now.
	// We can't afford it; serious impact on performance.
	//gl_render( scene);
}

void
extrusion::grow_extent( extent& world)
{
	if (degenerate())
		return;
	const double* pos_i = pos.data();
	const double* pos_end = pos.end();
	for ( ; pos_i < pos_end; pos_i += 3)
		world.add_sphere( vector(pos_i), 1);
		//world.add_sphere( vector(pos_i), radius);
	world.add_body();
}

bool
extrusion::adjust_colors( const view& scene, float* tcolor, size_t pcount)
{
	rgb rendered_color;
	bool mono = monochrome(tcolor, pcount);
	if (mono) {
		// We can get away without using a color array.
		rendered_color = rgb( tcolor[0], tcolor[1], tcolor[2]);
		if (scene.anaglyph) {
			if (scene.coloranaglyph)
				rendered_color.desaturate().gl_set(opacity);
			else
				rendered_color.grayscale().gl_set(opacity);
		}
		else
			rendered_color.gl_set(opacity);
	}
	else {
		glEnableClientState( GL_COLOR_ARRAY);
		if (scene.anaglyph) {
			// Must desaturate or grayscale the color.

			for (size_t i = 0; i < pcount; ++i) {
				rendered_color = rgb( tcolor[3*i], tcolor[3*i+1], tcolor[3*i+2]);
				if (scene.coloranaglyph)
					rendered_color = rendered_color.desaturate();
				else
					rendered_color = rendered_color.grayscale();
				tcolor[3*i] = rendered_color.red;
				tcolor[3*i+1] = rendered_color.green;
				tcolor[3*i+2] = rendered_color.blue;
			}
		}
	}
	return mono;
}

namespace {
template <typename T>
struct converter
{
	T data[3];
};
} // !namespace (anonymous)

void
extrusion::extrude( const view& scene, double* spos, float* tcolor, size_t pcount)
{

	if (pcount < 2) return;

	//std::cout << "--------pcount=" << pcount << std::endl;

	size_t ncontours = pcontours[0];
	if (ncontours == 0) return;
	size_t npoints = contours.size()/2; // total number of 2D points in all contours

	// cp[:npoints] is previous contour points; cp[npoints:] is current contour points
	std::vector<vector> cp(2*npoints);
	std::vector<vector> normals(2*npoints);

	glVertexPointer(3, GL_DOUBLE, sizeof( vector), &cp[0]);
	glNormalPointer( GL_DOUBLE, sizeof(vector), &normals[0]);

	vector xaxis, yaxis; // local unit-vector axes on the 2D shape
	vector prev;

	bool closed = vector(&spos[0]) == vector(&spos[(pcount-1)*3]);

	vector lastA; // unit vector of previous segment, initially <0,0,0>

	// pos and color iterators
	const double* v_i = spos;
	const float* c_i = tcolor;
	bool mono = adjust_colors( scene, tcolor, pcount);

	gl_enable_client vertex_arrays( GL_VERTEX_ARRAY);
	gl_enable_client normal_arrays( GL_NORMAL_ARRAY);
	if (!mono) {
		glEnableClientState( GL_COLOR_ARRAY);
	}

	for (size_t corner=0; corner < pcount; ++corner, v_i += 3, c_i += 3) {
		vector current( &v_i[0] );
		double sectheta;

		vector next, A, bisecting_plane_normal;
		//std::cout << "corner=" << corner << " current=<" << current[0] << "," << current[1] << "," << current[2]<< ">" << std::endl;
		if (corner != pcount-1) {
			next = vector( &v_i[3] ); // The next vector in spos
			A = (next - current).norm();
			if (!A) {
				if (corner == 0) {
					const double* tv_i = v_i;
					for (size_t tcorner=0; tcorner < pcount; ++tcorner, tv_i += 3) {
						A = (vector( &tv_i[3] ) - current).norm();
						if (!A) continue;
					}
					if (!A) { // all the points of this extrusion are at the same location; abort
						return;
					}
					lastA = A;
				} else {
					A = lastA;
				}
			}

			bisecting_plane_normal = (A + lastA).norm();
			if (!bisecting_plane_normal) {  //< Exactly 180 degree bend
				bisecting_plane_normal = vector(0,0,1).cross(A);
				if (!bisecting_plane_normal)
					bisecting_plane_normal = vector(0,1,0).cross(A);
			}

			sectheta = bisecting_plane_normal.dot( lastA );
			if (sectheta) sectheta = 1.0 / sectheta;
		}

		if (corner == 0) {
			yaxis = vector(0,1,0);
			xaxis = A.cross(yaxis).norm();
			if (!xaxis) {
				xaxis = A.cross( vector(0, 0, 1)).norm();
			}
			yaxis = xaxis.cross(A).norm();

			if (!xaxis || !yaxis || xaxis == yaxis) {
				std::ostringstream msg;
				msg << "Degenerate extrusion case! please report the following "
					"information to visualpython-users@lists.sourceforge.net: ";
				msg << "current:" << current << " next:" << next
			 		<< " A:" << A << " x:" << xaxis << " y:" << yaxis
			 		<< std::endl;
			 	VPYTHON_WARNING( msg.str());
			}

			//std::cout << "initial xaxis, yaxis = " << xaxis << " " << yaxis << std::endl;
			size_t proj = 0;
			for (size_t c=0; c < ncontours; c++) {
				size_t np = pcontours[2*c+2]; // number of 2D points in this contour
				size_t base = 2*pcontours[2*c+3]; // initial (x,y) = (contour[base], contour[base+1])
				//std::cout <<"initial c=" << c << " np=" << np << " base=" << base << std::endl;
				for (size_t pt=0; pt < np; pt++, proj++, base+=2) {
					cp[proj+npoints] = scene.gcf*(current + xaxis*contours[base] + yaxis*contours[base+1]);
					normals[proj+npoints] = xaxis*normals2D[base] + yaxis*normals2D[base+1];
					//std::cout << "initial c=" << c << " pt=" << pt << " proj="  << proj << " base=" << base << ": " << cp[proj+npoints]/scene.gcf << std::endl;
				}
			}

		} else {
			vector relx = current - (prev+xaxis);
			vector rely = current - (prev+yaxis);
			//std::cout << "prev=" << prev << " relx, rely = " << relx << " " << rely << std::endl;
			double dx, dy;
			if (corner != (pcount-1) && sectheta > 0.0) {
				dx = (relx.dot(bisecting_plane_normal)) * sectheta;
				dy = (rely.dot(bisecting_plane_normal)) * sectheta;
			} else {
				dx = relx.dot(lastA);
				dy = rely.dot(lastA);
			}
			//std::cout << "dx, dy = " << dx << " " << dy << std::endl;
			xaxis += prev + dx*lastA - current;
			yaxis += prev + dy*lastA - current;

			/*
			std::cout << "--cp before---" << std::endl;
			for (size_t i=0; i<cp.size(); i++) {
				std::cout << "i=" << i << ": " << cp[i]/scene.gcf << std::endl;
			}
			*/

			//std::cout << "xaxis, yaxis = " << xaxis << " " << yaxis << std::endl;
			size_t proj = 0;
			for (size_t c=0; c < ncontours; c++) {
				size_t np = pcontours[2*c+2]; // number of 2D points in this contour
				size_t base = 2*pcontours[2*c+3]; // initial (x,y) = (contour[base], contour[base+1])
				for (size_t pt=0; pt < np; pt++, proj++, base+=2) {
					cp[proj+npoints] = scene.gcf*(current + xaxis*contours[base] + yaxis*contours[base+1]);
					normals[proj+npoints] = xaxis*normals2D[base] + yaxis*normals2D[base+1];
					//std::cout << "initial c=" << c << " pt=" << pt << " proj="  << proj << " base=" << base << ": " << cp[proj+npoints]/scene.gcf << std::endl;
				}
			}


			std::cout << "--cp after---" << std::endl;
			for (size_t i=0; i<cp.size(); i++) {
				std::cout << "i=" << i << " " << cp[i]/scene.gcf << ", normals " << normals[i] << std::endl;
			}

			std::cout << "current=" << current << " npoints=" << npoints << std::endl;
			for (size_t i=0; i<indices.size(); i++) {
				std::cout << "i=" << i << ": " << indices[i] << ": " << cp[indices[i]]/scene.gcf;
				std::cout << " normals" << normals[indices[i]] << std::endl;
			}


			int* ind = &indices[0];
			for (size_t c=0; c < ncontours; c++) {
				int np = 2*pcontours[2*c+2]+2;
				//std::cout << "c=" << c << " np=" << np << std::endl;
				// Generate triangle strips and normals for this contour of the extrusion:
				glDrawElements(GL_QUAD_STRIP, np, GL_UNSIGNED_INT, ind);
				ind += np;
			}

		}
		lastA = A;
		prev = current;
		//std::cout << "endloop prev, current = " << prev << " " << current << std::endl;
		for (size_t i=0; i<npoints; i++) {
			cp[i] = cp[i+npoints];
			normals[i] = normals[i+npoints];
		}
	}
	if (!mono) {
		glDisableClientState( GL_COLOR_ARRAY);
	}
}

	/* Ignore for now
	if (closed) {
		// Connect the end of the extrusion to the start... can be ugly because the basis has gotten twisted around!
		size_t i = (vcount - 1)*sides;
		for(size_t a=0; a<sides; a++) {
			projected[i+a] = projected[a];
			normals[i+a] = normals[a];
			if (!mono) light[i+a] = light[a];
		}
	}
	*/

	// We want to smooth the normals at the joints.
	// But that can make a sharp corner do odd things,
	// so we smoothly disable the smoothing when the joint angle
	// is too big. This is somewhat arbitrary but seems to work well.
	/*
	size_t prev_i = closed ? (vcount-1)*sides : 0;
	for( i = closed ? 0 : sides; i < vcount*sides; i += 2*sides ) {
		for(size_t a=0; a<sides; a++) {
			vector& n1 = normals[i+a];
			vector& n2 = normals[prev_i+a];
			double smooth_amount = (n1.dot(n2) - .65) * 4.0;
			smooth_amount = std::min(1.0, std::max(0.0, smooth_amount));
			if (smooth_amount) {
				vector n_smooth = (n1+n2).norm() * smooth_amount;
				n1 = n1 * (1-smooth_amount) + n_smooth;
				n2 = n2 * (1-smooth_amount) + n_smooth;
			}
		}
		prev_i = i + sides;
	}
	*/

	/*
	gl_enable_client vertex_arrays( GL_VERTEX_ARRAY);
	gl_enable_client normal_arrays( GL_NORMAL_ARRAY);
	if (!mono) {
		glEnableClientState( GL_COLOR_ARRAY);
	}

	// curve_slice had to do with the multi-sided thick curve; this code must change
	int *ind = extrusion_slice;
	for (size_t a=0; a < sides; a++) {
		size_t ai = a;
		if (a == sides-1) {
			ind += 256; // upper portion of extrusion_slice indices, for the last side
			ai = 0;
		}

		// List all the vertices for the ai-th side of the thick line:
		for (size_t i = 0; i < vcount; i += 127u) {
			glVertexPointer(3, GL_DOUBLE, sizeof( vector), &projected[i*sides + ai].x);
			if (!mono)
				glColorPointer(3, GL_FLOAT, sizeof( rgb), &light[(i*sides + ai)].red );
			glNormalPointer( GL_DOUBLE, sizeof(vector), &normals[i*sides + ai].x);
			if (vcount-i < 128)
				glDrawElements(GL_TRIANGLE_STRIP, 2*(vcount-i), GL_UNSIGNED_INT, ind);
			else
				glDrawElements(GL_TRIANGLE_STRIP, 256u, GL_UNSIGNED_INT, ind);
		}
	}
	if (!mono)
		glDisableClientState( GL_COLOR_ARRAY);
}
	*/

void
extrusion::gl_render( const view& scene)
{
	if (degenerate())
		return;
	const size_t true_size = count;
	// Set up the leading and trailing points for the joins.  See
	// glePolyCylinder() for details.  The intent is to create joins that are
	// perpendicular to the path at the last segment.  When the path appears
	// to be closed, it should be rendered that way on-screen.
	// The maximum number of points to display.
	const int LINE_LENGTH = 1000;
	// Data storage for the position and color data (plus room for 3 extra points)
	double spos[3*(LINE_LENGTH+3)];
	float tcolor[3*(LINE_LENGTH+3)]; // opacity not yet implemented for extrusions
	float fstep = (float)(count-1)/(float)(LINE_LENGTH-1);
	if (fstep < 1.0F) fstep = 1.0F;
	size_t iptr=0, iptr3, pcount=0;

	const double* p_i = pos.data();
	const double* c_i = color.data();

	// Choose which points to display
	for (float fptr=0.0; iptr < count && pcount < LINE_LENGTH; fptr += fstep, iptr = (int) (fptr+.5), ++pcount) {
		iptr3 = 3*iptr;
		spos[3*pcount] = p_i[iptr3];
		spos[3*pcount+1] = p_i[iptr3+1];
		spos[3*pcount+2] = p_i[iptr3+2];
		tcolor[3*pcount] = c_i[iptr3];
		tcolor[3*pcount+1] = c_i[iptr3+1];
		tcolor[3*pcount+2] = c_i[iptr3+2];
	}

	/*
	// Do scaling if necessary
	std::cout << "scene.gcf=" << scene.gcf << " scene.gcfvec=" << scene.gcfvec << std::endl;
	if (scene.gcf != 1.0) {
		for (size_t i = 0; i < pcount; ++i) {
			spos[3*i] *= scene.gcf;
			spos[3*i+1] *= scene.gcf;
			spos[3*i+2] *= scene.gcf;
		}
	}
	*/

	clear_gl_error();

	extrude( scene, spos, tcolor, pcount);

	check_gl_error();
}

void
extrusion::outer_render( const view& v ) {
	arrayprim::outer_render(v);
}

void
extrusion::get_material_matrix( const view& v, tmatrix& out ) {
	if (degenerate()) return;

	// TODO: note this code is identical to faces::get_material_matrix, except for considering radius

	// TODO: Add some caching for extent with grow_extent etc
	vector min_extent, max_extent;
	const double* pos_i = pos.data();
	const double* pos_end = pos.end();
	min_extent = max_extent = vector( pos_i ); pos_i += 3;
	while (pos_i < pos_end)
		for(int j=0; j<3; j++) {
			if (*pos_i < min_extent[j]) min_extent[j] = *pos_i;
			else if (*pos_i > max_extent[j]) max_extent[j] = *pos_i;
			pos_i++;
		}

	//min_extent -= vector(radius,radius,radius);
	//max_extent += vector(radius,radius,radius);

	out.translate( vector(.5,.5,.5) );
	out.scale( vector(1,1,1) * (.999 / (v.gcf * std::max(max_extent.x-min_extent.x, std::max(max_extent.y-min_extent.y, max_extent.z-min_extent.z)))) );
	out.translate( -.5 * v.gcf * (min_extent + max_extent) );
}

} } // !namespace cvisual::python
