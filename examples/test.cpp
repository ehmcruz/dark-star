#include <iostream>
#include <exception>
#include <chrono>
#include <thread>

#include <cstdlib>
#include <cmath>

#include <SDL.h>

#include <my-game-lib/debug.h>

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

using MyGlib::dprint;
using MyGlib::dprintln;

using Clock = std::chrono::steady_clock;
using ClockDuration = Clock::duration;
using ClockTime = Clock::time_point;

using DarkStar::fp_t;
using DarkStar::Vector;
using DarkStar::gVector;
using DarkStar::Color;

using DarkStar::renderer;
using DarkStar::event_manager;
using DarkStar::audio_manager;

using DarkStar::meters_to_dist_unit;

// -------------------------------------------

namespace Config {
	inline constexpr fp_t target_fps = 30.0;
	inline constexpr fp_t min_fps = 20.0; // if fps gets lower than min_fps, we slow down the simulation
	inline constexpr fp_t target_dt = 1.0 / target_fps;
	inline constexpr fp_t max_dt = 1.0 / min_fps;
	inline constexpr fp_t sleep_threshold = target_dt * 0.9;
	inline constexpr bool sleep_to_save_cpu = true;
	inline constexpr bool busy_wait_to_ensure_fps = true;
	inline constexpr fp_t player_speed = 0.5;
	inline constexpr fp_t camera_rotate_angular_speed = Mylib::Math::degrees_to_radians(fp(90));
	inline constexpr fp_t camera_move_speed = 0.5;
}

// -------------------------------------------

bool alive = true;

DarkStar::N_Body *n_body = nullptr;

Body *earth = nullptr;
Body *moon = nullptr;

MyGlib::Graphics::RenderArgs3D render_opts = {
	.world_camera_pos = gVector::zero(),
	.world_camera_target = gVector::zero(),
	.fov_y = Mylib::Math::degrees_to_radians(fp(45)),
	.z_near = 0.1,
	.z_far = 100,
	.ambient_light_color = {1, 1, 1, 0.3},
	};

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

	moon = &n_body->add_body(DarkStar::UserLib::make_moon());
	moon->set_pos(earth->get_ref_pos() + Vector(meters_to_dist_unit(384400000), 0, 0));
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

	render_opts.world_camera_pos = gVector(e_pos.x, e_pos.y, e_pos.z - meters_to_dist_unit(100000000));
	render_opts.world_camera_target = gVector(e_pos.x, e_pos.y, e_pos.z);

	renderer->setup_render_3D(render_opts);

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
		n_body->simulate_step(virtual_dt);
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
