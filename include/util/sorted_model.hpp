#ifndef VPYTHON_UTIL_SORTED_MODEL_HPP
#define VPYTHON_UTIL_SORTED_MODEL_HPP

#include "util/vector.hpp"
#include <algorithm>

/** A helper class for texture coordinates. */
struct tcoord
{
	float s;
	float t;
	inline tcoord( float s_ = 0, float t_ = 0) : s(s_), t(t_) {}
	inline explicit tcoord( const vector& v)
	 : s(v.x), t(v.y) {}
	inline void gl_render() const
	{ glTexCoord2f( s, t); }
};

/** A single triangular face whose corners are ordered counterclockwise in the
	forward facing direction.  The normals, corners, and center are all constant.
	Arrays of these objects are not layout-compatable with any OpenGL function
	calls.
*/
struct triangle
{
	vector corner[3]; ///< The vertex coordinates.
	vector normal;    ///< The bodies flat normal vector.
	vector center;    ///< The center of the body, used for depth sorting.
	triangle() {}
	/** Construct a new triangle with these corners.  normal and center are
		computed automatically.
	*/
	triangle( const vector& v1, const vector& v2, const vector& v3);
	/** Render the triangle to OpenGL. */
	void gl_render() const;
};

inline 
triangle::triangle( const vector& v1, const vector& v2, const vector& v3)
{
	corner[0] = v1;
	corner[1] = v2;
	corner[2] = v3;
	center = (v1 + v2 + v3) / 3.0;
	normal = -(corner[0] - corner[1]).cross( corner[2] - corner[1]).norm();
}

inline void
triangle::gl_render() const
{
	normal.gl_normal();
	corner[0].gl_render();
	corner[1].gl_render();
	corner[2].gl_render();
}

/** A single 4-sided face whose corners are ordered counterclockwise in the
forward-facing direction.  All of its geometry is constant.
*/
struct quad
{
	vector corner[4]; ///< The vertexes of this quad.  They must be coplanar.
	vector normal; ///< The flat-shaded normal vector for this quad.
	vector center; ///< The center of the quad, used for depth sorting.
	quad() {}
	/** Construct a new quad with these corners.  normal and center are
		computed automatically.
	*/
	quad( const vector& v1, const vector& v2, const vector& v3, const vector& v4);
	/** Render the triangle to OpenGL. */
	void gl_render() const;
};

inline
quad::quad( const vector& v1, const vector& v2, const vector& v3, const vector& v4)
{
	corner[0] = v1;
	corner[1] = v2;
	corner[2] = v3;
	corner[3] = v4;
	center = (v1 + v2 + v3 + v4) * 0.25;
	normal = -(corner[0] - corner[1]).cross( corner[2] - corner[1]).norm();
}

inline void
quad::gl_render() const
{
	normal.gl_normal();
	corner[0].gl_render();
	corner[1].gl_render();
	corner[2].gl_render();
	corner[3].gl_render();
}

/** A quadrilateral object that also uses texture coordinates. */
struct tquad : public quad
{
	tcoord tex[4]; ///< The texture coordinates.
	tquad() {}
	tquad( const vector& v1, const tcoord& t1, 
		const vector& v2, const tcoord& t2, 
		const vector& v3, const tcoord& t3,
		const vector& v4, const tcoord& t4);

	void gl_render() const;
};

inline 
tquad::tquad( const vector& v1, const tcoord& t1, 
		const vector& v2, const tcoord& t2, 
		const vector& v3, const tcoord& t3,
		const vector& v4, const tcoord& t4)
	: quad( v1, v2, v3, v4)
{
	tex[0] = t1;
	tex[1] = t2;
	tex[2] = t3;
	tex[3] = t4;
}

inline void
tquad::gl_render() const
{
	normal.gl_normal();
	tex[0].gl_render();
	corner[0].gl_render();
	tex[1].gl_render();
	corner[1].gl_render();
	tex[2].gl_render();
	corner[2].gl_render();
	tex[3].gl_render();
	corner[3].gl_render();
}

/** A depth-sorting criteria for the sorting algorithms in the STL.  It is a
	model of BinaryPredicate.
*/
class face_z_comparator
{
 private:
	vector forward; ///< The axis along which to sort.
 
 public:
	/** Construct a new comparison object.  
 		@param axis The depth axis along which to compare.
	*/
	face_z_comparator( const vector& axis) throw()
		: forward(axis)
	{}
	
	/** Test the sorting criterion.  This varient is for pointer types.
		@param lhs A face to be compared.
		@param rhs Another face to be compared
		@return true if the lhs is farther away than rhs, false otherwise. 
	*/
	template <typename Face>
	inline bool
	operator()( const Face* lhs, const Face* rhs) const throw()
	{ return forward.dot( lhs->center) > forward.dot( rhs->center); }
	
	/** Test the sorting criterion.  This varient is for reference types.
		@param lhs A face to be compared.
		@param rhs Another face to be compared
		@return true if the lhs is farther away than rhs, false otherwise. 
	*/
	template <typename Face>
	inline bool
	operator()( const Face& lhs, const Face& rhs) const throw()
	{ return forward.dot( lhs.center) > forward.dot( rhs.center); }
};

/** A depth-sortable model for a static geometry.  Resorting is pretty damn fast
	when using any of the STL algorithms, but the variability in all of the
	algorithms except stable_sort is just too unacceptable.  We definately hit 
	the worst-case sort frequently.  See also test/model_zsort_bench.cpp.
	Face is any class with a center data member.  nfaces is the number of faces
	in the body.
*/
template <typename Face, size_t nfaces>
struct z_sorted_model
{
	Face faces[nfaces]; ///< The geometry data.
	/** Sort such that the faces are ordered from farthest away to nearest. 
		@param n_forward The axis along which to sort.  It should point from
			the camera towards the body.
	*/
	void sort( const vector& n_forward);
	
	/** Renders all of this object's polygons, in order.  This member function
		performs the rendering in immediate mode, which may be too slow for
		large models.
	*/
	void gl_render() const;
};

template <typename Face, size_t nfaces>
void
z_sorted_model<Face, nfaces>::sort( const vector& n_forward)
{
	face_z_comparator cmp( n_forward);
	std::stable_sort( faces, faces + nfaces, cmp);
}

template <typename Face, size_t nfaces>
void
z_sorted_model<Face, nfaces>::gl_render() const
{
	for (size_t i = 0; i < nfaces; ++i) {
		faces[i].gl_render();
	}
}

#endif // !defined VPYTHON_UTIL_SORTED_MODEL_HPP
