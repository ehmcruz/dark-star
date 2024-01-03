#include <cmath>

#include <darkstar/types.h>
#include <darkstar/debug.h>
#include <darkstar/config.h>
#include <darkstar/constants.h>
#include <darkstar/body.h>
#include <darkstar/n-body.h>
#include <darkstar/gravity.h>
#include <darkstar/lib.h>
#include <darkstar/dark-star.h>

// ---------------------------------------------------

namespace DarkStar
{

// ---------------------------------------------------

void SimpleGravitySolver::calc_gravity ()
{
	const std::size_t n = this->bodies.size();

	for (std::size_t i = 0; i < n; i++) {
		Body& b1 = this->bodies[i];

		for (std::size_t j = i + 1; j < n; j++) {
			Body& b2 = this->bodies[j];

			const fp_t dist = Mylib::Math::distance(b1.get_ref_pos(), b2.get_ref_pos());
			const fp_t force = calc_gravitational_force(b1.get_mass(), b2.get_mass(), dist);
			const Vector grav_force = Mylib::Math::with_length(b2.get_ref_pos() - b1.get_ref_pos(), force);

			b1.get_ref_rforce() += grav_force;
			b2.get_ref_rforce() -= grav_force;
		}
	}
}

// ---------------------------------------------------

SimpleParallelGravitySolver::SimpleParallelGravitySolver (std::vector<Body>& bodies_)
	: GravitySolver(bodies_)
{
	dprintln("Thread pool number of threads: ", this->tpool.get_thread_count());
}

void SimpleParallelGravitySolver::calc_gravity ()
{
	const std::size_t n = this->bodies.size();
	const auto nt = this->tpool.get_thread_count();

	if (this->forces.size() != (n * nt))
		this->forces.resize(n * nt);
	
	for (auto& force : this->forces)
		force.set_zero();
	
	this->tpool.detach_blocks<std::size_t>(0, n,
		[this, n, nt] (const std::size_t i_ini, const std::size_t i_end) -> void {
			const auto tid = *BS::this_thread::get_index();
			const auto row = n * tid;
			//dprintln("Thread ", tid, " calculating gravity for bodies ", i_ini, " to ", i_end - 1);
			for (std::size_t i = i_ini; i < i_end; i++) {
				const Body& b1 = this->bodies[i];
				auto& force_i = this->forces[row + i];

				for (std::size_t j = i + 1; j < n; j++) {
					const Body& b2 = this->bodies[j];

					const fp_t dist = Mylib::Math::distance(b1.get_ref_pos(), b2.get_ref_pos());
					const fp_t force = calc_gravitational_force(b1.get_mass(), b2.get_mass(), dist);
					const Vector grav_force = Mylib::Math::with_length(b2.get_ref_pos() - b1.get_ref_pos(), force);

					auto& force_j = this->forces[row + j];

					force_i += grav_force;
					force_j -= grav_force;
				}
			}
		}, nt * 2); // see the below comment about this number
		// We create twice the number of blocks as threads to try to
		// balance the load between the threads, since the load of
		// the blocks are different.
	
	this->tpool.wait();
	
	for (std::size_t tid = 0; tid < nt; tid++) {
		for (std::size_t i = 0; i < n; i++)
			this->bodies[i].get_ref_rforce() += this->forces[n*tid + i];
	}
}

// ---------------------------------------------------

BarnesHutGravitySolver::BarnesHutGravitySolver (std::vector<Body>& bodies_)
	: GravitySolver(bodies_)
{
	mylib_assert_exception_msg(bodies_.size() > 0, "bodies vector is empty")

	// Check universe size
	Vector top_north_east = this->bodies[0].get_ref_pos();
	Vector bottom_south_west = this->bodies[0].get_ref_pos();

	for (const Body& body : bodies_) {
		const Vector& pos = body.get_ref_pos();

		if (pos.x > top_north_east.x)
			top_north_east.x = pos.x;
		if (pos.y > top_north_east.y)
			top_north_east.y = pos.y;
		if (pos.z > top_north_east.z)
			top_north_east.z = pos.z;

		if (pos.x < bottom_south_west.x)
			bottom_south_west.x = pos.x;
		if (pos.y < bottom_south_west.y)
			bottom_south_west.y = pos.y;
		if (pos.z < bottom_south_west.z)
			bottom_south_west.z = pos.z;
	}

	// create a universe size with twice the size of the universe
	// calculated above
	top_north_east *= fp(2);
	bottom_south_west *= fp(2);

	// create the root node
	this->root = new (memory_manager->allocate_type<Node>(1)) Node;
	this->root->center_pos = (top_north_east + bottom_south_west) / fp(2);
	this->root->size = top_north_east - bottom_south_west;
	this->root->half_size = this->root->size / fp(2);

	// insert the bodies
	for (Body& body : this->bodies)
		this->insert_body(&body, this->root);
}

BarnesHutGravitySolver::~BarnesHutGravitySolver ()
{
	this->destroy_subtree(this->root);
}

void BarnesHutGravitySolver::calc_gravity ()
{

}

void BarnesHutGravitySolver::calc_gravity (Body *body, Node *node) const noexcept
{

}

void BarnesHutGravitySolver::insert_body (Body *body, Node *node)
{
	switch (node->type) {
		case Node::Type::Empty:
			node->type = Node::Type::External;
			node->data = ExternalNode {
				.body = body
				};
			body->any = node;
			this->calc_center_of_mass_bottom_up(node);
		break;

		case Node::Type::External: {
			ExternalNode& external_node = std::get<ExternalNode>(node->data);

			// backup current Body
			Body *current_body = external_node.body;

			// transform current node into an internal node
			node->data = this->create_internal_node(node);
			node->type = Node::Type::Internal;

			InternalNode& internal_node = std::get<InternalNode>(node->data);

			// insert the current body
			this->insert_body(current_body, internal_node.nodes[this->map_position(current_body, node)]);

			// insert the new body
			this->insert_body(body, internal_node.nodes[this->map_position(body, node)]);
		}
		break;

		case Node::Type::Internal: {
			InternalNode& internal_node = std::get<InternalNode>(node->data);
			this->insert_body(body, internal_node.nodes[this->map_position(body, node)]);
		}
		break;
	}
}

void BarnesHutGravitySolver::remove_body (Body *body)
{
	Node *node = body->any.get_ref<Node*>();

	darkstar_sanity_check(node->type == Node::Type::External, "node is not an external node")

	Node *parent = node->parent;
	const Position parent_pos = node->parent_pos;

	node->reset();

	if (parent == nullptr) [[unlikely]] // root node
		return;
	
	darkstar_sanity_check(parent->type == Node::Type::Internal, "parent node is not an internal node")

	parent->n_bodies--;

	darkstar_sanity_check(parent->n_bodies > 0, "parent has no elements")

	if (parent->n_bodies == 1) {
		// we have to transform the parent node into an external node

		InternalNode& internal_node = std::get<InternalNode>(parent->data);
		auto& nodes = internal_node.nodes;

		if constexpr (Config::sanity_checks) {
			uint32_t n_children = 0;
			uint32_t n_children_external = 0;
			
			for (Node *child : nodes) {
				n_children += (child->type != Node::Type::Empty);
				
				if (child->type == Node::Type::External) {
					n_children_external++;

					ExternalNode& child_external = std::get<ExternalNode>(child->data);
					mylib_assert_exception_msg(child_external.body == body, "external child node is not the body we are removing")
				}
			}

			mylib_assert_exception_msg(n_children == 1, "parent node has more than one child")
			mylib_assert_exception_msg(n_children_external == 1, "parent node has more than one external child")
		}

		// We can free the memory of all children nodes

		for (Node *child : nodes) {
			darkstar_sanity_check(child->type == Node::Type::Empty || child->type == Node::Type::External, "child node is empty")
			this->destroy_subtree(child);
		}

		// node was already child of parent,
		// so we don't have to remove it because
		// it was already removed in the previus loop

		// Now, we have to transform the parent node into an external node

		parent->type = Node::Type::External;
		parent->data = ExternalNode {
			.body = body
			};
		body->any = parent;

		this->calc_center_of_mass_bottom_up(parent);
	}
	else // we have to recalculate the center of mass of the parent node
		this->calc_center_of_mass_bottom_up(parent);
}

void BarnesHutGravitySolver::check_body_movement ()
{
	for (Body& body : this->bodies) {
		Node*& node = body.any.get_ref<Node*>();

		if (this->is_body_inside_node(&body, node) == false) {
			// Body moved to another node
			this->remove_body(&body);
			this->insert_body(&body, this->root);
		}
	}
}

BarnesHutGravitySolver::InternalNode BarnesHutGravitySolver::create_internal_node (Node *node)
{
	mylib_assert_exception_msg(node->type == Node::Type::External, "source node is not an external node")

	InternalNode internal_node;
	auto& nodes = internal_node.nodes;

	const Vector half_size = node->size / fp(2);
	const Vector quarter_size = node->size / fp(4);

	for (uint32_t i = 0; i < nodes.size(); i++) {
		nodes[i] = new (memory_manager->allocate_type<Node>(1)) Node;
		auto *n = nodes[i];
		n->parent = node;
		n->parent_pos = static_cast<Position>(i);
		n->size = half_size;
		n->half_size = quarter_size;
	}

	nodes[TopNorthEast]->center_pos = node->center_pos + quarter_size;
	nodes[TopNorthWest]->center_pos = node->center_pos + Vector(-quarter_size.x, quarter_size.y, quarter_size.z);
	nodes[TopSouthEast]->center_pos = node->center_pos + Vector(quarter_size.x, quarter_size.y, -quarter_size.z);
	nodes[TopSouthWest]->center_pos = node->center_pos + Vector(-quarter_size.x, quarter_size.y, -quarter_size.z);
	nodes[BottomNorthEast]->center_pos = node->center_pos + Vector(quarter_size.x, -quarter_size.y, quarter_size.z);
	nodes[BottomNorthWest]->center_pos = node->center_pos + Vector(-quarter_size.x, -quarter_size.y, quarter_size.z);
	nodes[BottomSouthEast]->center_pos = node->center_pos + Vector(quarter_size.x, -quarter_size.y, -quarter_size.z);
	nodes[BottomSouthWest]->center_pos = node->center_pos - quarter_size;

	return internal_node;
}

/*
	Calculation of the center of mass of a node

	Supposing that node is another Internal node, therefore it represents
	the center of mass of the bodies inside its region.
	Suppose this region has bodies a and b, where
	- ma is the mass of a
	- mb is the mass of b
	- pa is the position of a
	- pb is the position of b

	We have:
		ma + mb = mass
		(pa*ma + pb*mb) / mass = center_of_mass
		(pa*ma + pb*mb) = center_of_mass * mass

	We can calculate the center of mass of the bodies inside a region
	making use of a center of mass of a sub-region, such that:

		new_mass = ma + mb + mc = (ma + mb) + mc = mass + mc

		new_center_of_mass = (pa*ma + pb*mb + pc*mc) / new_mass
			= (pa*ma + pb*mb) / new_mass + (pc*mc) / new_mass
			= (center_of_mass * mass) / new_mass + (pc*mc) / new_mass
		
	Remember also that in an External region, the center of mass is
	is the position of the only body inside it:
		pc = center_of_mass_pc
		pc = ((pc * mc) / mc)
	
	Therefore, we can calculate the center of mass of a node by:

		new_center_of_mass = (center_of_mass * mass) / new_mass + (pc*mc) / new_mass
			= (center_of_mass * mass) / new_mass + (center_of_mass_pc*mc) / new_mass
			= sum of children [child_center_of_mass * child_mass / new_mass]
*/

void BarnesHutGravitySolver::calc_center_of_mass_bottom_up (Node *node)
{
	if (node == nullptr) [[unlikely]] // we reached the root node
		return;

	switch (node->type) {
		case Node::Type::Empty:
			mylib_assert_exception_msg(false, "node is empty")
		break;

		case Node::Type::External: {
			const ExternalNode& external_node = std::get<ExternalNode>(node->data);
			const Body *body = external_node.body;
			node->n_bodies = 1;
			node->mass = body->get_mass();
			node->center_of_mass = body->get_ref_pos();
		}
		break;

		case Node::Type::Internal: {
			const InternalNode& internal_node = std::get<InternalNode>(node->data);

			node->n_bodies = 0;
			node->mass = 0;
			node->center_of_mass = Vector::zero();

			// first, we need our total mass
			
			for (const Node *child : internal_node.nodes) {
				node->n_bodies += child->n_bodies;
				node->mass += child->mass;
				node->center_of_mass += child->center_of_mass * child->mass;
			}

			// now, we calculate the center of mass of our children
			node->center_of_mass /= node->mass;
		}
		break;
	}

	this->calc_center_of_mass_bottom_up(node->parent);
}

BarnesHutGravitySolver::Position BarnesHutGravitySolver::map_position (const Vector& pos, const Node *node) const noexcept
{
	const Vector& center_pos = node->center_pos;
	const Vector& size = node->size;
	Position r;

	if (pos.x > center_pos.x) {
		if (pos.y > center_pos.y) {
			if (pos.z > center_pos.z)
				r = TopNorthEast;
			else
				r = TopSouthEast;
		}
		else {
			if (pos.z > center_pos.z)
				r = BottomNorthEast;
			else
				r = BottomSouthEast;
		}
	}
	else {
		if (pos.y > center_pos.y) {
			if (pos.z > center_pos.z)
				r = TopNorthWest;
			else
				r = TopSouthWest;
		} else {
			if (pos.z > center_pos.z)
				r = BottomNorthWest;
			else
				r = BottomSouthWest;
		}
	}

	return r;
}

void BarnesHutGravitySolver::destroy_subtree (Node *node)
{
	switch (node->type) {
		case Node::Type::Empty: [[fallthrough]];
		case Node::Type::External:
			memory_manager->deallocate_type<Node>(node, 1);
		break;

		case Node::Type::Internal: {
			InternalNode& internal_node = std::get<InternalNode>(node->data);

			for (Node *node : internal_node.nodes)
				this->destroy_subtree(node);

			memory_manager->deallocate_type<Node>(node, 1);
		}
		break;
	}
}

// ---------------------------------------------------

} // end namespace