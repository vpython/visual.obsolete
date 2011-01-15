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
	: antialias( true), enabled( false)
{
	contours.insert(contours.begin(), 0.0);
	strips.insert(strips.begin(), 0.0);
	pcontours.insert(pcontours.begin(), 0);
	pstrips.insert(pstrips.begin(), 0);
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
	}
}

void
extrusion::set_contours( const numpy::array& _contours,  const numpy::array& _pcontours,
		const numpy::array& _strips,  const numpy::array& _pstrips  )
{
	enabled = false; // block rendering while set_contours processes a shape change
	// primitives.py sends to set_contours descriptions of the 2D surface; see extrusions.hpp
	// We store the information in std::vector containers in flattened form.
	build_contour<double>(_contours, contours);
	build_contour<int>(_pcontours, pcontours);

	build_contour<double>(_strips, strips);
	build_contour<int>(_pstrips, pstrips);

	size_t ncontours = pcontours[0];
	if (ncontours == 0) return;
	size_t npoints = contours.size()/2; // total number of 2D points in all contours

	maxextent = 0.; // biggest distance of the edge of the 2D surface from the curve
	maxcontour = 0; // maximum number of points in any of the contours
	for (size_t c=0; c < ncontours; c++) {
		size_t nd = 2*pcontours[2*c+2]; // number of doubles in this contour
		size_t base = 2*pcontours[2*c+3]; // location of first (x) member of 2D (x,y) point
		if (nd/2 > maxcontour) maxcontour = nd/2;
		for (size_t pt=0; pt < nd; pt+=2) {
			double dist = vector(contours[base+pt],contours[base+pt+1],0.).mag();
			if (dist > maxextent) maxextent = dist;
		}
	}

	// When there is just one contour, Polygon returns a winding order corresponding to
	// the winding order of the list of points given to Polygon, whereas if there is more
	// than one contour, Polygon correctly returns a winding order corresponding to
	// whether the contour is external (clockwise) or internal (counterclockwise).
	// Therefore, check for a counterclockwise single contour and reverse it.
	if (ncontours == 1) {
		double sum = 0.;
		for (size_t pt=0; pt<npoints; pt++) {
			size_t pt2 = (pt+1) % npoints;
			vector r1 = vector(contours[2*pt], contours[2*pt+1], 0);
			vector r2 = vector(contours[2*pt2], contours[2*pt2+1], 0);
			sum += r1.cross(r2).z;
		}
		if (sum >= 0.) { // sum > 0 means this contour runs counterclockwise and must be reversed
			size_t end = npoints;
			std::vector<double> temp(2*end);
			for (size_t i=0; i<end; i++) {
				temp[2*end-2-2*i] = contours[2*i];
				temp[2*end-1-2*i] = contours[2*i+1];
			}
			contours.swap(temp);
		}
	}

	// Set up 2D normals used to build GL_tris
	// There are two per vertex, because each face is a quad, and each vertex appears in two adjacent tris
	normals2D.resize(4*npoints); // each normal is (x,y), two doubles; there are two normals per vertex (2 tris per vertex)
	size_t i=0;
	for (size_t c=0; c < ncontours; c++) {
		size_t nd = 2*pcontours[2*c+2]; // number of doubles in this contour
		size_t base = 2*pcontours[2*c+3]; // location of first (x) member of 2D (x,y) point
		vector N, Nbefore, Nafter, Navg1, Navg2;
		for (size_t pt=0; pt < nd; pt+=2) {
			if (pt == 0) {
				Nbefore = vector(contours[base+nd-1]-contours[base+1], contours[base]-contours[base+nd-2], 0).norm();
				N =       vector(contours[base+1]-contours[base+3], contours[base+2]-contours[base], 0).norm();
			}
			int after  = (pt+2); // use modulo arithmetic to make the linear sequence effectively circular
			Nafter  = vector(contours[base+((after+1)%nd)]-contours[base+((after+3)%nd)],
					         contours[base+((after+2)%nd)]-contours[base+(after%nd)], 0).norm();
			double cosangle = N.dot(Nbefore);
			Navg1 = Navg2 = N;
			if (cosangle > 0.95) {
				Navg1 = (N+Nbefore).norm();
			}
			cosangle = N.dot(Nafter);
			if (cosangle > 0.95) {
				Navg2 = (N+Nafter).norm();
			}
			Nbefore = N;
			N = Nafter;
			normals2D[i  ] = Navg1[0];
			normals2D[i+1] = Navg1[1];
			normals2D[i+2] = Navg2[0];
			normals2D[i+3] = Navg2[1];
			i += 4;
		}
	}
	enabled = true;
}

