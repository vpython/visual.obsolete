
#include "util/tmatrix.hpp"
#include <cmath>

#include <GL/gl.h>

void 
frustum( tmatrix& T, tmatrix& I, double l, double r, double b, double t, double n, double f ) throw()
{
	T.x_column(2*n/(r-l),0,0);
	T.y_column(0,2*n/(t-b),0);
	T.z_column((r+l)/(r-l),(t+b)/(t-b),(f+n)/(n-f));
	T.w_column(0,0,2*f*n/(n-f));
	T.w_row(0,0,-1,0);

	I.x_column((r-l)/(2*n), 0, 0);
	I.y_column(0,(t-b)/(2*n),0);
	I.z_column(0,0,0);
	I.w_column((r+l)/(2*n),(t+b)/(2*n),-1);
	I.w_row(0,0,(n-f)/(2*f*n),(f+n)/(2*f*n));
}

const tmatrix& 
tmatrix::identity() throw()
{ 
	static const tmatrix t = tmatrix().ident();
	return t;
}

tmatrix 
rotation( double angle, const vector& axis)
{
	double c = std::cos(angle);
	double s = std::sin(angle);
	double ic = 1.0-c;
	double icxx = ic * axis.x * axis.x;
	double icxy = ic * axis.x * axis.y;
	double icxz = ic * axis.x * axis.z;
	double icyy = ic * axis.y * axis.y;
	double icyz = ic * axis.y * axis.z;
	double iczz = ic * axis.z * axis.z;

	tmatrix ret;
	ret.x_column( icxx +        c, icxy + axis.z*s, icxz - axis.y*s );
	ret.y_column( icxy - axis.z*s, icyy +     c   , icyz + axis.x*s );
	ret.z_column( icxz + axis.y*s, icyz - axis.x*s, iczz +        c );
	ret.w_column();
	ret.w_row();
	return ret;
}

tmatrix
rotation( double angle, const vector& axis, const vector& origin)
{
	tmatrix ret = rotation(angle, axis.norm());
	ret.w_column( origin - ret * origin);
	return ret;
}

#if 0
const tmatrix&
tmatrix::operator*=( const tmatrix& other)
{
	for (size_t row = 0; row < 4; ++row) {
		M[0][row] = 
		
	}
	return *this;
}
#endif

tmatrix
tmatrix::operator*( const tmatrix& o) const
{
	tmatrix ret;
	for (size_t col = 0; col < 4; ++col) {
		ret.M[col][0] = M[0][0]*o.M[col][0] + M[1][0]*o.M[col][1] 
			          + M[2][0]*o.M[col][2] + M[3][0]*o.M[col][3];
		ret.M[col][1] = M[0][1]*o.M[col][0] + M[1][1]*o.M[col][1] 
			          + M[2][1]*o.M[col][2] + M[3][1]*o.M[col][3];
		ret.M[col][2] = M[0][2]*o.M[col][0] + M[1][2]*o.M[col][1] 
			          + M[2][2]*o.M[col][2] + M[3][2]*o.M[col][3];
		ret.M[col][3] = M[0][3]*o.M[col][0] + M[1][3]*o.M[col][1] 
			          + M[2][3]*o.M[col][2] + M[3][3]*o.M[col][3];
	}
	return ret;	
}

// TODO: Reverse address me
void 
tmatrix::invert_ortho(const tmatrix& A) throw()
{
	// Precondition: w = Mv = Rv + t  (R orthogonal)
	// Therefore: (M^-1)w = v = (R^T)Rv 
	//                        = (R^T)(Rv+t - t) 
	//                        = (R^T)(w-t) 
	//                        = (R^T)w - (R^T)t

	x_column(A(0, 0), A(0, 1), A(0,2));  // row 0
	y_column(A(1,0), A(1,1), A(1,2));  // row 1
	z_column(A(2,0), A(2,1), A(2,2));  // row 2
	w_column(-A(0,0)*A(0,3) - A(1,0)*A(1,3) - A(2,0)*A(2,3)
			 , -A(0,1)*A(0,3) - A(1,1)*A(1,3) - A(2,1)*A(2,3)
			 , -A(0,2)*A(0,3) - A(1,2)*A(1,3) - A(2,2)*A(2,3)
			 );
	w_row();
}

