#ifndef VPYTHON_CONFIG_H
#define VPYTHON_CONFIG_H

#define check check_fix

#include <math.h>

#ifndef M_PI
# define M_PI 3.14159265359
#endif

#if defined _MSC_VER
# pragma warning(disable: 4996)
# pragma warning(disable: 4005)
# pragma warning(disable: 4715)
#endif

#define BOOST_DATE_TIME_NO_LIB

#endif
