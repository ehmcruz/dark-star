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

	inline Body make_earth () {
		return Body(kg_to_mass_unit(earth_mass_kg), meters_to_dist_unit(earth_radius_m), Vector::zero(), Vector::zero(), Shape::Type::Sphere3D);
	}

	inline constexpr fp_t moon_mass_kg = fp(7.34767309e22);
	inline constexpr fp_t moon_radius_m = fp(1737400);

	inline Body make_moon () {
		return Body(kg_to_mass_unit(moon_mass_kg), meters_to_dist_unit(moon_radius_m), Vector::zero(), Vector::zero(), Shape::Type::Sphere3D);
	}
}

// ---------------------------------------------------

} // end namespace

#endif