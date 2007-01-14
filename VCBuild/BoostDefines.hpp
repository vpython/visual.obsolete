//----------------------------------------------------------------------------
// 
//----------------------------------------------------------------------------
// Filename:      BoostDefines.hpp
// Type:          Declaration
// Language:      Microsoft Visual C++ V7.1
// Module:        
//----------------------------------------------------------------------------
// Description:   class BoostDefines
//----------------------------------------------------------------------------
#ifndef BOOSTDEFINES_HPP
#define BOOSTDEFINES_HPP

#pragma once

#define BOOST_AUTO_LINK_NOMANGLE
#define BOOST_ALL_DYN_LINK
#define BOOST_THREAD_USE_DLL  //thread header not compliant with 'BOOST_ALL_DYN_LINK'
#define BOOST_LIB_DIAGNOSTIC


#ifdef BOOST_ALL_DYN_LINK
#pragma message("Include: dynamic linking with boost libraries")
#endif


#endif //BOOSTDEFINES_HPP
