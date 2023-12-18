#ifndef __DARKSTAR_LIB_HEADER_H__
#define __DARKSTAR_LIB_HEADER_H__

// ---------------------------------------------------

#include <cmath>

#include <darkstar/types.h>
#include <darkstar/config.h>
#include <darkstar/constants.h>

// ---------------------------------------------------

namespace DarkStar
{

// ---------------------------------------------------

constexpr fp_t meters_to_dist_unit (const fp_t meters) noexcept
{
	//return std::ldexp(meters, Config::meters_to_dist_unit_exp);
	return meters;
}

constexpr fp_t dist_unit_to_meters (const fp_t dist_unit) noexcept
{
	//return std::ldexp(dist_unit, -Config::meters_to_dist_unit_exp);
	return dist_unit;
}

// ---------------------------------------------------

constexpr fp_t calc_gravitational_force (const fp_t mass_a, const fp_t mass_b, const fp_t dist) noexcept
{
	return newtonian_gravitational_constant * mass_a * mass_b / (dist * dist);
}

// ---------------------------------------------------

} // end namespace

#endif