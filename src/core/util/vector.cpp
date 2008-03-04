// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.


// Most functions are inlined in the parent header.

#include "util/vector.hpp"
#include "util/tmatrix.hpp"

#include <istream>
#include <ostream>
#include <sstream>

namespace cvisual {
	
vector 
vector::cross( const vector& v) const throw()
{
	vector ret( this->y*v.z - this->z*v.y
		, this->z*v.x - this->x*v.z
		, this->x*v.y - this->y*v.x);
	return ret;
}

/* This function represents the conversion from any Python sequence type to a 
 * visual::vector.  Ideally, we could specify an implicit conversion to be
 * applied whenever explicit argument lookup failed in all cases, but we cannot.
 * So, in nearly all of the places where a vector argument is required (mostly 
 * set_foo() functions), we also explicitly provide an overload for a Python
 * object and perform the conversion here.  Note that this overload will match
 * *anything* from Boost.Python's perspective, including another vector.  So,
 * since this function is much slower than simply accepting a vector argument,
 * we must ensure that Boost.Python tries the explicit vector overload before 
 * resorting to this one.  To make this happen we must rely on the fact that
 * Boost.Python tries to match a signature in the reverse order that they are
 * specified in class_<>::def() and def().
 *
 * Bottom line: Provide the generic overload (usually named set_foo_t() for
 * historical reasons) before the vector form of the overloaded function.
 * Failure to do this will not result in a compile-time error or run-time
 * exception, but will incur a heavy performance penalty.
 */

vector 
vector::norm( void) const throw()
{
	double magnitude = mag();
	if (magnitude)
	// This step ensures that vector(0,0,0).norm() returns vector(0,0,0)
	// instead of NaN
		magnitude = 1.0 / magnitude;
	return vector( x*magnitude, y*magnitude, z*magnitude);
}

// TODO: Figure out why this doesn't quite work...
double
vector::stable_mag(void) const
{
	double ret = 0.0;
	
	double x1 = std::fabs(x);
	double x2 = std::fabs(y);
	double x3 = std::fabs(z);
	// sort the temporaries into descending order.
	if (x1 < x2)
		std::swap( x1, x2);
	if (x2 < x3) {
		std::swap(x2, x3);
		if (x1 < x2)
			std::swap( x1, x2);
	}
	
	if (x1 == 0.0)
		return 0.0;
	if (x2 == 0.0)
		return x1;
	// at this point, ret is equal to the length of an R2 vector.
	ret = x1 / std::cos( std::atan( x1/ x2));
	if (x3 == 0.0)
		return ret;
	ret = ret / std::cos( std::atan( ret/x3));
	return ret;	
}

// Evaluate the determinant:
// | x1, y1, z1  |
// | x2, y2, z2  |
// | x3, y3, z3  |
double 
vector::dot_b_cross_c( const vector& b, const vector& c) const throw()
{
	return ( this->x*(b.y*c.z - b.z*c.y)
	       - this->y*(b.x*c.z - b.z*c.x)
	       + this->z*(b.x*c.y - b.y*c.x)
	       );
}
  
  
/* Vector triple product.
 */
vector 
vector::cross_b_cross_c( const vector& b, const vector& c) const throw()
{
	return (this->dot( c) * b - this->dot( b) * c);
}

// Vector projections
double 
vector::comp( const vector& v) const throw()
{
	return (this->dot( v) / v.mag());
}

vector 
vector::proj( const vector& v) const throw()
{
	return (this->dot( v)/v.mag2() * v);
}

double 
vector::diff_angle( const vector& v) const throw()
{
	double magfirst = this->mag2();
	double magsecond = v.mag2();
	if (magfirst == 0.0 || magsecond == 0.0)
		return (double) 0.0;
	return std::acos( this->dot( v) / std::sqrt(magfirst*magsecond) );
}
  
std::string 
vector::repr( void) const
{
	std::stringstream ret;
	ret.precision( std::numeric_limits<double>::digits10);
	// Since this function is inteded to produce Python code that can be used to 
	// rebuild this object, we use the full precision of the data type here.
	ret << "vector(" << x << ", " << y << ", " << z << ")";
	return ret.str();
}

vector
vector::rotate( double angle, vector axis) throw()
{
	tmatrix R = rotation( angle, axis);
	return R * *this;
}

double
vector::py_getitem( int index) const
{
	switch (index) {
		case -3:
			return x;
		case -2:
			return y;
		case -1:
			return z;
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		default:
			std::ostringstream s;
			s << "vector index out of bounds: " << index;
			throw std::out_of_range( s.str() );
	}
}

void
vector::py_setitem(int index, double value)
{
	switch (index) {
		case -3:
			x = value;
			break;
		case -2:
			y = value;
			break;
		case -1:
			z = value;
			break;
		case 0:
			x = value;
			break;
		case 1:
			y = value;
			break;
		case 2:
			z = value;
			break;
		default:
			std::ostringstream s;
			s << "vector index out of bounds: " << index;
			throw std::out_of_range( s.str() );
	}    	
}

bool 
vector::stl_cmp( const vector& v) const
{
	if (this->x != v.x) {
		return this->x < v.x;
	}
	else if (this->y != v.y) {
		return this->y < v.y;
	}
	else return this->z < v.z;
}

} // !namespace cvisual
