#ifndef __DARKSTAR_BODY_HEADER_H__
#define __DARKSTAR_BODY_HEADER_H__

// ---------------------------------------------------

#include <variant>
#include <string_view>

#include <cmath>

#include <my-lib/macros.h>
#include <my-lib/std.h>
#include <my-lib/any.h>

#include <my-game-lib/graphics.h>

#include <dark-star/types.h>
#include <dark-star/lib.h>

// ---------------------------------------------------

namespace DarkStar
{

// ---------------------------------------------------

using MyGlib::Graphics::Color;
using MyGlib::Graphics::TextureDescriptor;
using MyGlib::Graphics::TextureRenderOptions;
using MyGlib::Graphics::Shape;
using MyGlib::Graphics::Cube3D;
using MyGlib::Graphics::Sphere3D;

// ---------------------------------------------------

/*
 * Body represents a body that is affected by gravitaty.
 * It may or not have artificial propulsion.
 * 
 * I chose not to use a generic Body that could be inherited
 * by Star, Planet, Satellite, etc.
 * The reason is that I want to use std::vector<Body> to store
 * the bodies, such that all bodies would be contiguous in memory.
 * In this way, we have a better cache efficiency and should make
 * it easier for the compiler to use SIMD instructions.
*/

class N_Body;
struct BodyDescriptor;

class Body
{
public:
	Mylib::Any<sizeof(void*)> any;

public:
	enum class Type {
		Star,
		Planet,
		Satellite
	};

	struct Star {
		MyGlib::Graphics::LightPointDescriptor light_desc;
	};

	struct Planet {
	};

protected:
	struct RenderColor {
		Color color;
	};

	struct RenderTexture {
		TextureDescriptor texture_desc;
	};

protected:
	MYLIB_OO_ENCAPSULATE_SCALAR(Type, type)
	MYLIB_OO_ENCAPSULATE_SCALAR(fp_t, mass)

	// the following variable write function is set manually
	MYLIB_OO_ENCAPSULATE_SCALAR_READONLY(fp_t, radius)

	MYLIB_OO_ENCAPSULATE_OBJ_WITH_COPY_MOVE(Point, pos)
	MYLIB_OO_ENCAPSULATE_OBJ_WITH_COPY_MOVE(Vector, vel)
	MYLIB_OO_ENCAPSULATE_SCALAR_READONLY(Shape::Type, shape_type)

	MYLIB_OO_ENCAPSULATE_OBJ_INIT(Vector, self_force, Vector::zero())
	MYLIB_OO_ENCAPSULATE_SCALAR_INIT(fp_t, angular_velocity, 0)
	MYLIB_OO_ENCAPSULATE_SCALAR_INIT(fp_t, rotation_angle, 0)

	// resulting force of a simulation step
	// must be reset to zero before each step
	MYLIB_OO_ENCAPSULATE_OBJ_WITH_COPY_MOVE(Vector, rforce)

	MYLIB_OO_ENCAPSULATE_PTR_INIT(N_Body*, n_body, nullptr)

	std::variant<Cube3D, Sphere3D> shape;
	std::variant<Star, Planet> type_specific;
	std::variant<RenderColor, RenderTexture> render_specific;

	fp_t distance_to_camera; // used by N_Body::render()

public:
	Body (const BodyDescriptor& desc);

	void process_physics (const fp_t dt) noexcept
	{
		// we manipulate a bit the equations to reduce the number of operations
		const Vector acc_dt = this->rforce / (this->mass / dt);
		this->pos += this->vel * dt + acc_dt * (dt / fp(2));
		this->vel += acc_dt;
		this->rotation_angle = std::fmod(this->rotation_angle + this->angular_velocity * dt, Mylib::Math::degrees_to_radians(fp(360)));
	}

	void render ();

	auto& get_type_specific () noexcept
	{
		return this->type_specific;
	}

	const auto& get_type_specific () const noexcept
	{
		return this->type_specific;
	}

	void set_color (const Color& color) noexcept
	{
		this->render_specific = RenderColor { .color = color };
	}

	void set_texture (const TextureDescriptor& texture_desc) noexcept
	{
		this->render_specific = RenderTexture {
			.texture_desc = texture_desc
			};
	}

	inline void set_radius (const fp_t radius)
	{
		this->radius = radius;
		this->update_radius();
	}

	void setup_rotation (const fp_t angular_velocity, const Vector& axis);
	void update_radius ();

	/*
		is_inside_frustum is used to determine if the body is inside the frustum.
		Bodies that are outside the frustum are not rendered.
		z_middle is the z coordinate of the middle of the frustum.
		z_half_size is the half of the z size of the frustum.
	*/

	bool is_inside_frustum (const fp_t z_middle, const fp_t z_half_size) const noexcept
	{
		bool r = false;
		const fp_t distance_between_centers = std::abs(this->distance_to_camera - z_middle);

		switch (this->shape_type) {
			case Shape::Type::Sphere3D:
				r = (distance_between_centers <= (this->radius + z_half_size));
			break;

			case Shape::Type::Cube3D:
				r = (distance_between_centers <= (this->radius * fp(1.5) + z_half_size));
			break;
		}

		return r;
	}

	// --------------------------

	friend class N_Body;
};

// ---------------------------------------------------

struct BodyDescriptor
{
	Body::Type type;
	fp_t mass;
	fp_t radius;
	Point pos;
	Vector vel;
	Shape::Type shape_type;
};

// ---------------------------------------------------

constexpr fp_t distance (const Body& a, const Body& b) noexcept
{
	return (a.get_ref_pos() - b.get_ref_pos()).length();
}

// ---------------------------------------------------

} // end namespace

#endif