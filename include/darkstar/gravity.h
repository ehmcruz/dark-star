#ifndef __DARKSTAR_GRAVITY_HEADER_H__
#define __DARKSTAR_GRAVITY_HEADER_H__

// ---------------------------------------------------

#include <variant>
#include <array>

#include <cmath>

#include <my-lib/macros.h>

#include <darkstar/types.h>
#include <darkstar/config.h>
#include <darkstar/constants.h>
#include <darkstar/body.h>

#include <BS_thread_pool.hpp>

// ---------------------------------------------------

namespace DarkStar
{

// ---------------------------------------------------

class GravitySolver
{
protected:
	std::vector<Body>& bodies;

public:
	GravitySolver (std::vector<Body>& bodies_)
		: bodies(bodies_)
	{
	}

	virtual ~GravitySolver () = default;

	virtual void calc_gravity () = 0;
};

// ---------------------------------------------------

class SimpleGravitySolver : public GravitySolver
{
private:

public:
	SimpleGravitySolver (std::vector<Body>& bodies_)
		: GravitySolver(bodies_)
	{
	}

	void calc_gravity () override final;
};

// ---------------------------------------------------

class SimpleParallelGravitySolver : public GravitySolver
{
private:
	std::vector<Vector> forces;
	BS::thread_pool tpool;

public:
	SimpleParallelGravitySolver (std::vector<Body>& bodies_);

	void calc_gravity () override final;
};

// ---------------------------------------------------

class BarnesHutGravitySolver : public GravitySolver
{
public:
	struct Node;

private:
	enum Position {
		// West-East refers to the X axis
		// Top-Bottom refers to the Y axis
		// North-South refers to the Z axis

		TopNorthEast,
		TopNorthWest,
		TopSouthEast,
		TopSouthWest,
		BottomNorthEast,
		BottomNorthWest,
		BottomSouthEast,
		BottomSouthWest
	};

	struct ExternalNode {
		Body *body;
	};

	struct InternalNode {
		std::array<Node*, 8> nodes;
	};

	OO_ENCAPSULATE_SCALAR_INIT(fp_t, theta, 0.5)

public:
	struct Node {
		enum class Type {
			Empty,
			External,
			Internal
		};

		Type type;
		Vector center_pos;
		Vector size;
		Vector half_size;
		std::variant<ExternalNode, InternalNode> data;

		std::size_t n_bodies;
		fp_t mass;
		Vector center_of_mass;

		Node *parent;
		Position parent_pos;

		Node () noexcept
		{
			this->reset();
		}

		void reset () noexcept
		{
			this->type = Type::Empty;
			this->n_bodies = 0;
			this->mass = 0;
			this->center_of_mass = Vector::zero();
			this->parent = nullptr;
		}
	};

private:
	Node *root = nullptr;

public:
	BarnesHutGravitySolver (std::vector<Body>& bodies_);
	~BarnesHutGravitySolver ();

	void calc_gravity () override final;

private:
	void insert_body (Body *body, Node *node);
	void remove_body (Body *body);
	InternalNode create_internal_node (Node *node);
	void destroy_subtree (Node *node);
	Position map_position (const Vector& pos, const Node *node) const noexcept;

	Position map_position (const Body *body, const Node *node) const noexcept
	{
		return this->map_position(body->get_ref_pos(), node);
	}

	void calc_gravity (Body *body, Node *node) const noexcept;
	void calc_center_of_mass_bottom_up (Node *node);
	void check_body_movement ();

	bool is_body_inside_node (const Body *body, const Node *node) const noexcept
	{
		const Vector& pos = body->get_ref_pos();
		const Vector& half_size = node->half_size;
		const Vector& center_pos = node->center_pos;
		const Vector distance = Mylib::Math::abs(pos - center_pos);

		return (distance.x <= half_size.x) &&
			   (distance.y <= half_size.y) &&
			   (distance.z <= half_size.z);
	}
};

// ---------------------------------------------------

} // end namespace

#endif