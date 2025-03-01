#ifndef __DARKSTAR_N_BODY_HEADER_H__
#define __DARKSTAR_N_BODY_HEADER_H__

// ---------------------------------------------------

#include <vector>

#include <my-lib/macros.h>

#include <my-game-lib/graphics.h>

#include <dark-star/types.h>
#include <dark-star/lib.h>
#include <dark-star/body.h>
#include <dark-star/gravity.h>

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
	Vector camera_up;

	MYLIB_OO_ENCAPSULATE_PTR_INIT(GravitySolver*, gravity_solver, nullptr)
	MYLIB_OO_ENCAPSULATE_OBJ(std::vector<Body>, bodies)

public:
	N_Body (const std::size_t max_elements_);
	~N_Body ();

	Body& operator[] (const std::size_t i)
	{
		return this->bodies[i];
	}

	Body& add_body (const Body& body);
	
	void simulate_step (const fp_t dt_, const std::size_t n_steps = 1);

	inline void setup_render (const Point& camera_pos, const Point& camera_target, const Vector& camera_up) noexcept
	{
		this->camera_pos = camera_pos;
		this->camera_target = camera_target;
		this->camera_up = camera_up;
	}

	void render ();
};

// ---------------------------------------------------

} // end namespace

#endif