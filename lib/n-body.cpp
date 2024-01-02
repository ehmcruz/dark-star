#include <array>
#include <algorithm>
#include <ranges>

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
	//this->gravity_solver = new SimpleParallelGravitySolver(this->bodies);
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

//	dprintln("N_Body::simulate_step: dt = ", dt, ", n_steps = ", n_steps);

	for (uint32_t i = 0; i < n_steps; i++) {
		for (Body& body : this->bodies)
			body.get_ref_rforce().set_zero();
		
		this->gravity_solver->calc_gravity();

		for (Body& body : this->bodies)
			body.process_physics(dt);
	}
}

// ---------------------------------------------------

void N_Body::render ()
{
	struct Range {
		fp_t z_near;
		fp_t z_far;
		fp_t z_middle;
		fp_t z_half_size;
		gfp_t graphics_z_near;
		gfp_t graphics_z_far;
	};

	static constexpr auto z_ranges = [] () -> auto {
		struct DoubleRange {
			double z_near;
			double z_far;
		};
		auto dist_list_meters = std::to_array<DoubleRange>({
			{ .z_near = 0.1, .z_far = 1e2 },
			{ .z_near = 1e2, .z_far = 1e5 },
			{ .z_near = 1e5, .z_far = 1e8 },
			{ .z_near = 1e8, .z_far = 1e11 },
			{ .z_near = 1e11, .z_far = 1e14 },
		});

		std::ranges::reverse(dist_list_meters);

		std::array<Range, dist_list_meters.size()> dist_list;

		for (uint32_t i = 0; i < dist_list_meters.size(); i++) {
			dist_list[i].z_near = meters_to_dist_unit(dist_list_meters[i].z_near);
			dist_list[i].z_far = meters_to_dist_unit(dist_list_meters[i].z_far);
			dist_list[i].z_middle = (dist_list[i].z_near + dist_list[i].z_far) / fp(2);
			dist_list[i].z_half_size = (dist_list[i].z_far - dist_list[i].z_near) / fp(2);
			dist_list[i].graphics_z_near = to_graphics_dist(dist_list[i].z_near);
			dist_list[i].graphics_z_far = to_graphics_dist(dist_list[i].z_far);
		}

		return dist_list;
	}();

//	dprintln("N_Body::render");

	// Calculate the distance of each body to the camera
	std::for_each(this->bodies.begin(), this->bodies.end(), [this] (Body& body) {
		body.distance_to_camera = (body.get_ref_pos() - this->camera_pos).length();
	});

	this->render_opts.world_camera_pos = to_graphics_dist(this->camera_pos);
	this->render_opts.world_camera_target = to_graphics_dist(this->camera_target);

	for (Body *star : this->stars) {
		Body::Star& star_specific = std::get<Body::Star>(star->get_type_specific());
		renderer->move_light_point_source(star_specific.light_desc, to_graphics_dist(star->get_ref_pos()));
	}

	renderer->wait_next_frame();

	for (bool first = true; const Range& range : z_ranges) {
		if (!first) [[likely]]
			renderer->clear_buffers(MyGlib::Graphics::Manager::VertexBufferBit | MyGlib::Graphics::Manager::DepthBufferBit);
		else
			first = false;

		this->render_opts.z_near = range.graphics_z_near;
		this->render_opts.z_far = range.graphics_z_far;

//		dprintln("N_Body::render: rendering range: z_near = ", range.z_near, ", z_far = ", range.z_far);

//	this->render_opts.z_near = to_graphics_dist(k_meters_to_dist_unit(fp(1e4)));
//	this->render_opts.z_far = to_graphics_dist(k_meters_to_dist_unit(fp(1e7)));

		renderer->setup_render_3D(this->render_opts);

		for (Body& body : this->bodies) {
			if (body.is_inside_frustum(range.z_middle, range.z_half_size))
				body.render();
		}

		renderer->render();
	}

	renderer->update_screen();

//	dprintln("N_Body::render: finished rendering");
}

// ---------------------------------------------------

} // end namespace