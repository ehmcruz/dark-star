#include <cmath>

#include <darkstar/types.h>
#include <darkstar/dark-star.h>

// ---------------------------------------------------

namespace DarkStar
{

// ---------------------------------------------------

void init ()
{
	memory_manager = new Mylib::Memory::DefaultManager();

	game_lib = &MyGlib::Lib::init({
		.graphics_type = MyGlib::Graphics::Manager::Type::Opengl,
		.window_name = "Dark Star",
		.window_width_px = 1900,
		.window_height_px = 900,
		//.fullscreen = true
		.fullscreen = false
	}, *memory_manager);
	event_manager = &game_lib->get_event_manager();
	audio_manager = &game_lib->get_audio_manager();
	renderer = &game_lib->get_graphics_manager();
}

// ---------------------------------------------------

void quit ()
{
	MyGlib::Lib::quit();
}

// ---------------------------------------------------

} // end namespace