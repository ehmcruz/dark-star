#ifndef __DARKSTAR_BODY_HEADER_H__
#define __DARKSTAR_BODY_HEADER_H__

// ---------------------------------------------------

#include <variant>

#include <my-lib/macros.h>

#include <my-game-lib/graphics.h>

#include <darkstar/types.h>
#include <darkstar/lib.h>

// ---------------------------------------------------

namespace DarkStar
{

// ---------------------------------------------------

using MyGlib::Graphics::Shape;
using MyGlib::Graphics::Cube3D;
using MyGlib::Graphics::Sphere3D;

// ---------------------------------------------------

/*
 * Body represents a body that is affected by gravitaty.
 * It may or not have artificial propulsion.
*/

class Body
{
public:


protected:
	OO_ENCAPSULATE_SCALAR(fp_t, mass)
	OO_ENCAPSULATE_OBJ(Point, pos)
	OO_ENCAPSULATE_OBJ(Vector, vel)
	OO_ENCAPSULATE_SCALAR_READONLY(Shape::Type, shape_type)

	OO_ENCAPSULATE_OBJ_INIT(Vector, self_force, Vector::zero())

	// resulting force of a simulation step
	// must be reset to zero before each step
	OO_ENCAPSULATE_OBJ(Vector, rforce)

	std::variant<Cube3D, Sphere3D> shape;

public:
	constexpr Body (const fp_t mass_, const Point& pos_, const Vector& vel_, const Shape::Type shape_type_) noexcept
		: mass(mass_), pos(pos_), vel(vel_), shape_type(shape_type_)
	{
	}

	void process_physics (const fp_t dt) noexcept
	{
		// we manipulate a bit the equations to reduce the number of operations
		const Vector acc_dt = this->rforce / (this->mass / dt);
		this->pos += this->vel * dt + acc_dt * (dt / fp(2));
		this->vel += acc_dt;
	}

	void reset_cycle () noexcept
	{
		this->rforce.set_zero();
	}

	void render ();
};

// ---------------------------------------------------

constexpr fp_t distance (const NaturalBody& a, const NaturalBody& b) noexcept
{
	return (a.get_ref_pos() - b.get_ref_pos()).length();
}

// ---------------------------------------------------

} // end namespace

#endif