void
extrusion::set_antialias( bool aa)
{
	antialias = aa;
}

bool
extrusion::degenerate() const
{
	return count < 2 || !enabled;
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
	// TODO: Should be able to pick an extrusion, presumably. Old comment about curves:
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
		world.add_sphere( vector(pos_i), maxextent);
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
extrusion::render_end(const vector V, const double gcf, const vector current, const vector xaxis, const vector yaxis)
{
	// Use the triangle strips in "strips" to paint an end of the extrusion
	size_t npstrips = pstrips[0]; // number of triangle strips in the cross section
	size_t spoints = strips.size()/2; // total number of 2D points in all strips

	for (size_t c=0; c<npstrips; c++) {
		size_t nd = 2*pstrips[2*c+2]; // number of doubles in this strip
		size_t base = 2*pstrips[2*c+3]; // initial (x,y) = (strips[base], strips[base+1])
		std::vector<vector> tristrip(nd/2), snormals(nd/2);

		for (size_t pt=0, n=0; pt<nd; pt+=2, n++) {
			tristrip[n] = gcf*(current + xaxis*strips[base+pt] + yaxis*strips[base+pt+1]);
			snormals[n] = V;
		}

		glNormalPointer( GL_DOUBLE, 0, &snormals[0]);
		glVertexPointer(3, GL_DOUBLE, 0, &tristrip[0]);
		// nd doubles, nd/2 vertices
		glDrawArrays(GL_TRIANGLE_STRIP, 0, nd/2);

		for (size_t pt=0, n=0; pt<nd; pt+=2, n++) {
			size_t nswap;
			if (n % 2) { // if odd
				nswap = n-1;
			} else {
				if (pt == nd-2) { // total number of points is odd
					nswap = n;
				} else {
					nswap = n+1;
				}
			}
			tristrip[nswap] = gcf*(current + xaxis*strips[base+pt] + yaxis*strips[base+pt+1]);
			snormals[n] = -V;
		}

		// nd doubles, nd/2 vertices
		glDrawArrays(GL_TRIANGLE_STRIP, 0, nd/2);
	}
}

void
extrusion::extrude( const view& scene, double* spos, float* tcolor, size_t pcount)
{
	// TODO: multicolors; smooth along curve; scaling; attributes per contour

	if (pcount < 2) return;

	size_t ncontours = pcontours[0];
	if (ncontours == 0) return;
	size_t npoints = contours.size()/2; // total number of 2D points in all contours

	// 3 positions and normals per triangle, and the number of triangles = 2 times the number of points in the 2D shape,
	// times 2 for front and back of each triangle.
	// Allocate space to hold the largest contour:
	std::vector<vector> tris(12*maxcontour), normals(12*maxcontour);

	vector xaxis, yaxis; // local unit-vector axes on the 2D shape
	vector prevx, prevy; // local axes on previous bisecting plane
	vector prev;

	bool closed = vector(&spos[0]) == vector(&spos[(pcount-1)*3]);

	vector lastA; // unit vector of previous segment, initially <0,0,0>
	vector current; // point along the curve currently being processed

	// pos and color iterators
	const double* v_i = spos;
	const float* c_i = tcolor;
	bool mono = adjust_colors( scene, tcolor, pcount);

	clear_gl_error();
	if (!mono) glEnableClientState( GL_COLOR_ARRAY);
	gl_enable_client vertex_arrays( GL_VERTEX_ARRAY);
	gl_enable_client normal_arrays( GL_NORMAL_ARRAY);
	gl_enable cull_face( GL_CULL_FACE);

	for (size_t corner=0; corner < pcount; ++corner, v_i += 3, c_i += 3) {
		current = vector(&v_i[0]);

		// A is a unit vector pointing from the current location to the next location along the curve.
		// lastA is a unit vector pointing from the previous location to the current location.
		vector A = lastA;
		vector next, bisecting_plane_normal;
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

			// Calculate the normal to the plane which is the intersection of adjacent segments:
			bisecting_plane_normal = (A + lastA).norm();
			if (!bisecting_plane_normal) {  //< Exactly 180 degree bend
				bisecting_plane_normal = vector(0,0,1).cross(A);
				if (!bisecting_plane_normal)
					bisecting_plane_normal = vector(0,1,0).cross(A);
			}
		}

		if (corner == 0) {
			// Establish local xaxis,yaxis unit vectors that span the 2D surface
			yaxis = vector(0,1,0);
			xaxis = A.cross(yaxis).norm();
			if (!xaxis) {
				xaxis = A.cross( vector(0, 0, 1)).norm();
			}
			yaxis = xaxis.cross(A).norm();

			prevx = xaxis;
			prevy = yaxis;
			lastA = A;

			if (!xaxis || !yaxis || xaxis == yaxis) {
				std::ostringstream msg;
				msg << "Degenerate extrusion case! please report the following "
					"information to visualpython-users@lists.sourceforge.net: ";
				msg << "current:" << current << " next:" << next
			 		<< " A:" << A << " x:" << xaxis << " y:" << yaxis
			 		<< std::endl;
			 	VPYTHON_WARNING( msg.str());
			}

			render_end(-A, scene.gcf, current, xaxis, yaxis); // render both sides of first end

		} else {
			vector dx = -xaxis.dot(bisecting_plane_normal)*bisecting_plane_normal;
			vector dy = -yaxis.dot(bisecting_plane_normal)*bisecting_plane_normal;
			vector x = xaxis + dx;
			vector y = yaxis + dy;

			glVertexPointer(3, GL_DOUBLE, sizeof( vector), &tris[0]);
			glNormalPointer( GL_DOUBLE, sizeof(vector), &normals[0]);

			for (size_t c=0, nbase=0; c < ncontours; c++) {
				size_t nd = 2*pcontours[2*c+2]; // number of doubles in this contour
				size_t base = 2*pcontours[2*c+3]; // initial (x,y) = (contour[base], contour[base+1])
				// Triangle order is early v0, early v1, late v0, early v1, late v1, late v0; render front and back of each triangle
				for (size_t pt=0, proj=0; pt<nd; pt+=2, nbase+=4, proj+=6) {
					tris[3*nd+proj+1] = tris[proj  ] = scene.gcf*(prev    + prevx*contours[base+pt] + prevy*contours[base+pt+1]);
					// Use modulo arithmetic here because last point is the first point:
					tris[3*nd+proj  ] = tris[proj+1] = scene.gcf*(prev    + prevx*contours[base+((pt+2)%nd)] + prevy*contours[base+((pt+3)%nd)]);
					tris[3*nd+proj+2] = tris[proj+2] = scene.gcf*(current +     x*contours[base+pt] +              y*contours[base+pt+1]);
					tris[3*nd+proj+3] = tris[proj+3] = tris[proj+1];
					tris[3*nd+proj+5] = tris[proj+4] = scene.gcf*(current +     x*contours[base+((pt+2)%nd)] +     y*contours[base+((pt+3)%nd)]);
					tris[3*nd+proj+4] = tris[proj+5] = tris[proj+2];

					normals[proj  ] = (xaxis*normals2D[nbase  ] + yaxis*normals2D[nbase+1]);
					normals[proj+1] = (xaxis*normals2D[nbase+2] + yaxis*normals2D[nbase+3]);
					normals[proj+2] = normals[proj  ];
					normals[proj+3] = normals[proj+1];
					normals[proj+4] = (xaxis*normals2D[nbase+2] + yaxis*normals2D[nbase+3]);
					normals[proj+5] = normals[proj+2];

					normals[3*nd+proj+1] = -normals[proj  ];
					normals[3*nd+proj  ] = -normals[proj+1];
					normals[3*nd+proj+2] = -normals[proj+2];
					normals[3*nd+proj+3] = -normals[proj+3];
					normals[3*nd+proj+5] = -normals[proj+4];
					normals[3*nd+proj+4] = -normals[proj+5];
				}

				// nd doubles, nd/2 vertices, 2 triangles per vertex, 3 points per triangle, 2 sides, so 6*nd vertices per extrusion segment
				glDrawArrays(GL_TRIANGLES, 0, 6*nd);

			}

			prevx = x;
			prevy = y;

			// Think of the bisecting plane as a mirror, and reflect through this
			// mirror to find the new local axes for the new segment.
			xaxis = xaxis + 2*dx;
			yaxis = yaxis + 2*dy;
			lastA = A;
		}
		prev = current;
	}

	render_end(-lastA, scene.gcf, current, xaxis, yaxis); // render both sides of last end

	if (!mono) glDisableClientState( GL_COLOR_ARRAY);
	check_gl_error();
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

	extrude( scene, spos, tcolor, pcount);
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
