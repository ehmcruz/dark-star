#include <cmath>

#include <darkstar/types.h>
#include <darkstar/body.h>
#include <darkstar/dark-star.h>

// ---------------------------------------------------

namespace DarkStar
{

// ---------------------------------------------------

void Body::render ()
{
	switch (this->shape_type) {
		using enum Shape::Type;

		case Sphere3D:
			renderer->draw_sphere3D(std::get<Sphere3D>(this->shape), to_vec3f(this->pos), this->color);
		break;

		default:
			mylib_throw_exception_msg("invalid shape type");
	}
}

// ---------------------------------------------------

} // end namespace