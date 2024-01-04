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
	//return std::ldexp(meters, Config::meters_to_dist_unit_exp);
	//return meters;
	return meters / std::pow(fp(10), Config::distance_exp_factor);
}

constexpr fp_t dist_unit_to_meters (const fp_t dist_unit) noexcept
{
	//return std::ldexp(dist_unit, -Config::meters_to_dist_unit_exp);
	//return dist_unit;
	return dist_unit * std::pow(fp(10), Config::distance_exp_factor);
}

// ---------------------------------------------------

constexpr fp_t kg_to_mass_unit (const fp_t kg) noexcept
{
	//return std::ldexp(kg, Config::kg_to_mass_unit_exp);
	//return kg;
	return kg / std::pow(fp(10), Config::mass_exp_factor);
}

constexpr fp_t mass_unit_to_kg (const fp_t mass_unit) noexcept
{
	//return std::ldexp(mass_unit, -Config::kg_to_mass_unit_exp);
	//return mass_unit;
	return mass_unit * std::pow(fp(10), Config::mass_exp_factor);
}

// ---------------------------------------------------

/*
	We need to make some transformations to the formula to make it work with our units.
	We don't se international units due to floating point precision issues.
	This is not perfect, but it reduces the error.

	g = C * m1 * m2 / d^2

	C = 6.67430e-11
	C_ = 6.67430
	C = C_ * 1e-11

	$d; distance factor
	$m: mass factor

	m1 and m2 are in kg
	m1_ and m2_ are in mass units (stored in the objects)

	d is in meters
	d_ is in distance units (stored in the objects)

	m1 = m1_ * 1e$m
	m2 = m2_ * 1e$m

	d = d_ * 1e$d

	g = C * m1 * m2 / d^2
	g = (C_ * 1e-11) * (m1_ * 1e$m) * (m2_ * 1e$m) / (d_ * 1e$d)^2
	g = C_ * m1_ * m2_ * 1e(-11 + $m + $m) / (d_^2 * 1e2$d)
	g = C_ * m1_ * m2_ * 1e(-11 + $2m) / (d_^2 * 1e2$d)
	g = C_ * m1_ * m2_ * 1e(-11 + $2m - $2d) / d_^2
	g = (C_ * m1_ * m2_ / d_^2) * 1e(-11 + $2m - $2d)

	Force unit: kg * m / s^2

	Then we need to convert the force to our force unit.
	For that, we need to divide the mass by 1e$m, and the distance by 1e$d.
	We don't need to manipulate the time, since we use seconds in the simulation.

	g_unit = g / (1e$m * 1e$d)
	g_unit = ((C_ * m1_ * m2_ / d_^2) * 1e(-11 + $2m - $2d)) / (1e$m * 1e$d)
	g_unit = (C_ * m1_ * m2_ / d_^2) * 1e(-11 + $2m - $2d -$m - $d)
	g_unit = (C_ * m1_ * m2_ / d_^2) * 1e(-11 + $m - 3$d)
*/

constexpr fp_t calc_gravitational_force (const fp_t mass_a, const fp_t mass_b, const fp_t dist) noexcept
{
	//return newtonian_gravitational_constant * mass_a * mass_b / (dist * dist);

	constexpr fp_t C_ = fp(6.67430);
	return (C_ * (mass_a / dist) * (mass_b / dist))
	       * std::pow(fp(10), fp(-11) + Config::mass_exp_factor - fp(3)*Config::distance_exp_factor);
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
	return static_cast<gfp_t>(v / Config::graphics_factor);
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