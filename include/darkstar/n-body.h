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
	std::vector<Body> bodies;
	std::vector<Body*> stars;
	GravitySolver *gravity_solver;
	MyGlib::Graphics::RenderArgs3D render_opts;
	Point camera_pos;
	Point camera_target;

public:
	N_Body (const std::size_t max_elements_);
	~N_Body ();

	Body& operator[] (const std::size_t i)
	{
		return this->bodies[i];
	}

	Body& add_body (const Body& body);
	
	void simulate_step (const fp_t dt);

	inline void setup_render (const Point& camera_pos, const Point& camera_target) noexcept
	{
		this->camera_pos = camera_pos;
		this->camera_target = camera_target;
	}

	void render ();

	constexpr gfp_t to_graphics_dist (const fp_t v) const noexcept
	{
		return static_cast<gfp_t>(v / fp(1e6));
	}

	constexpr gVector to_graphics_dist (const Vector& v) const noexcept
	{
		return gVector(this->to_graphics_dist(v.x),
		               this->to_graphics_dist(v.y),
					   this->to_graphics_dist(v.z));
	}
};

// ---------------------------------------------------

} // end namespace

#endif