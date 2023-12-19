#include <iostream>
#include <exception>
#include <chrono>
#include <thread>

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

// -------------------------------------------

namespace App
{

// -------------------------------------------

using DarkStar::Body;
using DarkStar::Shape;

using MyGlib::dprint;
using MyGlib::dprintln;

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
using DarkStar::k_meters_to_dist_unit;
using DarkStar::kg_to_mass_unit;

// -------------------------------------------

namespace Config {
	inline constexpr fp_t target_fps = 30.0;
	inline constexpr fp_t min_fps = 20.0; // if fps gets lower than min_fps, we slow down the simulation
	inline constexpr fp_t target_dt = 1.0 / target_fps;
	inline constexpr fp_t max_dt = 1.0 / min_fps;
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

static void load ()
{
	n_body = new DarkStar::N_Body(1000);

	earth = &n_body->add_body(DarkStar::UserLib::make_earth());
	//earth = &n_body->add_body(Body(kg_to_mass_unit(1000), k_meters_to_dist_unit(0.5), Vector::zero(), Vector::zero(), Shape::Type::Cube3D));
	earth->set_color(Color::blue());

	moon = &n_body->add_body(DarkStar::UserLib::make_moon());
	moon->set_pos(earth->get_ref_pos() + Vector(k_meters_to_dist_unit(384400), 0, 0));
	moon->set_color(Color::white());

	sun = &n_body->add_body(DarkStar::UserLib::make_sun());
	sun->set_radius(earth->get_radius() * fp(2));
	sun->set_pos((earth->get_ref_pos() + moon->get_ref_pos()) / fp(2));
	sun->get_ref_pos().z -= k_meters_to_dist_unit(30000);
	sun->set_color(Color::green());

	std::cout << std::setprecision(30);
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

static fp_t setup_step (fp_t virtual_dt)
{
	Vector e_pos = earth->get_value_pos();

	world_camera_vector = Vector(1, -1, 1);
	world_camera_vector.set_length(k_meters_to_dist_unit(300000));
	world_camera_target = sun->get_ref_pos();
	world_camera_pos = world_camera_target - world_camera_vector;
	//Point center = (earth->get_ref_pos() + moon->get_ref_pos()) / fp(2);
	//world_camera_pos = center;
	//world_camera_pos.y += k_meters_to_dist_unit(100000);
	//world_camera_pos = Vector(e_pos.x, e_pos.y, e_pos.z - k_meters_to_dist_unit(100000));
	//world_camera_pos = Vector(e_pos.x, e_pos.y, e_pos.z - k_meters_to_dist_unit(2));
	//world_camera_target = center;

	n_body->setup_render(world_camera_pos, world_camera_target);

	dprintln("earth pos=", earth->get_ref_pos(), " r=", earth->get_radius());
	//dprintln("moon pos=", moon->get_ref_pos(), " r=", moon->get_radius());

//exit(1);
	return virtual_dt;
}

// -------------------------------------------

static void main_loop ()
{
	const Uint8 *keys;
	fp_t real_dt, virtual_dt, required_dt, sleep_dt, busy_wait_dt, fps;

	keys = SDL_GetKeyboardState(nullptr);

	real_dt = 0;
	virtual_dt = 0;
	required_dt = 0;
	sleep_dt = 0;
	busy_wait_dt = 0;
	fps = 0;

	while (alive) {
		const ClockTime tbegin = Clock::now();
		ClockTime tend;
		ClockDuration elapsed;

		virtual_dt = (real_dt > Config::max_dt) ? Config::max_dt : real_dt;

	#if 1
		dprintln("----------------------------------------------");
		dprintln("start new frame render target_dt=", Config::target_dt,
			" required_dt=", required_dt,
			" real_dt=", real_dt,
			" sleep_dt=", sleep_dt,
			" busy_wait_dt=", busy_wait_dt,
			" virtual_dt=", virtual_dt,
			" max_dt=", Config::max_dt,
			" target_dt=", Config::target_dt,
			" fps=", fps
			);
	#endif

		DarkStar::event_manager->process_events();
		virtual_dt = setup_step(virtual_dt);
		//n_body->simulate_step(virtual_dt);
		n_body->render();

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
