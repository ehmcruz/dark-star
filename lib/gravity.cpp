#include <algorithm>

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

BarnesHutGravitySolver::BarnesHutGravitySolver (std::vector<Body>& bodies_, const fp_t size_scale)
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

	// We need now to transform the universe size into a cube
	// with the same size in all dimensions.

	const Vector size = top_north_east - bottom_south_west;
	fp_t max_size = std::max({ size.x, size.y, size.z });

	// create a universe size bgger than the size of the universe
	// calculated above
	mylib_assert_exception_msg(size_scale >= fp(2), "size_scale must be greater or equal to 2")
	max_size *= size_scale;

	{
		const fp_t diff = (max_size - size.x) / fp(2);
		top_north_east.x += diff;
		bottom_south_west.x -= diff;
	}
	{
		const fp_t diff = (max_size - size.y) / fp(2);
		top_north_east.y += diff;
		bottom_south_west.y -= diff;
	}
	{
		const fp_t diff = (max_size - size.z) / fp(2);
		top_north_east.z += diff;
		bottom_south_west.z -= diff;
	}

	// create the root node
	this->root = allocate_node();
	auto *node = this->root;

	node->type = Node::Type::External;
	node->center_pos = (top_north_east + bottom_south_west) / fp(2);
	node->size = max_size;
	node->data = ExternalNode {
		.body = &this->bodies[0]
		};
	node->parent = nullptr;
	node->n_bodies = 1;

	this->bodies[0].any = node;

	// insert the other bodies
	for (std::size_t i = 1; i < this->bodies.size(); i++)
		this->insert_body(&this->bodies[i], allocate_node());
}

BarnesHutGravitySolver::~BarnesHutGravitySolver ()
{
	destroy_subtree(this->root);
}

void BarnesHutGravitySolver::calc_gravity ()
{
	// Don't process a body where it's node is nullptr.
	// This happens when a body is outside the universe.
	// This can happen when the universe is too small.
	// Therefore, it is important to create a big universe.

	this->check_body_movement();
	this->calc_center_of_mass_top_down();

	for (Body& body : this->bodies) {
		Node *node = body.any.get_value<Node*>();

		if (node != nullptr)
			this->calc_gravity(&body, this->root);
	}
}

void BarnesHutGravitySolver::calc_gravity (Body *body, Node *other_node) const noexcept
{
	Node *node = body->any.get_value<Node*>();

	if (node == other_node) [[unlikely]]
		return;

	const fp_t dist = Mylib::Math::distance(body->get_ref_pos(), other_node->center_of_mass);

	if (other_node->type == Node::Type::Internal) [[likely]] {
		const fp_t ratio = other_node->size / dist;

		if (ratio > this->theta) [[unlikely]] {
			InternalNode& internal_node = std::get<InternalNode>(other_node->data);
			for (Node *child : internal_node.nodes) {
				if (child == nullptr)
					continue;

				this->calc_gravity(body, child);
			}
			return;
		}
	}

	const fp_t force = calc_gravitational_force(body->get_mass(), other_node->mass, dist);
	const Vector grav_force = Mylib::Math::with_length(other_node->center_of_mass - body->get_ref_pos(), force);

	body->get_ref_rforce() += grav_force;
}

/*
	insert_body

	The new_node variable is the memory pointer to the node that
	will be inserted.
	We need then to pre-allocate this pointer prior to call this function.
	Why we do that?
	Because when a body moves from one node to another, we need
	to delete it from the tree and insert it again.
	To avoid de-allocating and then re-allocation the memory of the node,
	we use this technique of passing the pointer to the that will store the element.
*/

void BarnesHutGravitySolver::insert_body (Body *body, Node *new_node, Node *node)
{
	switch (node->type) {
		case Node::Type::External:
			upgrade_to_internal(node);

		[[fallthrough]];

		case Node::Type::Internal: {
			InternalNode& internal_node = std::get<InternalNode>(node->data);
			auto& nodes = internal_node.nodes;
			const Position pos = map_position(body, node);

			if (nodes[pos] == nullptr) {
				nodes[pos] = new_node;
				setup_external_node(body, nodes[pos], node, pos);
			}
			else
				insert_body(body, new_node, nodes[pos]);

			node->n_bodies++;
		}
		break;
	}
}

