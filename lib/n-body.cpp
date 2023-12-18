#include <cmath>

#include <darkstar/types.h>
#include <darkstar/config.h>
#include <darkstar/constants.h>
#include <darkstar/body.h>
#include <darkstar/n-body.h>

// ---------------------------------------------------

namespace DarkStar
{

// ---------------------------------------------------

N_Body::N_Body (const std::size_t max_elements_)
	: max_elements(max_elements_)
{
	this->bodies.reserve(this->max_elements);
}

// ---------------------------------------------------

void N_Body::simulate_step (const fp_t dt)
{
	for (Body& body : this->bodies) {
		body.process_physics(dt);
	}
}

// ---------------------------------------------------

void N_Body::render ()
{
	renderer->wait_next_frame();

	for (Body& body : this->bodies) {
		body.render();
	}
}

// ---------------------------------------------------

} // end namespace