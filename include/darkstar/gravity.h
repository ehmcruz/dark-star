#ifndef __DARKSTAR_GRAVITY_HEADER_H__
#define __DARKSTAR_GRAVITY_HEADER_H__

// ---------------------------------------------------

#include <cmath>

#include <darkstar/types.h>
#include <darkstar/config.h>
#include <darkstar/constants.h>
#include <darkstar/body.h>

#include <BS_thread_pool.hpp>

// ---------------------------------------------------

namespace DarkStar
{

// ---------------------------------------------------

class GravitySolver
{
protected:
	std::vector<Body>& bodies;

public:
	GravitySolver (std::vector<Body>& bodies_)
		: bodies(bodies_)
	{
	}

	virtual ~GravitySolver () = default;

	virtual void calc_gravity () = 0;
};

// ---------------------------------------------------

class SimpleGravitySolver : public GravitySolver
{
private:

public:
	SimpleGravitySolver (std::vector<Body>& bodies_)
		: GravitySolver(bodies_)
	{
	}

	void calc_gravity () override final;
};

// ---------------------------------------------------

class SimpleParallelGravitySolver : public GravitySolver
{
private:
	std::vector<Vector> forces;
	BS::thread_pool tpool;

public:
	SimpleParallelGravitySolver (std::vector<Body>& bodies_);

	void calc_gravity () override final;
};

// ---------------------------------------------------

} // end namespace

#endif