BarnesHutGravitySolver::Node* BarnesHutGravitySolver::upgrade_to_internal (Node *node)
{
	darkstar_sanity_check(node->type == Node::Type::External)

	// backup Body
	Body *body = std::get<ExternalNode>(node->data).body;

	// transform current node into an internal node
	node->type = Node::Type::Internal;
	node->data = InternalNode {
		.nodes = std::array<Node*, 8>({ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr })
		};
	node->n_bodies = 1;

	InternalNode& internal_node = std::get<InternalNode>(node->data);
	auto& nodes = internal_node.nodes;

	// insert the current body
	const Position pos = map_position(body, node);
	nodes[pos] = allocate_node(); // allocate since we need two nodes in this case
	setup_external_node(body, nodes[pos], node, pos);

	return nodes[pos];
}

void BarnesHutGravitySolver::setup_external_node (Body *body, Node *node, Node *parent, const Position parent_pos)
{
	const fp_t quarter_size = parent->size / fp(4);

	node->type = Node::Type::External;

	switch (parent_pos) {
		case TopNorthEast:
			node->center_pos = parent->center_pos + quarter_size;
		break;

		case TopNorthWest:
			node->center_pos = parent->center_pos + Vector(-quarter_size, quarter_size, quarter_size);
		break;

		case TopSouthEast:
			node->center_pos = parent->center_pos + Vector(quarter_size, quarter_size, -quarter_size);
		break;

		case TopSouthWest:
			node->center_pos = parent->center_pos + Vector(-quarter_size, quarter_size, -quarter_size);
		break;

		case BottomNorthEast:
			node->center_pos = parent->center_pos + Vector(quarter_size, -quarter_size, quarter_size);
		break;

		case BottomNorthWest:
			node->center_pos = parent->center_pos + Vector(-quarter_size, -quarter_size, quarter_size);
		break;

		case BottomSouthEast:
			node->center_pos = parent->center_pos + Vector(quarter_size, -quarter_size, -quarter_size);
		break;

		case BottomSouthWest:
			node->center_pos = parent->center_pos - quarter_size;
		break;
	}

	node->size = parent->size / fp(2);
	node->data = ExternalNode {
		.body = body
		};
	node->parent = parent;
	node->parent_pos = parent_pos;
	node->n_bodies = 1;

	body->any = node;
}

[[nodiscard]] BarnesHutGravitySolver::Node* BarnesHutGravitySolver::remove_body (Body *body)
{
	Node *node = body->any.get_value<Node*>();

	darkstar_sanity_check(node != nullptr)
	darkstar_sanity_check(node->type == Node::Type::External)

	Node *parent = node->parent;

	darkstar_sanity_check_msg(parent != nullptr, "cannot remove root node")
	
	// remove body from body count on all parent nodes

	for (Node *p = parent; p != nullptr; p = p->parent) {
		darkstar_sanity_check(p->type == Node::Type::Internal)
		darkstar_sanity_check(p->n_bodies >= 1)
		p->n_bodies--;
	}
	
	if (parent->n_bodies == 0) // If the parent node has no bodies, we have to remove it.
		remove_internal_node(parent);
	else {
		InternalNode& parent_internal_node = std::get<InternalNode>(parent->data);
		auto& parent_nodes = parent_internal_node.nodes;
		parent_nodes[node->parent_pos] = nullptr;
	}

	body->any = nullptr;

	// The caller is responsible for deallocating the node.
	// We just return the pointer to the node that was removed.
	// We do this to avoid de-allocating and then re-allocating
	// the memory of the node when a body moves from one node to another.
	// See the check_body_movement function.

	return node;
}

void BarnesHutGravitySolver::remove_internal_node (Node *node)
{
	darkstar_sanity_check(node->type == Node::Type::Internal)
	darkstar_sanity_check(node->n_bodies == 0)

	Node *parent = node->parent;

	darkstar_sanity_check_msg(parent != nullptr, "cannot remove root node")

	if (parent->n_bodies == 0) // If the parent node has no bodies, we have to remove it.
		remove_internal_node(parent);
	else {
		InternalNode& parent_internal_node = std::get<InternalNode>(parent->data);
		auto& parent_nodes = parent_internal_node.nodes;
		parent_nodes[node->parent_pos] = nullptr;
	}

	deallocate_node(node);
}

