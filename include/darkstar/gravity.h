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
#include <darkstar/dark-star.h>

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
			External,
			Internal
		};

		Type type;
		Point center_pos;
		fp_t size;
		std::variant<ExternalNode, InternalNode> data;
		Node *parent;
		Position parent_pos;

		// The three following variables are set by calc_center_of_mass_bottom_up
		std::size_t n_bodies;
		fp_t mass;
		Vector center_of_mass;
	};

private:
	Node *root;

public:
	// Size_scale is how much we will multiply the size of the given universe.
	// Bodies that eventually fall outisde the universe will be removed
	// from gravity calculation.
	BarnesHutGravitySolver (std::vector<Body>& bodies_, const fp_t size_scale = 2);
	~BarnesHutGravitySolver ();

	void calc_gravity () override final;

private:
	void calc_gravity (Body *body, Node *other_node) const noexcept;

	// remove_body doesn't de-allocate the node, neither the body.
	// Just removes from the tree.
	// The caller is responsible for any memory de-allocation.
	// Returns a pointer to the node that was removed.
	[[nodiscard]] Node* remove_body (Body *body);
	
	void check_body_movement ();

	void insert_body (Body *body, Node *new_node)
	{
		insert_body(body, new_node, this->root);
	}

	static void insert_body (Body *body, Node *new_node, Node *node);
	static void calc_center_of_mass_bottom_up (Node *node);
	static Position map_position (const Vector& pos, const Node *node) noexcept;
	static void setup_external_node (Body *body, Node *node, Node *parent, const Position parent_pos);
	static void destroy_subtree (Node *node);

	static inline Position map_position (const Body *body, const Node *node) noexcept
	{
		return map_position(body->get_ref_pos(), node);
	}

	static inline bool is_body_inside_node (const Body *body, const Node *node) noexcept
	{
		const Vector distance = Mylib::Math::abs(body->get_ref_pos() - node->center_pos);
		const fp_t half_size = node->size / fp(2);

		return (distance.x <= half_size) &&
			   (distance.y <= half_size) &&
			   (distance.z <= half_size);
	}

	[[nodiscard]] static inline Node* allocate_node ()
	{
		return new (memory_manager->allocate_type<Node>(1)) Node;
	}

	static inline void deallocate_node (Node *node)
	{
		memory_manager->deallocate_type<Node>(node, 1);
	}
};

// ---------------------------------------------------

} // end namespace

#endif