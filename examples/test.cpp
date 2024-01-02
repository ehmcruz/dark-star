#include <iostream>
#include <exception>
#include <chrono>
#include <thread>
#include <list>
#include <random>

#include <cstdlib>
#include <cmath>

#include <SDL.h>

#include <my-game-lib/debug.h>
#include <my-game-lib/graphics.h>

#include <darkstar/types.h>
#include <darkstar/config.h>
#include <darkstar/constants.h>
#include <darkstar/body.h>
#include <darkstar/n-body.h>
#include <darkstar/dark-star.h>
#include <darkstar/user-lib.h>

#include <my-lib/math.h>
#include <my-lib/math-vector.h>

// -------------------------------------------

namespace App
{

// -------------------------------------------

using Mylib::Math::degrees_to_radians;
using Mylib::Math::normalize;

using DarkStar::Body;
using DarkStar::BodyDescriptor;
using DarkStar::Shape;

using MyGlib::dprint;
using MyGlib::dprintln;
using MyGlib::Graphics::TextureDescriptor;

using Clock = std::chrono::steady_clock;
using ClockDuration = Clock::duration;
using ClockTime = Clock::time_point;

using DarkStar::fp_t;
using DarkStar::Vector;
using DarkStar::Point;
using DarkStar::gVector;
using DarkStar::gPoint;
using DarkStar::Color;

using DarkStar::renderer;
using DarkStar::event_manager;
using DarkStar::audio_manager;

using DarkStar::fp;
using DarkStar::gfp;
using DarkStar::meters_to_dist_unit;
using DarkStar::k_meters_to_dist_unit;
using DarkStar::kg_to_mass_unit;

// -------------------------------------------

namespace Config {
	inline constexpr fp_t target_fps = 60.0;
	inline constexpr fp_t target_dt = 1.0 / target_fps;
	inline constexpr fp_t sleep_threshold = target_dt * 0.9;
	inline constexpr bool sleep_to_save_cpu = true;
	inline constexpr bool busy_wait_to_ensure_fps = true;
}

// -------------------------------------------

bool alive = true;

DarkStar::N_Body *n_body = nullptr;

Body *earth = nullptr;
Body *moon = nullptr;
Body *sun = nullptr;

Point world_camera_pos;
Point world_camera_target;
Point world_camera_vector;

std::size_t n_steps = 1;

std::list<Body*> cubes;

std::mt19937_64 rgenerator;

// -------------------------------------------

TextureDescriptor texture_earth;
TextureDescriptor texture_moon;

// -------------------------------------------

constexpr ClockDuration fp_to_ClockDuration (const fp_t t)
{
	return std::chrono::duration_cast<ClockDuration>(std::chrono::duration<fp_t>(t));
}

constexpr fp_t ClockDuration_to_fp (const ClockDuration& d)
{
	return std::chrono::duration_cast<std::chrono::duration<fp_t>>(d).count();
}

// -------------------------------------------

Vector gen_random_vector (const fp_t min, const fp_t max, const fp_t min_length)
{
	Vector v;

	std::uniform_real_distribution<fp_t> dist(min, max);

	do {
		v = Vector(dist(rgenerator), 0, dist(rgenerator));
	} while (v.length() < k_meters_to_dist_unit(min_length));
	
	return v;
}

Body create_random_cube ()
{
	auto b = Body (BodyDescriptor {
		.type = Body::Type::Satellite,
		.mass = kg_to_mass_unit(1000),
		.radius = k_meters_to_dist_unit(10000),
		.pos = earth->get_ref_pos() + gen_random_vector(k_meters_to_dist_unit(-300000), k_meters_to_dist_unit(300000), k_meters_to_dist_unit(150000)),
		.vel = gen_random_vector(-k_meters_to_dist_unit(0.9), k_meters_to_dist_unit(0.9), k_meters_to_dist_unit(0.3)),
		.shape_type = Shape::Type::Cube3D
		});

	b.set_color(Color::random(rgenerator));
	b.setup_rotation(degrees_to_radians(fp(360) / fp(60*60*24)), normalize(gen_random_vector(-1, 1, 0.1)));

	dprintln("create_random_cube: pos=", b.get_ref_pos(), " vel=", b.get_ref_vel());

	return b;
}

void create_cubes (const uint32_t n)
{
	for (uint32_t i = 0; i < n; i++) {
		cubes.push_back(&n_body->add_body(create_random_cube()));
	}
}

// -------------------------------------------

static void load ()
{
	std::random_device rd;
	rgenerator.seed( rd() );

	constexpr fp_t scale = 10;

	renderer->begin_texture_loading();
	texture_earth = renderer->load_texture("assets/earth-medium.jpg");
	texture_moon = renderer->load_texture("assets/moon-medium.jpg");
	renderer->end_texture_loading();

	n_body = new DarkStar::N_Body(1000);

	earth = &n_body->add_body(DarkStar::UserLib::make_earth());
	earth->set_radius(earth->get_radius() * scale);
	//earth = &n_body->add_body(Body(kg_to_mass_unit(1000), k_meters_to_dist_unit(0.5), Vector::zero(), Vector::zero(), Shape::Type::Cube3D));
	earth->set_texture(texture_earth);
	earth->set_angular_velocity(earth->get_angular_velocity() * fp(-0.25));

	moon = &n_body->add_body(DarkStar::UserLib::make_moon());
	moon->set_radius(moon->get_radius() * scale);
	moon->set_pos(earth->get_ref_pos() + Vector(meters_to_dist_unit(DarkStar::UserLib::distance_from_moon_to_earth_m), 0, 0));
	moon->set_vel(Vector(0, 0, k_meters_to_dist_unit(0.9)));
	moon->set_texture(texture_moon);

	sun = &n_body->add_body(DarkStar::UserLib::make_sun());
	//sun->set_radius(earth->get_radius() * fp(2));
	//sun->set_pos((earth->get_ref_pos() + moon->get_ref_pos()) / fp(2));
	sun->set_pos(earth->get_ref_pos());
	sun->get_ref_pos().z -= meters_to_dist_unit(DarkStar::UserLib::distance_from_earth_to_sun_m);
	sun->set_color(Color::green());

	create_cubes(10);

	std::cout << std::setprecision(2);
}

// -------------------------------------------

static void unload ()
{
	delete n_body;
}

// -------------------------------------------

static void quit_callback (const MyGlib::Event::Quit::Type& event)
{
	alive = false;
}

// -------------------------------------------

static fp_t setup_step (const fp_t real_dt)
{
	const fp_t virtual_dt = real_dt * fp(3600) * fp(24);
	n_steps = 24*60;

	return virtual_dt;
}

// -------------------------------------------

static void setup_render ()
{
	Vector e_pos = earth->get_value_pos();

	world_camera_vector = Mylib::Math::with_length(Vector(1, -0.5, 1), k_meters_to_dist_unit(6e5));
	world_camera_target = earth->get_ref_pos();
	world_camera_pos = world_camera_target - world_camera_vector;
	//Point center = (earth->get_ref_pos() + moon->get_ref_pos()) / fp(2);
	//world_camera_pos = center;
	//world_camera_pos.y += k_meters_to_dist_unit(100000);
	//world_camera_pos = Vector(e_pos.x, e_pos.y, e_pos.z - k_meters_to_dist_unit(100000));
	//world_camera_pos = Vector(e_pos.x, e_pos.y, e_pos.z - k_meters_to_dist_unit(2));
	//world_camera_target = center;

	n_body->setup_render(world_camera_pos, world_camera_target);
}

// -------------------------------------------

static void main_loop ()
{
	const Uint8 *keys;
	fp_t real_dt, required_dt, sleep_dt, busy_wait_dt, fps, physics_dt, render_dt;

	keys = SDL_GetKeyboardState(nullptr);

	real_dt = 0;
	required_dt = 0;
	sleep_dt = 0;
	busy_wait_dt = 0;
	fps = 0;
	physics_dt = 0;
	render_dt = 0;

	while (alive) {
		const ClockTime tbegin = Clock::now();
		ClockTime tend;
		ClockDuration elapsed;

	#if 1
		dprintln("----------------------------------------------");
		dprintln("start new frame render target_dt=", Config::target_dt,
			" required_dt=", required_dt,
			" real_dt=", real_dt,
			" sleep_dt=", sleep_dt,
			" busy_wait_dt=", busy_wait_dt,
			" target_dt=", Config::target_dt,
			" physics_dt=", physics_dt,
			" render_dt=", render_dt,
			" fps=", fps
			);
	#endif

		DarkStar::event_manager->process_events();
		const fp_t virtual_dt = setup_step(real_dt);

		const ClockTime tbefore_physics = Clock::now();
		n_body->simulate_step(virtual_dt, n_steps);
		const ClockTime tafter_physics = Clock::now();
		physics_dt = ClockDuration_to_fp(tafter_physics - tbefore_physics);
		
		setup_render();

		const ClockTime tbefore_render = Clock::now();
		n_body->render();
		const ClockTime tafter_render = Clock::now();
		render_dt = ClockDuration_to_fp(tafter_render - tbefore_render);

		const ClockTime trequired = Clock::now();
		elapsed = trequired - tbegin;
		required_dt = ClockDuration_to_fp(elapsed);

		if constexpr (Config::sleep_to_save_cpu) {
			if (required_dt < Config::sleep_threshold) {
				sleep_dt = Config::sleep_threshold - required_dt; // target sleep time
				//dprintln( "sleeping for " << delay << "ms..." )
				std::this_thread::sleep_for(fp_to_ClockDuration(sleep_dt));
			}
		}
		
		const ClockTime tbefore_busy_wait = Clock::now();
		elapsed = tbefore_busy_wait - trequired;
		sleep_dt = ClockDuration_to_fp(elapsed); // check exactly time sleeping

		do {
			tend = Clock::now();
			elapsed = tend - tbegin;
			real_dt = ClockDuration_to_fp(elapsed);

			if constexpr (!Config::busy_wait_to_ensure_fps)
				break;
		} while (real_dt < Config::target_dt);

		elapsed = tend - tbefore_busy_wait;
		busy_wait_dt = ClockDuration_to_fp(elapsed);

		fps = 1.0 / real_dt;
	}
}

// -------------------------------------------

void main (int argc, char **argv)
{
	DarkStar::init();

	DarkStar::event_manager->quit().subscribe( Mylib::Trigger::make_callback_function<MyGlib::Event::Quit::Type>(&quit_callback) );

	load();
	main_loop();
	unload();

	DarkStar::quit();
}

// -------------------------------------------

} // namespace App

// -------------------------------------------

int main (int argc, char **argv)
{
	try {
		App::main(argc, argv);
	}
	catch (const std::exception& e) {
		std::cout << "Exception happenned!" << std::endl << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	catch (...) {
		std::cout << "Unknown exception happenned!" << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
