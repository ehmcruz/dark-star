#include <cmath>

#include <darkstar/types.h>
#include <darkstar/body.h>
#include <darkstar/n-body.h>
#include <darkstar/debug.h>
#include <darkstar/dark-star.h>

// ---------------------------------------------------

namespace DarkStar
{

// ---------------------------------------------------

void Body::render ()
{
	const gfp_t gradius = this->n_body->to_graphics_dist(this->radius);

	switch (this->shape_type) {
		case Shape::Type::Sphere3D: {
			Sphere3D& sphere = std::get<Sphere3D>(this->shape);
			sphere.set_radius(gradius);
			renderer->draw_sphere3D(sphere, this->n_body->to_graphics_dist(this->pos), this->color);
		}
		break;

		case Shape::Type::Cube3D: {
			Cube3D& cube = std::get<Cube3D>(this->shape);
			cube.set_size(gradius);
			renderer->draw_cube3D(cube, this->n_body->to_graphics_dist(this->pos), this->color);
		}
		break;

		default:
			mylib_throw_exception_msg("invalid shape type");
	}
}

// ---------------------------------------------------

} // end namespace