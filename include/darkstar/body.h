#ifndef __DARKSTAR_BODY_HEADER_H__
#define __DARKSTAR_BODY_HEADER_H__

// ---------------------------------------------------

#include <variant>

#include <my-lib/macros.h>
#include <my-lib/std.h>

#include <my-game-lib/graphics.h>

#include <darkstar/types.h>
#include <darkstar/lib.h>

// ---------------------------------------------------

namespace DarkStar
{

// ---------------------------------------------------

using MyGlib::Graphics::Color;
using MyGlib::Graphics::Shape;
using MyGlib::Graphics::Cube3D;
using MyGlib::Graphics::Sphere3D;

// ---------------------------------------------------

/*
 * Body represents a body that is affected by gravitaty.
 * It may or not have artificial propulsion.
 * 
 * I chose not to use a generic Body that could be inherited
 * by Star, Planet, Satellite, etc.
 * The reason is that I want to use std::vector<Body> to store
 * the bodies, such that all bodies would be contiguous in memory.
 * In this way, we have a better cache efficiency and should make
 * it easier for the compiler to use SIMD instructions.
*/

class N_Body;

class Body
{
public:
	enum class Type {
		Star,
		Planet,
		Satellite
	};

	struct Star {
		MyGlib::Graphics::LightPointDescriptor light_desc;
	};

	struct Planet {
	};

protected:
	OO_ENCAPSULATE_SCALAR(Type, type)
	OO_ENCAPSULATE_SCALAR(fp_t, mass)
	OO_ENCAPSULATE_SCALAR(fp_t, radius)
	OO_ENCAPSULATE_OBJ(Point, pos)
	OO_ENCAPSULATE_OBJ(Vector, vel)
	OO_ENCAPSULATE_SCALAR_READONLY(Shape::Type, shape_type)

	OO_ENCAPSULATE_OBJ_INIT(Vector, self_force, Vector::zero())
	OO_ENCAPSULATE_OBJ_INIT(Color, color, Color::white())

	// resulting force of a simulation step
	// must be reset to zero before each step
	OO_ENCAPSULATE_OBJ(Vector, rforce)

	OO_ENCAPSULATE_PTR_INIT(N_Body*, n_body, nullptr)

	std::variant<Cube3D, Sphere3D> shape;
	std::variant<Star, Planet> type_specific;

public:
	Body (const Type type_, const fp_t mass_, const fp_t radius_, const Point& pos_, const Vector& vel_, const Shape::Type shape_type_)
		: type(type_), mass(mass_), radius(radius_), pos(pos_), vel(vel_), shape_type(shape_type_)
	{
		if (this->shape_type == Shape::Type::Sphere3D)
			this->shape = Sphere3D(this->radius);
		else if (this->shape_type == Shape::Type::Cube3D)
			this->shape = Cube3D(this->radius * fp(2));
		else
			mylib_throw_exception_msg("invalid shape type");
	}

	void process_physics (const fp_t dt) noexcept
	{
		// we manipulate a bit the equations to reduce the number of operations
		const Vector acc_dt = this->rforce / (this->mass / dt);
		this->pos += this->vel * dt + acc_dt * (dt / fp(2));
		this->vel += acc_dt;
	}

	void render ();

	auto& get_type_specific () noexcept
	{
		return this->type_specific;
	}

	const auto& get_type_specific () const noexcept
	{
		return this->type_specific;
	}
};

// ---------------------------------------------------

constexpr fp_t distance (const Body& a, const Body& b) noexcept
{
	return (a.get_ref_pos() - b.get_ref_pos()).length();
}

// ---------------------------------------------------

} // end namespace

#endif