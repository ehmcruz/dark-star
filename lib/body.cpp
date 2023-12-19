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
		//case Shape::Type::Sphere3D:
			//renderer->draw_sphere3D(std::get<Sphere3D>(this->shape), to_vec3f(this->pos), this->color);
		//break;

		case Shape::Type::Cube3D:
			cube.set_size(gradius);
			//renderer->draw_cube3D(std::get<Cube3D>(this->shape), to_vec3f(this->pos), this->color);
			dprintln("Body::render: pos = ", this->pos);
			renderer->draw_cube3D(cube, this->n_body->to_graphics_dist(this->pos), this->color);
		break;

		default:
			mylib_throw_exception_msg("invalid shape type");
	}
}

// ---------------------------------------------------

} // end namespace