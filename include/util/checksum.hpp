#ifndef VPYTHON_UTIL_CHECKSUM_HPP
#define VPYTHON_UTIL_CHECKSUM_HPP

// Copyright (c) 2000, 2001, 2002, 2003 by David Scherer and others.
// Copyright (c) 2004 by Jonathan Brandmeyer and others.
// See the file license.txt for complete license terms.
// See the file authors.txt for a complete list of contributors.

namespace cvisual {

void checksum( long& sum, const double* d);
void checksum( long& sum, const float* f);

} // !namespace cvisual

#endif // !defined VPYTHON_CHECKSUM_HPP
