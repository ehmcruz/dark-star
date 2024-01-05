#ifndef __DARKSTAR_CONFIG_HEADER_H__
#define __DARKSTAR_CONFIG_HEADER_H__

// ---------------------------------------------------

#include <cmath>

#include <darkstar/types.h>

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

	inline constexpr fp_t distance_exp_factor = 9;
	inline constexpr fp_t mass_exp_factor = 9;
	inline constexpr fp_t graphics_factor = 1e6;
}

// ---------------------------------------------------

} // end namespace

#endif