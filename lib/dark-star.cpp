#include <cmath>

#include <my-lib/memory-pool.h>

#include <dark-star/types.h>
#include <dark-star/debug.h>
#include <dark-star/dark-star.h>

// ---------------------------------------------------

namespace DarkStar
{

// ---------------------------------------------------

void init ()
{
	//memory_manager = new Mylib::Memory::DefaultManager();
	memory_manager = new Mylib::Memory::PoolManager(1024, 8, 32 * 1024);

	thread_pool = new BS::thread_pool;
	
	dprintln("Thread pool number of threads: ", thread_pool->get_thread_count());

	#ifdef _OPENMP
	{
		#pragma omp parallel
		{
			#pragma omp single nowait
			{
				const auto nt = omp_get_num_threads();
				mylib_assert_exception(nt == my_omp_num_threads)
			}
		}
	}
	#endif

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