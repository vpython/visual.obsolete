#include "util/rgba.hpp"

#include <cmath>
#include <GL/gl.h>

rgba
rgba::desaturate() const
{
	const float saturation = 0.5; // cut the saturation by this factor
	float h, s, v;
	rgba ret; ret.alpha = alpha;
	float cmin, cmax, delta;
	int i;
	float f, p, q, t;

	// r,g,b values are from 0 to 1
	// h = [0,360], s = [0,1], v = [0,1]
	//		if s == 0, then arbitrarily set h = 0

	cmin = red;
	if (green < cmin) {
		cmin = green;
	}
	if (blue < cmin) {
		cmin = blue;
	}

	cmax = red;
	if (green > cmax) {
		cmax = green;
	}
	if (blue > cmax) {
		cmax = blue;
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
		if (red == cmax)
			h = ( green - blue ) / delta;		// between yellow & magenta
		else if (green == cmax)
			h = 2.0 + ( blue - red ) / delta;	// between cyan & yellow
		else
			h = 4.0 + ( red - green ) / delta;	// between magenta & cyan

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

rgba
rgba::grayscale() const
{
	// The constants 0.299, 0.587, and 0.114 are intended to account for the 
	// relative intensity of each color to the human eye.
	static const float GAMMA = 2.5;
	const float black = std::pow( 0.299 * std::pow( red, GAMMA) 
		+ 0.587* std::pow( green, GAMMA) 
		+ 0.114* std::pow( blue, GAMMA)
		, 1.0/GAMMA);
	return rgba( black, black, black, alpha);
}
