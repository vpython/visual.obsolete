#include "util/rgba.hpp"

#include <cmath>

static rgb
desaturate( const rgb& c)
{
	const float saturation = 0.5; // cut the saturation by this factor
	float h, s, v;
	rgb ret;
	float cmin, cmax, delta;
	int i;
	float f, p, q, t;

	// r,g,b values are from 0 to 1
	// h = [0,360], s = [0,1], v = [0,1]
	//		if s == 0, then arbitrarily set h = 0

	cmin = c.red;
	if (c.green < cmin) {
		cmin = c.green;
	}
	if (c.blue < cmin) {
		cmin = c.blue;
	}

	cmax = c.red;
	if (c.green > cmax) {
		cmax = c.green;
	}
	if (c.blue > cmax) {
		cmax = c.blue;
	}
	v = cmax;				// v

	delta = cmax - cmin;

	if (cmin == cmax) { // completely unsaturated; color is some gray
		// if r = g = b = 0, s = 0, v in principle undefined but set to 0
		s = 0.0;
		h = 0.0;
	} 
	else {
		s = delta / cmax;		// s
		if (c.red == cmax)
			h = ( c.green - c.blue ) / delta;		// between yellow & magenta
		else if (c.green == cmax)
			h = 2.0 + ( c.blue - c.red ) / delta;	// between cyan & yellow
		else
			h = 4.0 + ( c.red - c.green ) / delta;	// between magenta & cyan

		if (h < 0.0)
			h += 6.0;  // make it 0 <= h < 6
	}

	// unsaturate somewhat to make sure both eyes have something to see
	s *= saturation;
	
	if (s == 0.0) {
		// achromatic (grey)
		ret.red = ret.green = ret.blue = v;
	}
	else {
		i = static_cast<int>( h);  // h represents sector 0 to 5
		f = h - i;                 // fractional part of h
		p = v * ( 1.0 - s );
		q = v * ( 1.0 - s * f );
		t = v * ( 1.0 - s * ( 1.0 - f ) );

		switch (i) {
			case 0:
				ret.red = v;
				ret.green = t;
				ret.blue = p;
				break;
			case 1:
				ret.red = q;
				ret.green = v;
				ret.blue = p;
				break;
			case 2:
				ret.red = p;
				ret.green = v;
				ret.blue = t;
				break;
			case 3:
				ret.red = p;
				ret.green = q;
				ret.blue = v;
				break;
			case 4:
				ret.red = t;
				ret.green = p;
				ret.blue = v;
				break;
			default:		// case 5:
				ret.red = v;
				ret.green = p;
				ret.blue = q;
				break;
		}
	}
	return ret;
}

static rgb
grayscale( const rgb& c)
{
	// The constants 0.299, 0.587, and 0.114 are intended to account for the 
	// relative intensity of each color to the human eye.
	static const float GAMMA = 2.5;
	const float black = std::pow( 0.299 * std::pow( c.red, GAMMA) 
		+ 0.587* std::pow( c.green, GAMMA) 
		+ 0.114* std::pow( c.blue, GAMMA)
		, 1.0/GAMMA);
	return rgb( black, black, black);
}

rgba
rgba::desaturate() const
{
	rgb ret = ::desaturate( rgb(red, green, blue));
	return rgba( ret.red, ret.green, ret.blue, alpha);
}

rgba
rgba::grayscale() const
{
	rgb ret = ::grayscale( rgb(red, blue, green));
	return rgba( ret.red, ret.green, ret.blue, alpha);
}

rgb
rgb::desaturate() const
{
	return ::desaturate( *this);
}

rgb
rgb::grayscale() const
{
	return ::grayscale( *this);
}
