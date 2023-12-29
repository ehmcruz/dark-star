#include <cmath>

#include <darkstar/types.h>
#include <darkstar/debug.h>
#include <darkstar/config.h>
#include <darkstar/constants.h>
#include <darkstar/body.h>
#include <darkstar/n-body.h>
#include <darkstar/dark-star.h>

// ---------------------------------------------------

namespace DarkStar
{

// ---------------------------------------------------

N_Body::N_Body (const std::size_t max_elements_)
	: max_elements(max_elements_)
{
	this->bodies.reserve(this->max_elements);

	this->render_opts.fov_y = Mylib::Math::degrees_to_radians(gfp(45));
	this->render_opts.z_near = 0.1f;
	this->render_opts.z_far = 100.0f;
	this->render_opts.ambient_light_color = {1, 1, 1, 0.2};

	this->gravity_solver = new SimpleGravitySolver(this->bodies);
}

// ---------------------------------------------------

N_Body::~N_Body ()
{
	delete this->gravity_solver;
}

// ---------------------------------------------------

Body& N_Body::add_body (const Body& body)
{
	mylib_assert_exception_msg(this->bodies.size() < this->max_elements, "N_Body::add_body: max_elements of ", this->max_elements, " reached")

	this->bodies.push_back(body);
	Body& b = this->bodies.back();
	b.set_n_body(this);

	if (b.get_type() == Body::Type::Star) {
		b.get_type_specific() = Body::Star {
			.light_desc = renderer->add_light_point_source(
				gPoint(0, 0, 0), Color::white()
			)
		};

		this->stars.push_back(&b);
	}

	return b;
}

// ---------------------------------------------------

void N_Body::simulate_step (const fp_t dt_, const std::size_t n_steps)
{
	const fp_t dt = dt_ / static_cast<fp_t>(n_steps);

	for (uint32_t i = 0; i < n_steps; i++) {
//		dprintln("N_Body::simulate_step: dt = ", dt);

		for (Body& body : this->bodies)
			body.get_ref_rforce().set_zero();
		
		this->gravity_solver->calc_gravity();

		for (Body& body : this->bodies)
			body.process_physics(dt);

//		dprintln("N_Body::simulate_step: finished processing physics");
	}
}

// ---------------------------------------------------

void N_Body::render ()
{
//	dprintln("N_Body::render");

	this->render_opts.world_camera_pos = to_graphics_dist(this->camera_pos);
	this->render_opts.world_camera_target = to_graphics_dist(this->camera_target);

	this->render_opts.z_near = to_graphics_dist(k_meters_to_dist_unit(fp(1e4)));
	this->render_opts.z_far = to_graphics_dist(k_meters_to_dist_unit(fp(1e7)));

	renderer->setup_render_3D(this->render_opts);

	renderer->wait_next_frame();

	for (Body *star : this->stars) {
		Body::Star& star_specific = std::get<Body::Star>(star->get_type_specific());
		renderer->move_light_point_source(star_specific.light_desc, to_graphics_dist(star->get_ref_pos()));
	}

	for (Body& body : this->bodies)
		body.render();

	renderer->render();
	renderer->update_screen();

//	dprintln("N_Body::render: finished rendering");
}

// ---------------------------------------------------

} // end namespace