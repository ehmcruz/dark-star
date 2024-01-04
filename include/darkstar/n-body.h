#ifndef __DARKSTAR_N_BODY_HEADER_H__
#define __DARKSTAR_N_BODY_HEADER_H__

// ---------------------------------------------------

#include <vector>

#include <my-lib/macros.h>

#include <my-game-lib/graphics.h>

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
	std::vector<Body*> stars;
	MyGlib::Graphics::RenderArgs3D render_opts;
	Point camera_pos;
	Point camera_target;

	OO_ENCAPSULATE_PTR_INIT(GravitySolver*, gravity_solver, nullptr)
	OO_ENCAPSULATE_OBJ(std::vector<Body>, bodies)

public:
	N_Body (const std::size_t max_elements_);
	~N_Body ();

	Body& operator[] (const std::size_t i)
	{
		return this->bodies[i];
	}

	Body& add_body (const Body& body);
	
	void simulate_step (const fp_t dt_, const std::size_t n_steps = 1);

	inline void setup_render (const Point& camera_pos, const Point& camera_target) noexcept
	{
		this->camera_pos = camera_pos;
		this->camera_target = camera_target;
	}

	void render ();
};

// ---------------------------------------------------

} // end namespace

#endif