void BarnesHutGravitySolver::calc_n_bodies_bottom_up (Node *node)
{
	if (node->type == Node::Type::External) {
		node->n_bodies = 1;
		darkstar_sanity_check(node->parent != nullptr)
		calc_n_bodies_bottom_up_internal(node->parent);
	}
	else
		calc_n_bodies_bottom_up_internal(node);
}

void BarnesHutGravitySolver::calc_n_bodies_bottom_up_internal (Node *node)
{
	darkstar_sanity_check(node->type == Node::Type::Internal)

	InternalNode& internal_node = std::get<InternalNode>(node->data);
	auto& nodes = internal_node.nodes;

	node->n_bodies = 0;

	for (Node *child : nodes) {
		if (child != nullptr)
			node->n_bodies += child->n_bodies;
	}

	if (node->parent != nullptr)
		calc_n_bodies_bottom_up_internal(node->parent);
}

BarnesHutGravitySolver::Position BarnesHutGravitySolver::get_only_child_pos (const Node *node)
{
	darkstar_sanity_check(node->type == Node::Type::Internal)

	const InternalNode& internal_node = std::get<InternalNode>(node->data);
	const auto& nodes = internal_node.nodes;
	Position only_child_pos;
	bool found = false;

	for (uint32_t pos = 0; const Node *child : nodes) {
		if (child != nullptr) {
			darkstar_sanity_check(found == false)
			found = true;
			only_child_pos = static_cast<Position>(pos);
		}
		pos++;
	}

	mylib_assert_exception_msg(found == true, "internal node has no children")

	return only_child_pos;
}

void BarnesHutGravitySolver::check_body_movement ()
{
	for (Body& body : this->bodies) {
		Node *node = body.any.get_value<Node*>();

		if (is_body_inside_node(&body, node) == false) {
			// Body moved to another node
			Node *node = this->remove_body(&body);

			// If a body is outside the universe, we don't insert
			// it back in the tree. Therefore, it is important to
			// create a big universe.

			if (is_body_inside_node(&body, this->root))
				this->insert_body(&body, node);
			else {
				// Body is outside the universe.
				// We just remove it from the tree and let it float
				// in the universe without gravity.
				deallocate_node(node);
			}
		}
	}
}

void BarnesHutGravitySolver::calc_center_of_mass_top_down ()
{
	calc_center_of_mass_top_down(this->root);
}

void BarnesHutGravitySolver::calc_center_of_mass_top_down (Node *node)
{
	if (node->type == Node::Type::External)
		calc_center_of_mass_external(node);
	else {
		InternalNode& internal_node = std::get<InternalNode>(node->data);
		auto& nodes = internal_node.nodes;

		for (Node *child : nodes) {
			if (child != nullptr)
				calc_center_of_mass_top_down(child);
		}

		calc_center_of_mass_internal(node);
	}
}

void BarnesHutGravitySolver::calc_center_of_mass_bottom_up (Node *node)
{
	if (node->type == Node::Type::External)
		calc_center_of_mass_external(node);
	else
		calc_center_of_mass_internal(node);

	if (node->parent != nullptr) [[likely]]
		calc_center_of_mass_bottom_up_internal(node->parent);
}

void BarnesHutGravitySolver::calc_center_of_mass_bottom_up_internal (Node *node)
{
	calc_center_of_mass_internal(node);

	if (node->parent != nullptr) [[likely]]
		calc_center_of_mass_bottom_up_internal(node->parent);
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

void BarnesHutGravitySolver::calc_center_of_mass_internal (Node *node)
{
	const InternalNode& internal_node = std::get<InternalNode>(node->data);

	node->mass = 0;
	node->center_of_mass = Vector::zero();

	// first, we need our total mass
	
	for (const Node *child : internal_node.nodes) {
		if (child == nullptr)
			continue;
		
		node->mass += child->mass;
		node->center_of_mass += child->center_of_mass * child->mass;
	}

	// now, we calculate the center of mass
	node->center_of_mass /= node->mass;
}

BarnesHutGravitySolver::Position BarnesHutGravitySolver::map_position (const Vector& pos, const Node *node) noexcept
{
	const Vector& center_pos = node->center_pos;
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
		}
		else {
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
	if (node->type == Node::Type::Internal) {
		InternalNode& internal_node = std::get<InternalNode>(node->data);

		for (Node *n : internal_node.nodes) {
			if (n != nullptr)
				destroy_subtree(n);
		}
	}

	deallocate_node(node);
}

// ---------------------------------------------------

} // end namespace