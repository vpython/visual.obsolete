#include "util/checksum.hpp"

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2003, 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

namespace cvisual {

void
checksum( long& sum, const double* d)
{
	const unsigned char* p = (const unsigned char*)d;
	for (unsigned int i = 0; i < sizeof(double); i++) {
		sum ^= *p;
		p++;
		if (sum < 0)
			sum = (sum << 1) | 1;
		else
			sum <<=  1;
	}
}

void
checksum( long& sum, const float* f)
{
	const unsigned char* p = (const unsigned char*)f;
	for (unsigned int i = 0; i < sizeof(float); i++) {
		sum ^= *p;
		p++;
 		if (sum < 0)
 			sum = (sum << 1) | 1;
		else
			sum <<= 1;
	}
}

} // !namespace cvisual
