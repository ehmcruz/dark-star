#ifndef __DARKSTAR_USER_LIB_HEADER_H__
#define __DARKSTAR_USER_LIB_HEADER_H__

// ---------------------------------------------------

#include <dark-star/types.h>
#include <dark-star/lib.h>
#include <dark-star/body.h>

#include <my-lib/math.h>

// ---------------------------------------------------

namespace DarkStar
{

// ---------------------------------------------------

namespace UserLib
{
	inline constexpr fp_t earth_mass_kg = fp(5.972e24);
	inline constexpr fp_t earth_radius_m = fp(6371e3);

	inline Body make_earth () {
		Body body = Body({
			.type = Body::Type::Planet,
			.mass = kg_to_mass_unit(earth_mass_kg),
			.radius = meters_to_dist_unit(earth_radius_m),
			.pos = Vector::zero(),
			.vel = Vector::zero(),
			.shape_type = Shape::Type::Sphere3D
			});
		
		body.setup_rotation(Mylib::Math::degrees_to_radians(fp(360) / fp(60*60*24)), Vector(0, 1, 0));
		
		return body;
	}

	inline constexpr fp_t moon_mass_kg = fp(7.34767309e22);
	inline constexpr fp_t moon_radius_m = fp(1737.4e3);

	inline Body make_moon () {
		return Body({
			.type = Body::Type::Satellite,
			.mass = kg_to_mass_unit(moon_mass_kg),
			.radius = meters_to_dist_unit(moon_radius_m),
			.pos = Vector::zero(),
			.vel = Vector::zero(),
			.shape_type = Shape::Type::Sphere3D
			});
	}

	inline constexpr fp_t sun_mass_kg = fp(1.98847e30);
	inline constexpr fp_t sun_radius_m = fp(696000e3);

	inline Body make_sun () {
		return Body({
			.type = Body::Type::Star,
			.mass = kg_to_mass_unit(sun_mass_kg),
			.radius = meters_to_dist_unit(sun_radius_m),
			.pos = Vector::zero(),
			.vel = Vector::zero(),
			.shape_type = Shape::Type::Sphere3D
			});
	}

	inline constexpr fp_t distance_from_moon_to_earth_m = fp(384400e3);
	inline constexpr fp_t distance_from_earth_to_sun_m = fp(149.6e9);
}

// ---------------------------------------------------

} // end namespace

#endif