vector 
tmatrix::times_inv( const vector& v, double w) const throw()
{
	double x = v.x - M[3][0]*w;
	double y = v.y - M[3][1]*w;
	double z = v.z - M[3][2]*w;
	return vector(
		M[0][0]*x + M[0][1]*y + M[0][2]*z,
		M[1][0]*x + M[1][1]*y + M[1][2]*z,
		M[2][0]*x + M[2][1]*y + M[2][2]*z
		);
}

vector 
tmatrix::times_v( const vector& v) const throw()
{
	return vector(
		M[0][0]*v.x + M[1][0]*v.y + M[2][0]*v.z,
		M[0][1]*v.x + M[1][1]*v.y + M[2][1]*v.z,
		M[0][2]*v.x + M[1][2]*v.y + M[2][2]*v.z
		);
}

vector 
tmatrix::operator*( const vector& v) const throw()
{
	return vector(
		M[0][0]*v.x + M[1][0]*v.y + M[2][0]*v.z + M[3][0],
		M[0][1]*v.x + M[1][1]*v.y + M[2][1]*v.z + M[3][1],
		M[0][2]*v.x + M[1][2]*v.y + M[2][2]*v.z + M[3][2]
		);
}

void
tmatrix::project( const vector& v, vertex& o) const throw()
{
	o.x = M[0][0]*v.x + M[1][0]*v.y + M[2][0]*v.z + M[3][0];
	o.y = M[0][1]*v.x + M[1][1]*v.y + M[2][1]*v.z + M[3][1];
	o.z = M[0][2]*v.x + M[1][2]*v.y + M[2][2]*v.z + M[3][2];
	o.w = M[0][3]*v.x + M[1][3]*v.y + M[2][3]*v.z + M[3][3];
}

void
tmatrix::scale( const vector& v, const double w)
{
	M[0][0] *= v.x;
	M[0][1] *= v.x;
	M[0][2] *= v.x;
	M[0][3] *= v.x;
	
	M[1][0] *= v.y;
	M[1][1] *= v.y;
	M[1][2] *= v.y;
	M[1][3] *= v.y;
	
	M[2][0] *= v.z;
	M[2][1] *= v.z;
	M[2][2] *= v.z;
	M[2][3] *= v.z;
	
	M[3][0] *= w;
	M[3][1] *= w;
	M[3][2] *= w;
	M[3][3] *= w;
}

tmatrix&
tmatrix::gl_modelview_get()
{
	float m[4][4];
	glGetFloatv( GL_MODELVIEW_MATRIX, m[0]);
	for (size_t i = 0; i < 4; ++i)
		for (size_t j = 0; j < 4; ++j)
			M[i][j] = static_cast<double>(m[i][j]);
	return *this;
}

tmatrix&
tmatrix::gl_texture_get()
{
	float m[4][4];
	glGetFloatv( GL_TEXTURE_MATRIX, m[0]);
	for (size_t i = 0; i < 4; ++i)
		for (size_t j = 0; j < 4; ++j)
			M[i][j] = static_cast<double>(m[i][j]);
	return *this;
}

tmatrix&
tmatrix::gl_projection_get()
{
	float m[4][4];
	glGetFloatv( GL_PROJECTION_MATRIX, m[0]);
	for (size_t i = 0; i < 4; ++i)
		for (size_t j = 0; j < 4; ++j)
			M[i][j] = static_cast<double>(m[i][j]);
	return *this;
}

tmatrix&
tmatrix::gl_color_get()
{
	float m[4][4];
	glGetFloatv( GL_COLOR_MATRIX, m[0]);
	for (size_t i = 0; i < 4; ++i)
		for (size_t j = 0; j < 4; ++j)
			M[i][j] = static_cast<double>(m[i][j]);
	return *this;
}

gl_matrix_stackguard::gl_matrix_stackguard()
{
	glPushMatrix();
}

gl_matrix_stackguard::gl_matrix_stackguard( const tmatrix& m)
{
	glPushMatrix();
	m.gl_mult();
}

gl_matrix_stackguard::~gl_matrix_stackguard()
{
	glPopMatrix();
}
