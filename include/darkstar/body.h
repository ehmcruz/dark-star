#ifndef __DARKSTAR_BODY_HEADER_H__
#define __DARKSTAR_BODY_HEADER_H__

// ---------------------------------------------------

#include <my-lib/macros.h>

#include <darkstar/types.h>
#include <darkstar/lib.h>

// ---------------------------------------------------

namespace DarkStar
{

// ---------------------------------------------------

/*
 * NaturalBody represents a body that follows a gravitational orbit.
 * It doesn't have any artificial propulsion.
*/

class NaturalBody
{
protected:
	OO_ENCAPSULATE_SCALAR(fp_t, mass)
	OO_ENCAPSULATE_OBJ(Point, pos)
	OO_ENCAPSULATE_OBJ(Vector, vel)

	// resulting force of a simulation step
	// must be reset to zero before each step
	OO_ENCAPSULATE_OBJ(Vector, rforce)

public:
	NaturalBody (const fp_t mass_, const Point& pos_, const Vector& vel_) noexcept
		: mass(mass_), pos(pos_), vel(vel_)
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
};

// ---------------------------------------------------

/*
 * ArtificialBody represents a body that has artificial propulsion.
*/

class ArtificialBody : public NaturalBody
{
protected:
	OO_ENCAPSULATE_OBJ_INIT(Vector, self_force, Vector::zero())

public:
	using NaturalBody::NaturalBody;
};

// ---------------------------------------------------

constexpr fp_t distance (const NaturalBody& a, const NaturalBody& b) noexcept
{
	return (a.get_ref_pos() - b.get_ref_pos()).length();
}

// ---------------------------------------------------

} // end namespace

#endif