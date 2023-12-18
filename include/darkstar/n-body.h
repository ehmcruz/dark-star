#ifndef __DARKSTAR_N_BODY_HEADER_H__
#define __DARKSTAR_N_BODY_HEADER_H__

// ---------------------------------------------------

#include <vector>

#include <my-lib/macros.h>

#include <darkstar/types.h>
#include <darkstar/lib.h>
#include <darkstar/body.h>
#include <darkstar/gravity.h>

// ---------------------------------------------------

namespace DarkStar
{

// ---------------------------------------------------

class N_Body
{
protected:
	const std::size_t max_elements;
	std::vector<Body> bodies;
	GravitySolver *gravity_solver;

public:
	N_Body (const std::size_t max_elements_);
	~N_Body ();

	Body& operator[] (const std::size_t i)
	{
		return this->bodies[i];
	}

	Body& add_body (const Body& body);
	void simulate_step (const fp_t dt);
	void render ();
};

// ---------------------------------------------------

} // end namespace

#endif