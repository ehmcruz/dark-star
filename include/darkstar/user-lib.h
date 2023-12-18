#ifndef __DARKSTAR_USER_LIB_HEADER_H__
#define __DARKSTAR_USER_LIB_HEADER_H__

// ---------------------------------------------------

#include <darkstar/types.h>
#include <darkstar/lib.h>
#include <darkstar/body.h>

// ---------------------------------------------------

namespace DarkStar
{

// ---------------------------------------------------

namespace UserLib
{
	inline constexpr fp_t earth_mass_kg = fp(5.972e24);
	inline constexpr fp_t earth_radius_m = fp(6371000);
	inline constexpr Body earth (earth_mass_kg, Vector::zero(), Vector::zero(), Shape::Type::Sphere3D);

	inline constexpr fp_t moon_mass_kg = fp(7.34767309e22);
	inline constexpr fp_t moon_radius_m = fp(1737400);
}

// ---------------------------------------------------

} // end namespace

#endif