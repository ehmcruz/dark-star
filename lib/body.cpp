#include <cmath>

#include <dark-star/types.h>
#include <dark-star/body.h>
#include <dark-star/n-body.h>
#include <dark-star/debug.h>
#include <dark-star/dark-star.h>

// ---------------------------------------------------

namespace DarkStar
{

// ---------------------------------------------------

Body::Body (const BodyDescriptor& desc)
: type(desc.type),
  mass(desc.mass),
  radius(desc.radius),
  pos(desc.pos),
  vel(desc.vel),
  shape_type(desc.shape_type)
{
	if (this->shape_type == Shape::Type::Sphere3D)
		this->shape = Sphere3D(to_graphics_dist(this->radius));
	else if (this->shape_type == Shape::Type::Cube3D)
		this->shape = Cube3D(to_graphics_dist(this->radius * fp(2)));
	else
		mylib_throw_exception_msg("invalid shape type");

	this->render_specific = RenderColor { .color = Color::white() }; // default
}

// ---------------------------------------------------

void Body::render ()
{
	constexpr fp_t threshold = 0.005;
	fp_t rotation_angle;

	if ((this->radius / this->distance_to_camera) > threshold)
		rotation_angle = this->rotation_angle;
	else
		rotation_angle = 0;

	switch (this->shape_type) {
		case Shape::Type::Sphere3D: {
			Sphere3D& sphere = std::get<Sphere3D>(this->shape);
			sphere.rotate(rotation_angle);

			if (std::holds_alternative<RenderColor>(this->render_specific))
				renderer->draw_sphere3D(sphere, to_graphics_dist(this->pos), std::get<RenderColor>(this->render_specific).color);
			else if (std::holds_alternative<RenderTexture>(this->render_specific))
				renderer->draw_sphere3D(sphere, to_graphics_dist(this->pos), { .desc = std::get<RenderTexture>(this->render_specific).texture_desc });
			else
				mylib_throw_exception_msg("invalid render specific type");
		}
		break;

		case Shape::Type::Cube3D: {
			Cube3D& cube = std::get<Cube3D>(this->shape);
			cube.rotate(rotation_angle);

			if (std::holds_alternative<RenderColor>(this->render_specific))
				renderer->draw_cube3D(cube, to_graphics_dist(this->pos), std::get<RenderColor>(this->render_specific).color);
			else if (std::holds_alternative<RenderTexture>(this->render_specific))
				renderer->draw_cube3D(cube, to_graphics_dist(this->pos), { .desc = std::get<RenderTexture>(this->render_specific).texture_desc });
			else
				mylib_throw_exception_msg("invalid render specific type");
		}
		break;

		default:
			mylib_throw_exception_msg("invalid shape type");
	}
}

// ---------------------------------------------------

void Body::setup_rotation (const fp_t angular_velocity, const Vector& axis)
{
	this->angular_velocity = angular_velocity;

	switch (this->shape_type) {
		case Shape::Type::Sphere3D: {
			Sphere3D& sphere = std::get<Sphere3D>(this->shape);
			sphere.rotate(to_gVector(axis), 0);
		}
		break;

		case Shape::Type::Cube3D: {
			Cube3D& cube = std::get<Cube3D>(this->shape);
			cube.rotate(to_gVector(axis), 0);
		}
		break;
	}
}

// ---------------------------------------------------

void Body::update_radius ()
{
	switch (this->shape_type) {
		case Shape::Type::Sphere3D: {
			Sphere3D& sphere = std::get<Sphere3D>(this->shape);
			sphere.set_radius(to_graphics_dist(this->radius));
		}
		break;

		case Shape::Type::Cube3D: {
			Cube3D& cube = std::get<Cube3D>(this->shape);
			cube.set_size(to_graphics_dist(this->radius * fp(2)));
		}
		break;
	}
}

// ---------------------------------------------------

} // end namespace