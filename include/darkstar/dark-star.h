#ifndef __DARKSTAR_MAIN_HEADER_H__
#define __DARKSTAR_MAIN_HEADER_H__

// ---------------------------------------------------

#include <my-game-lib/my-game-lib.h>

#include <my-lib/memory.h>

#include <BS_thread_pool.hpp>

#include <darkstar/types.h>

#ifdef _OPENMP
	#include <omp.h>
#endif

// ---------------------------------------------------

namespace DarkStar
{

// ---------------------------------------------------

inline Mylib::Memory::Manager *memory_manager = nullptr;

inline BS::thread_pool *thread_pool = nullptr;

#ifdef _OPENMP
	inline const int my_omp_num_threads = omp_get_max_threads();
#endif

inline MyGlib::Lib *game_lib = nullptr;
inline MyGlib::Graphics::Manager *renderer = nullptr;
inline MyGlib::Event::Manager *event_manager = nullptr;
inline MyGlib::Audio::Manager *audio_manager = nullptr;

// ---------------------------------------------------

void init ();
void quit ();

// ---------------------------------------------------

} // end namespace

#endif