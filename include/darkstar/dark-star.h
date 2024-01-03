#ifndef __DARKSTAR_MAIN_HEADER_H__
#define __DARKSTAR_MAIN_HEADER_H__

// ---------------------------------------------------

#include <my-game-lib/my-game-lib.h>

#include <my-lib/memory.h>

#include <darkstar/types.h>

// ---------------------------------------------------

namespace DarkStar
{

// ---------------------------------------------------

inline Mylib::Memory::Manager *memory_manager = nullptr;

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