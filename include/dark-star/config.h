#ifndef __DARKSTAR_CONFIG_HEADER_H__
#define __DARKSTAR_CONFIG_HEADER_H__

// ---------------------------------------------------

#include <cmath>

#include <dark-star/types.h>

// ---------------------------------------------------

namespace DarkStar
{

// ---------------------------------------------------

//#define DARKSTAR_SANITY_CHECK
//#define DARKSTAR_BARNES_HUT_ANALYSIS

// ---------------------------------------------------

namespace Config
{
	#ifdef DARKSTAR_SANITY_CHECK
		inline constexpr bool sanity_check = true;
	#else
		inline constexpr bool sanity_check = false;
	#endif
}

// ---------------------------------------------------

} // end namespace

#endif