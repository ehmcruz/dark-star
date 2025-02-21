#ifndef __DARKSTAR_LIB_HEADER_H__
#define __DARKSTAR_LIB_HEADER_H__

// ---------------------------------------------------

#include <cmath>

#include <dark-star/types.h>
#include <dark-star/config.h>
#include <dark-star/constants.h>

// ---------------------------------------------------

namespace DarkStar
{

// ---------------------------------------------------

#ifdef DARKSTAR_SANITY_CHECK
	#define darkstar_sanity_check_msg(cond, ...) mylib_assert_exception_msg(cond, __VA_ARGS__)
	#define darkstar_sanity_check(cond) mylib_assert_exception(cond)
#else
	#define darkstar_sanity_check_msg(cond, ...)
	#define darkstar_sanity_check(cond)
#endif

// ---------------------------------------------------

constexpr fp_t meters_to_dist_unit (const fp_t meters) noexcept
{
	return meters;
}

constexpr fp_t dist_unit_to_meters (const fp_t dist_unit) noexcept
{
	return dist_unit;
}

// ---------------------------------------------------

constexpr fp_t kg_to_mass_unit (const fp_t kg) noexcept
{
	return kg;
}

constexpr fp_t mass_unit_to_kg (const fp_t mass_unit) noexcept
{
	return mass_unit;
}

// ---------------------------------------------------

constexpr fp_t calc_gravitational_force (const fp_t mass_a, const fp_t mass_b, const fp_t dist_squared) noexcept
{
	return newtonian_gravitational_constant * mass_a * (mass_b / dist_squared);
}

// ---------------------------------------------------

constexpr fp_t k_meters_to_dist_unit (const fp_t k_meters) noexcept
{
	return meters_to_dist_unit(k_meters * fp(1000));
}

constexpr fp_t dist_unit_to_k_meters (const fp_t dist_unit) noexcept
{
	return dist_unit_to_meters(dist_unit) / fp(1000);
}

// ---------------------------------------------------

constexpr gVector to_gVector (const Vector& vec) noexcept
{
	return gVector(vec.x, vec.y, vec.z);
}

constexpr gfp_t to_graphics_dist (const fp_t v) noexcept
{
	return static_cast<gfp_t>(v);
}

constexpr gVector to_graphics_dist (const Vector& v) noexcept
{
	return gVector(to_graphics_dist(v.x),
				   to_graphics_dist(v.y),
				   to_graphics_dist(v.z));
}

// ---------------------------------------------------

} // end namespace

#endif