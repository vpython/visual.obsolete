#include "util/checksum.hpp"

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
