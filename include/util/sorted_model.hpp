#ifndef VPYTHON_UTIL_SORTED_MODEL_HPP
#define VPYTHON_UTIL_SORTED_MODEL_HPP

#include <algorithm>
#include "util/vector.hpp"

struct tcoord
{
	float s;
	float t;
	inline tcoord() : s(0), t(0) {}
	inline explicit tcoord( const vector& v)
	 : s(v.x), t(v.y) {}
	inline void gl_render() const
	{ glTexCoord2f( s, t); }
};

// A single triangular face whose corners are ordered counterclockwise in the
// forward facing direction.  The normals, corners, and center are all constant.
struct triangle
{
	vector corner[3];
	vector normal;
	vector center;
	triangle() {}
	triangle( const vector& v1, const vector& v2, const vector& v3);
	// Render the object.
	void gl_render() const;
	bool depth_cmp( const triangle& other, const vector& forward) const;
	inline size_t n_verticies() const
	{ return 3; }
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

inline bool
triangle::depth_cmp( const triangle& other, const vector& forward) const
{
	return forward.dot( center) > forward.dot( other.center);
}

// A single 4-sided face whose corners are ordered counterclockwise in the
// forward-facing direction.  All of its geometry is constant.
struct quad
{
	vector corner[4];
	vector normal;
	vector center;
	quad() {}
	quad( const vector& v1, const vector& v2, const vector& v3, const vector& v4);
	void gl_render() const;
	bool depth_cmp( const quad& other, const vector& forward) const;
	inline size_t n_verticies() const
	{ return 4; }
};

// NOTE: v1-v4 must be coplanar or undefined behavior is the result!
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

// This buggar needs to be redesigned as it hits several corner cases.
inline bool
quad::depth_cmp( const quad& other, const vector& forward) const
{
	return true;
}

// A quadrilateral object that also uses texture coordinates.
struct tquad : public quad
{
	tcoord texture[4];
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
	texture[0] = t1;
	texture[1] = t2;
	texture[2] = t3;
	texture[3] = t4;
}

inline void
tquad::gl_render() const
{
	normal.gl_render();
	texture[0].gl_render();
	corner[0].gl_render();
	texture[1].gl_render();
	corner[1].gl_render();
	texture[2].gl_render();
	corner[2].gl_render();
	texture[3].gl_render();
	corner[3].gl_render();
}

// A depth-sorting criteria for the sorting algorithms in the STL.
class face_z_comparator
{
 private:
	vector forward;
 
 public:
	face_z_comparator( const vector& f) throw()
		: forward(f)
	{}
	
	template <typename Face>
	inline bool
	operator()( const Face* lhs, const Face* rhs) const throw()
	{ return forward.dot( lhs->center) > forward.dot( rhs->center); }
	
	template <typename Face>
	inline bool
	operator()( const Face& lhs, const Face& rhs) const throw()
	{ return forward.dot( lhs.center) > forward.dot( rhs.center); }
};

// A z-sortable model for a static geometry.  Resorting is pretty damn fast when
// using any of the sorting algorithms, but the variability in all of the
// algorithms but stable_sort is just too unacceptable.  We definately hit the
// worst-case sort frequently.  See also test/model_zsort_bench.cpp.
// Face is any class with a center data member.
template <typename Face, size_t nfaces>
struct z_sorted_model
{
	Face faces[nfaces];
	// Sort such that the faces are ordered from farthest away to nearest.
	void sort( const vector& n_forward);
	// Renders all of the object's children in order.
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
