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
		/*
			An external node represents a single body.
			An internal node represents a group of bodies.

			Any node that is allocated is always an external node.
			Internal nodes only appears when they are upgraded from an external node.

			An internal node, when upgraded from an external node, will have
			two bodies:
				- The body that was already stored in the external node.
				- The body that was just inserted.
			The two bodies can be allocated in the same or different child nodes.
			An internal node can't have zero bodies.

			If after removing a body from the tree an internal node's
			number of children reduces to zero, then we need to remove
			this internal node from the tree.

			Don't confuse number of children with number of bodies.
			Number of children is the amount of non-null pointers in the nodes array.
			Number of bodies is the total amount of bodies in the subtree.
		*/
		
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
		std::size_t n_bodies;

		// The two following variables are set when
		// calc_center_of_mass_top_down is called.
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

	void calc_center_of_mass_top_down ()
	{
		calc_center_of_mass_top_down(this->root);
	}

	void insert_body (Body *body, Node *new_node)
	{
		insert_body(body, new_node, this->root);
	}

	static void insert_body (Body *body, Node *new_node, Node *node);

	// returns the new allocated node
	static Node* upgrade_to_internal (Node *node);
	
	static Position get_only_child_pos (const Node *node);

	static inline Node* get_only_child (const Node *node)
	{
		return std::get<InternalNode>(node->data).nodes[ get_only_child_pos(node) ];
	}

	static void calc_center_of_mass_top_down (Node *node);
	static void calc_center_of_mass_bottom_up (Node *node);
	static void calc_center_of_mass_bottom_up_internal (Node *node);

	static inline void calc_center_of_mass_external (Node *node)
	{
		Body *body = std::get<ExternalNode>(node->data).body;
		node->mass = body->get_mass();
		node->center_of_mass = body->get_ref_pos();
	}

	static void calc_center_of_mass_internal (Node *node);
	static void remove_internal_node (Node *node);
	static void calc_n_bodies_bottom_up (Node *node);
	static void calc_n_bodies_bottom_up_internal (Node *node);
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