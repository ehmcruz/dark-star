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

constexpr fp_t k_meters_to_dist_unit (const fp_t k_meters) noexcept
{
	return meters_to_dist_unit(k_meters * fp(1000));
}

constexpr fp_t dist_unit_to_meters (const fp_t dist_unit) noexcept
{
	//return std::ldexp(dist_unit, -Config::meters_to_dist_unit_exp);
	return dist_unit;
}

constexpr fp_t dist_unit_to_k_meters (const fp_t dist_unit) noexcept
{
	return dist_unit_to_meters(dist_unit) / fp(1000);
}

// ---------------------------------------------------

constexpr fp_t kg_to_mass_unit (const fp_t kg) noexcept
{
	//return std::ldexp(kg, Config::kg_to_mass_unit_exp);
	return kg;
}

constexpr fp_t mass_unit_to_kg (const fp_t mass_unit) noexcept
{
	//return std::ldexp(mass_unit, -Config::kg_to_mass_unit_exp);
	return mass_unit;
}

// ---------------------------------------------------

constexpr fp_t calc_gravitational_force (const fp_t mass_a, const fp_t mass_b, const fp_t dist) noexcept
{
	return newtonian_gravitational_constant * mass_a * mass_b / (dist * dist);
}

// ---------------------------------------------------

constexpr Vector3f to_vec3f (const Vector3& vec) noexcept
{
	return Vector3f(vec.x, vec.y, vec.z);
}

// ---------------------------------------------------

} // end namespace

#endif