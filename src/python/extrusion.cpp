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
	: antialias( true), enabled( false), up(vector(0,1,0))
{
	double* k = scale.data();
	k[0] = 1.0; //scalex
	k[1] = 1.0; //scaley
	k[2] = 0.0; // twist

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

vector
smooth(const vector& a, const vector& b) { // vectors a and b need not be normalized
	vector A = a.norm();
	vector B = b.norm();
	if (A.dot(B) > 0.95) {
		return (A+B).norm();
	} else {
		return A;
	}
}

void
extrusion::set_contours( const numpy::array& _contours,  const numpy::array& _pcontours,
		const numpy::array& _strips,  const numpy::array& _pstrips  )
{
	// Polygon does not guarantee the winding order of the list of points,
	// so in primitives.py we force the winding order to be clockwise if
	// external and counterclockwise if internal (a hole).

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

	// Set up 2D normals used to build OpenGL triangles.
	// There are two per vertex, because each face of a side of an extrusion segment is a quadrilateral,
	// and each vertex appears in two adjacent triangles.
	// The indexing of normals2D follows that of looping through contour, through point. To use normals2D, you need
	// to use the same nested for-loop structure used here.
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
			Navg1 = smooth(N,Nbefore);
			Navg2 = smooth(N,Nafter);
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

void
extrusion::set_up( const vector& n_up) {
	up = n_up;
}

shared_vector&
extrusion::get_up()
{
	return up;
}

void
extrusion::set_length(size_t new_len) {
	scale.set_length(new_len);
	arrayprim_color::set_length(new_len);
}

void
extrusion::set_scale( const double_array& n_scale)
{
	std::vector<npy_intp> dims = shape( n_scale );
	if (dims.size() == 1 && !dims[0]) { // scale(); reset to size 1
		scale[make_tuple(all(), slice(0,2))] = 1.0;
		return;
	}
	if (dims.size() == 1 && dims[0] == 1) { // scale([2])
		scale[make_tuple(all(), 0)] = n_scale;
		scale[make_tuple(all(), 1)] = n_scale;
		return;
	}
	if (dims.size() == 2 && dims[0] == 1 && dims[1] == 2) { // scale([(1,2)])
		scale[make_tuple(all(), slice(0,2))] = n_scale;
		return;
	}
	if (dims.size() == 1 && dims[0] == 2) { // scale(1,2)
		scale[make_tuple(all(), slice(0,2))] = n_scale;
		return;
	}
	if (dims.size() != 2) {
		throw std::invalid_argument( "scale must be an Nx2 array");
	}
	if (dims[1] == 2) {
		set_length( dims[0] );
		scale[make_tuple(all(), slice(0,2))] = n_scale;
		return;
	}
	else {
		throw std::invalid_argument( "scale must be an Nx2 array");
	}
}

void
extrusion::set_scale_d( const double n_scale)
{
	scale[make_tuple(all(), 0)] = n_scale;
	scale[make_tuple(all(), 1)] = n_scale;
}

boost::python::object extrusion::get_scale() {
	return scale[make_tuple(all(), slice(0,2))];
}

void
extrusion::set_twist( const double_array& n_twist)
{
	std::vector<npy_intp> dims = shape( n_twist );
	if (dims.size() == 1 && !dims[0]) { // twist()
		scale[make_tuple(all(), 2)] = 0.0;
		return;
	}
	if (dims.size() == 1 && dims[0] == 1) { // twist(t)
		scale[make_tuple(all(), 2)] = n_twist;
		return;
	}
	if (dims.size() == 1) { // twist(1,2,3)
		set_length( dims[0] );
		scale[make_tuple(all(), 2)] = n_twist;
		return;
	}
	if (dims.size() != 2) {
		throw std::invalid_argument( "twist must be an Nx1 array");
	}
	if (dims[1] == 1) {
		set_length( dims[0] );
		scale[make_tuple(all(), 2)] = n_twist;
		return;
	}
	else {
		throw std::invalid_argument( "twist must be an Nx1 array");
	}
}

void
extrusion::set_twist_d( const double n_twist)
{
	scale[make_tuple(all(), 2)] = n_twist;
}

boost::python::object extrusion::get_twist() {
	return scale[make_tuple(all(), 2)];
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

vector
extrusion::get_center() const
{
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

void
extrusion::render_end(const vector V, const double gcf, const vector current,
		const vector xaxis, const vector yaxis, const float* endcolor)
{
	// Use the triangle strips in "strips" to paint an end of the extrusion
	size_t npstrips = pstrips[0]; // number of triangle strips in the cross section
	size_t spoints = strips.size()/2; // total number of 2D points in all strips

	glEnableClientState( GL_COLOR_ARRAY);

	for (size_t c=0; c<npstrips; c++) {
		size_t nd = 2*pstrips[2*c+2]; // number of doubles in this strip
		size_t base = 2*pstrips[2*c+3]; // initial (x,y) = (strips[base], strips[base+1])
		std::vector<vector> tristrip(nd/2), snormals(nd/2);
		std::vector<float> endcolors(3*nd/2);

		for (size_t pt=0, n=0; pt<nd; pt+=2, n++) {
			tristrip[n] = gcf*(current + xaxis*strips[base+pt] + yaxis*strips[base+pt+1]);
			snormals[n] = V;
			endcolors[3*n  ] = endcolor[0];
			endcolors[3*n+1] = endcolor[1];
			endcolors[3*n+2] = endcolor[2];
		}

		glNormalPointer( GL_DOUBLE, 0, &snormals[0]);
		glVertexPointer(3, GL_DOUBLE, 0, &tristrip[0]);
		glColorPointer(3, GL_FLOAT, 0, &endcolors[0]);
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
	glDisableClientState( GL_COLOR_ARRAY);
}

void
extrusion::extrude( const view& scene, double* spos, float* tcolor, double* tscale, size_t pcount)
{
	// TODO: scale, should be an Nx2 array
	// TODO: twist should be an Nx1 array, to permit twisted extrusions.
	// TODO: start and end attributes (slice numbering convention); allows for non-square ends
	// TODO: attributes per contour?
	// TODO: library of simple shapes (star provided by Polygon.Shapes)

	// The basic architecture of the extrusion object:
	// Use the Polygon module to create by constructive geometry a 2D surface in the form of a
	// set of contours, each a sequence of 2D points. In primitives.py these contours are forced to
	// be ordered CW if external and CCW if internal (holes). In set_contours, 2D normals to these
	// contours are calculated, with smoothing around the contour. The 2D surface, including the
	// computed normals, is extruded as a cross section along a curve defined by the pos attribute,
	// which like other array objects (curve, points, faces, convex) is represented by a numpy array.

	// For efficiency, orthogonal unit vectors (xaxis,yaxis) in the 2D surface are defined so that a
	// contour point (a,b) relative to the curve is a vector in the plane a*xaxis+b*yaxis.
	// At a joint between adjacent extrusion segments, from (xaxis,yaxis) that are perpendicular
	// to the curve, we derive new unit vectors (x,y) in the plane of 2D surface such that y is
	// parallel to the "axle" of rotation of the joint, the locus of points for which no rotation
	// occurs. We determine coefficients such that a*xaxis+b*yaxis = aa*x+bb*y, so that
	// aa = c11*a + c12*b and bb = c21*a + c22*b. We then create a non-unit vector xrot in the
	// plane of the joint, perpendicular to y, so that positions in the joint corresponding to (a,b)
	// are given by aa*xrot + bb*y. These joint positions are connected to the previous joint positions
	// to form one segment of the extrusion. (We don't save all the previous positions; rather we simply
	// save the previous values of (x,y) from which the previous positions can easily be calculated.)

	// The normals to the sides of the extrusion are simply computed from the (nx,ny) normals,
	// computed in set_contours, as nx*xaxis+ny*yaxis. By look-ahead, using what (xaxis,yaxis) will
	// be in the next segment, the normals at the end of one segment are smoothed to normals at the
	// start of the next segment. At the start of the next segment we look behind to smooth the normals.

	// Using the scale array of (scalex,scaley) values, the actual position of a point (a,b) in the
	// 2D surface is given by (scalex*a, scaley*b). Note that the precalculated normal to a vector (a,b)
	// on the 2D surface is in the direction (-b, a), as can be seen from the dot product being zero.
	// This means that when nonuniform scaling is in effect (scalex != scaley), the direction of a normal
	// changes to be in the direction (-scaley*b, scalex*a).

	// The scale array is an array of (scalex, scaley, twist), where twist is the angle (in radians) of
	// the CCW rotation from the previous point.

	// extrusion.shape=Polygon(...) not only sends contour information to set_contours but also
	// sends triangle strips used to render the front and back surfaces of the extrusion.

	if (pcount < 2) return;

	size_t ncontours = pcontours[0];
	if (ncontours == 0) return;
	size_t npoints = contours.size()/2; // total number of 2D points in all contours

	// 3 positions and normals per triangle, and the number of triangles = 2 times the number of points in the 2D shape,
	// times 2 for front and back of each triangle. Allocate space to hold the largest contour:
	size_t maxtriangles = 12*maxcontour;
	std::vector<vector> tris(maxtriangles), normals(maxtriangles);
	std::vector<float> tcolors(3*maxtriangles);

	vector xaxis, yaxis; // local unit-vector axes on the 2D shape
	vector prevxaxis, prevyaxis; // local unit-vector axes on the 2D shape on preceding segment
	vector nextxaxis, nextyaxis; // local unit-vector axes on the 2D shape on following segment
	vector prevx, prevy; // local axes on previous plane perpendicular to curve
	vector prevxrot; // local axis in the plane of the joint
	double prevc11, prevc12, prevc21, prevc22; // previous rotation coefficients on the 2D surface

	bool closed = vector(&spos[0]) == vector(&spos[(pcount-1)*3]);

	// pos and color iterators
	const double* v_i = spos;
	const float* c_i = tcolor;
	const double* s_i = tscale;
	const float* current_color = tcolor;
	const float* prev_color = tcolor;
	bool mono = adjust_colors( scene, tcolor, pcount);

	vector A; // points from previous point to current point
	vector lastA; // unit vector of previous segment
	vector prev; // previous location
	vector current; // point along the curve currently being processed

	// Skip past duplicate points at start of extrusion
	size_t corner;
	for (corner=0; corner < pcount-1; ++corner, v_i += 3, c_i += 3, s_i += 3) {
		lastA = vector(&v_i[0]) - vector(&spos[0]);
		if (!lastA) {
			continue;
		} else {
			break;
		}
	}
	if (!lastA) return; // all the points of this extrusion are at the same location; abort
	A = lastA = lastA.norm();
	prev = vector(&spos[0]);
	prev_color = c_i;

	// On the 2D surface position (a,b) is at a*xaxis+b*yaxis, where
	//    xaxis=(1,0) and yaxis=(0,1) on that 2D surface.
	yaxis = up;
	xaxis = A.cross(yaxis).norm();
	if (!xaxis) xaxis = A.cross( vector(0, 0, 1)).norm();
	if (!xaxis) xaxis = A.cross( vector(1, 0, 0)).norm();
	yaxis = xaxis.cross(A).norm();

	prevxrot = prevx = prevxaxis = xaxis;
	prevy = prevyaxis = yaxis;
	prevc11 = tscale[0];
	prevc12 = prevc21 = 0;
	prevc22 = tscale[1];

	if (!xaxis || !yaxis || xaxis == yaxis) {
		std::ostringstream msg;
		msg << "Degenerate extrusion case! please report the following "
			"information to visualpython-users@lists.sourceforge.net: ";
		msg << "current:" << current
	 		<< " A:" << A << " x:" << xaxis << " y:" << yaxis
	 		<< std::endl;
	 	VPYTHON_WARNING( msg.str());
	}

	clear_gl_error();
	gl_enable_client vertex_arrays( GL_VERTEX_ARRAY);
	gl_enable_client normal_arrays( GL_NORMAL_ARRAY);
	gl_enable cull_face( GL_CULL_FACE);

	render_end(-A, scene.gcf, vector(&spos[0]), tscale[0]*xaxis, tscale[1]*yaxis, &tcolor[0]); // render both sides of first end
	if (!mono) glEnableClientState( GL_COLOR_ARRAY);

	for (; corner < pcount; ++corner, v_i += 3, c_i += 3, s_i += 3) {
		current_color = c_i;
		current = vector(&v_i[0]);

		// A is a unit vector pointing from the current location to the next location along the curve.
		// lastA is a unit vector pointing from the previous location to the current location.

		// Skip over duplicate points (but retain correct color information)
		if (corner == pcount-1) {
			A = lastA;
		} else {
			const double* tv_i = v_i;
			for (size_t tcorner=corner; tcorner < pcount-1; ++tcorner, tv_i += 3) {
				A = (vector( &tv_i[3]) - current).norm();
				if (!A) {
					v_i += 3;
					c_i += 3;
					corner++;
					continue;
				} else {
					break;
				}
			}
			if (!A) {
				A = lastA;
			}
		}

		// Calculate the normal to the plane which is the intersection of adjacent segments:
		vector bisecting_plane_normal;
		bisecting_plane_normal = (A + lastA).norm();
		if (!bisecting_plane_normal) {  //< Exactly 180 degree bend
			bisecting_plane_normal = vector(0,0,1).cross(A);
			if (!bisecting_plane_normal)
				bisecting_plane_normal = vector(0,1,0).cross(A);
		}

		// A point (a,b) in the 2D surface is located in 3D space at current+a*xaxis+b*yaxis.
		// Re-express points in the 2D surface in terms of (x,y) axes, where y is parallel
		// to the "axle" of rotation of the joint, along which points don't rotate.
		vector y = (lastA.cross(A)).norm(); // parallel to the axle of the joint
		vector x;
		if (!y) {
			y = yaxis; // the joint is perpendicular to the extrusion segment; no change of direction
			x = xaxis;
		} else {
			x = lastA.cross(y);
		}

		// If twist is positive, rotate xaxis and yaxis CCW
		if (s_i[2]) {
			double cost = cos(s_i[2]);
			double sint = sin(s_i[2]);
			vector xtemp = cost*xaxis + sint*yaxis;
			yaxis = -sint*xaxis + cost*yaxis;
			xaxis = xtemp;
		}

		// Calculate rotation coefficients from xaxis,yaxis to x,y in the 2D plane
		// a*xaxis + b*yaxis = aa*x + bb*y; dot with x and y to obtain this:
		// aa = (x dot xaxis)*a + (x dot yaxis)*b = c11*a + c12*b
		// bb = (y dot xaxis)*a + (y dot yaxis)*b = c21*a + c22*b
		double c11 = x.dot(xaxis)*s_i[0];
		double c12 = x.dot(yaxis)*s_i[1];
		double c21 = y.dot(xaxis)*s_i[0];
		double c22 = y.dot(yaxis)*s_i[1];

		// Points not on the axle must be rotated around the axle.
		double axlecos = lastA.dot(bisecting_plane_normal); // angle of rotation about axle
		vector xrot;
		if (!axlecos) {
			xrot = x;
		} else {
			xrot = bisecting_plane_normal.cross(y)/axlecos; // make xrot a non-unit vector, in the plane of the joint, to correct for rotation about axle
		}

		double xcos = xaxis.dot(bisecting_plane_normal);
		double ycos = yaxis.dot(bisecting_plane_normal);
		vector dx = -xcos*bisecting_plane_normal;
		vector dy = -ycos*bisecting_plane_normal;

		if (corner == pcount-1) { // processing final segment
			nextxaxis = xaxis;
			nextyaxis = yaxis;
		} else {
			// Think of the bisecting plane as a mirror, and reflect through this
			// mirror to find the new local axes for the next segment.
			nextxaxis = xaxis + 2*dx;
			nextyaxis = yaxis + 2*dy;
		}

		glVertexPointer(3, GL_DOUBLE, 0, &tris[0]);
		glNormalPointer( GL_DOUBLE, 0, &normals[0]);
		glColorPointer(3, GL_FLOAT, 0, &tcolors[0]);

		float r_old=prev_color[0], g_old=prev_color[1], b_old=prev_color[2]; // color at previous location along the curve
		float r_new=current_color[0], g_new=current_color[1], b_new=current_color[2];    // color at current location along the curve
		double v0x, v0y, v1x, v1y, prevv0x, prevv0y, prevv1x, prevv1y;
		// The following nested for loops is (necessarily) the same as that used to build the normals2D array.
		for (size_t c=0, nbase=0; c < ncontours; c++) {
			size_t nd = 2*pcontours[2*c+2]; // number of doubles in this contour
			size_t base = 2*pcontours[2*c+3]; // initial (x,y) = (contour[base], contour[base+1])
			// Triangle order is
			//    previous v0, current v1, current v0, previous v1, current v1, previous v0.
			// Render front and back of each triangle.
			for (size_t pt=0, i=0; pt<nd; pt+=2, i+=6, nbase+=4) {
				// Use modulo arithmetic here because last point is the first point, going around the sides of the extrusion
				prevv0x = prevc11*contours[base+pt  ] + prevc12*contours[base+pt+1];
				prevv0y = prevc21*contours[base+pt  ] + prevc22*contours[base+pt+1];
				prevv1x = prevc11*contours[base+((pt+2)%nd)] + prevc12*contours[base+((pt+3)%nd)];
				prevv1y = prevc21*contours[base+((pt+2)%nd)] + prevc22*contours[base+((pt+3)%nd)];
				v0x = c11*contours[base+pt  ] + c12*contours[base+pt+1];
				v0y = c21*contours[base+pt  ] + c22*contours[base+pt+1];
				v1x = c11*contours[base+((pt+2)%nd)] + c12*contours[base+((pt+3)%nd)];
				v1y = c21*contours[base+((pt+2)%nd)] + c22*contours[base+((pt+3)%nd)];
				tris[3*nd+i+1] = tris[i  ] = scene.gcf*(prev    + prevxrot*prevv0x + prevy*prevv0y);
				tris[3*nd+i  ] = tris[i+1] = scene.gcf*(prev    + prevxrot*prevv1x + prevy*prevv1y);
				tris[3*nd+i+2] = tris[i+2] = scene.gcf*(current +     xrot*v0x     + y*v0y);
				tris[3*nd+i+3] = tris[i+3] = tris[i+1];
				tris[3*nd+i+5] = tris[i+4] = scene.gcf*(current +     xrot*v1x     + y*v1y);
				tris[3*nd+i+4] = tris[i+5] = tris[i+2];

				tcolors[3*i  ] = tcolors[3*(i+1)  ] = tcolors[3*(i+3)  ] = r_old;
				tcolors[3*i+1] = tcolors[3*(i+1)+1] = tcolors[3*(i+3)+1] = g_old;
				tcolors[3*i+2] = tcolors[3*(i+1)+2] = tcolors[3*(i+3)+2] = b_old;

				tcolors[3*(i+2)  ] = tcolors[3*(i+4)  ] = tcolors[3*(i+5)  ] = r_new;
				tcolors[3*(i+2)+1] = tcolors[3*(i+4)+1] = tcolors[3*(i+5)+1] = g_new;
				tcolors[3*(i+2)+2] = tcolors[3*(i+4)+2] = tcolors[3*(i+5)+2] = b_new;

				tcolors[3*(3*nd+i)  ] = tcolors[3*(3*nd+i+1)  ] = tcolors[3*(3*nd+i+3)  ] = r_old;
				tcolors[3*(3*nd+i)+1] = tcolors[3*(3*nd+i+1)+1] = tcolors[3*(3*nd+i+3)+1] = g_old;
				tcolors[3*(3*nd+i)+2] = tcolors[3*(3*nd+i+1)+2] = tcolors[3*(3*nd+i+3)+2] = b_old;

				tcolors[3*(3*nd+i+2)  ] = tcolors[3*(3*nd+i+4)  ] = tcolors[3*(3*nd+i+5)  ] = r_new;
				tcolors[3*(3*nd+i+2)+1] = tcolors[3*(3*nd+i+4)+1] = tcolors[3*(3*nd+i+5)+1] = g_new;
				tcolors[3*(3*nd+i+2)+2] = tcolors[3*(3*nd+i+4)+2] = tcolors[3*(3*nd+i+5)+2] = b_new;

				normals[i  ] = smooth(     s_i[1]*xaxis*normals2D[nbase  ] +      s_i[0]*yaxis*normals2D[nbase+1],
						              s_i[-2]*prevxaxis*normals2D[nbase  ] + s_i[-3]*prevyaxis*normals2D[nbase+1]);

				normals[i+1] = smooth(     s_i[1]*xaxis*normals2D[nbase+2] +      s_i[0]*yaxis*normals2D[nbase+3],
						              s_i[-2]*prevxaxis*normals2D[nbase+2] + s_i[-3]*prevyaxis*normals2D[nbase+3]);

				normals[i+2] = smooth(     s_i[1]*xaxis*normals2D[nbase  ] +      s_i[0]*yaxis*normals2D[nbase+1],
						              s_i[-2]*nextxaxis*normals2D[nbase  ] + s_i[-3]*nextyaxis*normals2D[nbase+1]);

				normals[i+3] = normals[i+1]; // i+1 and i+3 are the same location

				normals[i+4] = smooth(     s_i[1]*xaxis*normals2D[nbase+2] +      s_i[0]*yaxis*normals2D[nbase+3],
						              s_i[-2]*nextxaxis*normals2D[nbase+2] + s_i[-3]*nextyaxis*normals2D[nbase+3]);

				normals[i+5] = normals[i+2]; // i+2 and i+5 are the same location

				// Normals for other sides of triangles:
				normals[3*nd+i+1] = -normals[i  ];
				normals[3*nd+i  ] = -normals[i+1];
				normals[3*nd+i+2] = -normals[i+2];
				normals[3*nd+i+3] = -normals[i+3];
				normals[3*nd+i+5] = -normals[i+4];
				normals[3*nd+i+4] = -normals[i+5];
			}

			// nd doubles, nd/2 vertices, 2 triangles per vertex,
			//    3 points per triangle, 2 sides, so 6*nd vertices per extrusion segment
			glDrawArrays(GL_TRIANGLES, 0, 6*nd);

		}

		prevx = x;
		prevy = y;
		prevxrot = xrot;
		prevc11 = c11;
		prevc12 = c12;
		prevc21 = c21;
		prevc22 = c22;
		prevxaxis = xaxis;
		prevyaxis = yaxis;
		xaxis = nextxaxis;
		yaxis = nextyaxis;
		lastA = A;
		prev = current;
		prev_color = c_i;
	}

	// Render both sides of last end
	render_end(-lastA, scene.gcf, vector(&spos[3*(pcount-1)]),
			tscale[3*(pcount-1)]*xaxis, tscale[3*(pcount-1)+1]*yaxis, &tcolor[3*(pcount-1)]);

	if (!mono) glDisableClientState( GL_COLOR_ARRAY);
	check_gl_error();
}

void
extrusion::gl_render( const view& scene)
{
	if (degenerate())
		return;

	const size_t true_size = count;
	const int LINE_LENGTH = 1000; // The maximum number of points to display.
	// Data storage for the position and color data (plus room for 3 extra points)
	double spos[3*(LINE_LENGTH+3)];
	double tscale[3*(LINE_LENGTH+3)];
	float tcolor[3*(LINE_LENGTH+3)]; // opacity not yet implemented for extrusions
	float fstep = (float)(count-1)/(float)(LINE_LENGTH-1);
	if (fstep < 1.0F) fstep = 1.0F;
	size_t iptr=0, iptr3, pcount=0;

	const double* p_i = pos.data();
	const double* c_i = color.data();
	const double* s_i = scale.data();

	// Choose which points to display
	for (float fptr=0.0; iptr < count && pcount < LINE_LENGTH; fptr += fstep, iptr = (int) (fptr+.5), ++pcount) {
		iptr3 = 3*iptr;
		spos[3*pcount  ] = p_i[iptr3];
		spos[3*pcount+1] = p_i[iptr3+1];
		spos[3*pcount+2] = p_i[iptr3+2];
		tcolor[3*pcount  ] = c_i[iptr3];
		tcolor[3*pcount+1] = c_i[iptr3+1];
		tcolor[3*pcount+2] = c_i[iptr3+2];
		tscale[3*pcount  ] = s_i[iptr3];
		tscale[3*pcount+1] = s_i[iptr3+1];
		tscale[3*pcount+2] = s_i[iptr3+2];
	}

	extrude( scene, spos, tcolor, tscale, pcount);
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
