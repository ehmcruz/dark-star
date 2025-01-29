#include <algorithm>

#include <cmath>

#ifdef _OPENMP
	#include <omp.h>
#endif

#include <dark-star/types.h>
#include <dark-star/debug.h>
#include <dark-star/config.h>
#include <dark-star/constants.h>
#include <dark-star/body.h>
#include <dark-star/n-body.h>
#include <dark-star/gravity.h>
#include <dark-star/lib.h>
#include <dark-star/dark-star.h>

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

			const Vector direction = b2.get_ref_pos() - b1.get_ref_pos();
			const fp_t dist_squared = direction.length_squared();
			const fp_t force = calc_gravitational_force(b1.get_mass(), b2.get_mass(), dist_squared);
			const Vector grav_force = Mylib::Math::with_length(direction, force);

			b1.get_ref_rforce() += grav_force;
			b2.get_ref_rforce() -= grav_force;
		}
	}
}

// ---------------------------------------------------

SimpleParallelGravitySolver::SimpleParallelGravitySolver (std::vector<Body>& bodies_)
	: GravitySolver(bodies_)
{
}

#if 1
void SimpleParallelGravitySolver::calc_gravity ()
{
	const std::size_t n = this->bodies.size();
	const auto nt = thread_pool->get_thread_count();

	if (this->forces.size() != (n * nt))
		this->forces.resize(n * nt);
	
	for (auto& force : this->forces)
		force.set_zero();
	
	thread_pool->detach_blocks<std::size_t>(0, n,
		[this, n, nt] (const std::size_t i_ini, const std::size_t i_end) -> void {
			const auto tid = *BS::this_thread::get_index();
			const auto row = n * tid;
			//dprintln("Thread ", tid, " calculating gravity for bodies ", i_ini, " to ", i_end - 1);
			for (std::size_t i = i_ini; i < i_end; i++) {
				const Body& b1 = this->bodies[i];
				auto& force_i = this->forces[row + i];

				for (std::size_t j = i + 1; j < n; j++) {
					const Body& b2 = this->bodies[j];

					const Vector direction = b2.get_ref_pos() - b1.get_ref_pos();
					const fp_t dist_squared = direction.length_squared();
					const fp_t force = calc_gravitational_force(b1.get_mass(), b2.get_mass(), dist_squared);
					const Vector grav_force = Mylib::Math::with_length(direction, force);

					auto& force_j = this->forces[row + j];

					force_i += grav_force;
					force_j -= grav_force;
				}
			}
		}, nt * 2); // see the below comment about this number
		// We create twice the number of blocks as threads to try to
		// balance the load between the threads, since the load of
		// the blocks are different.
	
	thread_pool->wait();
	
	for (std::size_t tid = 0; tid < nt; tid++) {
		for (std::size_t i = 0; i < n; i++)
			this->bodies[i].get_ref_rforce() += this->forces[n*tid + i];
	}
}
#else
void SimpleParallelGravitySolver::calc_gravity ()
{
	const std::size_t n = this->bodies.size();
	const auto nt = my_omp_num_threads;

	const std::size_t chunk_size = (n / nt) / 2;

	if (this->forces.size() != (n * nt))
		this->forces.resize(n * nt);
	
	for (auto& force : this->forces)
		force.set_zero();
	
	#pragma omp parallel
	{
		const auto tid = omp_get_thread_num();
		const auto row = n * tid;
		//dprintln("Thread ", tid, " calculating gravity for bodies ", i_ini, " to ", i_end - 1);

		#pragma omp for schedule(dynamic, chunk_size)
		for (std::size_t i = 0; i < n; i++) {
			const Body& b1 = this->bodies[i];
			auto& force_i = this->forces[row + i];

			for (std::size_t j = i + 1; j < n; j++) {
				const Body& b2 = this->bodies[j];

				const Vector direction = b2.get_ref_pos() - b1.get_ref_pos();
				const fp_t dist_squared = direction.length_squared();
				const fp_t force = calc_gravitational_force(b1.get_mass(), b2.get_mass(), dist_squared);
				const Vector grav_force = Mylib::Math::with_length(direction, force);

				auto& force_j = this->forces[row + j];

				force_i += grav_force;
				force_j -= grav_force;
			}
		}
	}
		
	for (std::size_t tid = 0; tid < nt; tid++) {
		for (std::size_t i = 0; i < n; i++)
			this->bodies[i].get_ref_rforce() += this->forces[n*tid + i];
	}
}
#endif

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

#ifdef DARKSTAR_BARNES_HUT_ANALYSIS
	static uint32_t gravity_fast_path;
	static uint32_t gravity_slow_path;
	static uint32_t gravity_slow_path_per_child;
	static uint64_t internal_child_sum;
	static uint64_t internal_child_n;
#endif

void BarnesHutGravitySolver::calc_gravity (Body *body, Node *other_node) const noexcept
{
	Node *node = body->any.get_value<Node*>();

	if (node == other_node) [[unlikely]]
		return;

	const Vector direction = other_node->center_of_mass - body->get_ref_pos();
	const fp_t dist_squared = direction.length_squared();

	if (other_node->type == Node::Type::Internal) [[likely]] {
		// The ratio should actually be calculated with the distance.
		// However, the square root calculation is expensive and was
		// killing the performance.
		// Therefore, we use the distance squared instead.
		const fp_t ratio = (other_node->size * other_node->size) / dist_squared;

		if (ratio > this->theta) [[unlikely]] {
		#ifdef DARKSTAR_BARNES_HUT_ANALYSIS
			gravity_slow_path++;
		#endif
			InternalNode& internal_node = std::get<InternalNode>(other_node->data);

			for (Node *child : internal_node.node_list) {
			#ifdef DARKSTAR_BARNES_HUT_ANALYSIS
				gravity_slow_path_per_child++;
			#endif

				this->calc_gravity(body, child);
			}
			return;
		}
	}

#ifdef DARKSTAR_BARNES_HUT_ANALYSIS
	gravity_fast_path++;
#endif

	const fp_t force = calc_gravitational_force(body->get_mass(), other_node->mass, dist_squared);
	const Vector grav_force = Mylib::Math::with_length(direction, force);

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
			const Position pos = map_position(body, node);

			if (internal_node[pos] == nullptr) {
				internal_node.insert(new_node, pos);
				setup_external_node(body, internal_node[pos], node, pos);
			}
			else
				insert_body(body, new_node, internal_node[pos]);

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
		.node_index = std::array<Node*, 8>({ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }),
		.node_list_pos = std::array<int8_t, 8>({ -1, -1, -1, -1, -1, -1, -1, -1 }),
		.node_list = boost::container::static_vector<Node*, 8>()
		};
	node->n_bodies = 1;

	InternalNode& internal_node = std::get<InternalNode>(node->data);

	// insert the current body
	const Position pos = map_position(body, node);
	internal_node.insert(allocate_node(), pos); // allocate since we need two nodes in this case
	setup_external_node(body, internal_node[pos], node, pos);

	return internal_node[pos];
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
		parent_internal_node.remove(node->parent_pos);
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
		parent_internal_node.remove(node->parent_pos);
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

	node->n_bodies = 0;

	for (Node *child : internal_node.node_list) {
		node->n_bodies += child->n_bodies;
	}

	if (node->parent != nullptr)
		calc_n_bodies_bottom_up_internal(node->parent);
}

void BarnesHutGravitySolver::check_body_movement ()
{
#ifdef DARKSTAR_BARNES_HUT_ANALYSIS
	uint32_t moved_bodies = 0;
#endif
	for (Body& body : this->bodies) {
		Node *node = body.any.get_value<Node*>();

		if (node != nullptr && is_body_inside_node(&body, node) == false) {
			// Body moved to another node
		#ifdef DARKSTAR_BARNES_HUT_ANALYSIS
				moved_bodies++;
		#endif
			// If a body is outside the universe, we don't insert
			// it back in the tree. Therefore, it is important to
			// create a big universe.

			this->move_body_bottom_up(&body);
		}
	}

#ifdef DARKSTAR_BARNES_HUT_ANALYSIS
	dprintln("check_body_movement: moved_bodies=", moved_bodies);
#endif
}

void BarnesHutGravitySolver::move_body_bottom_up (Body *body)
{
	// This function assumes that body has gone outside its node.

	Node *node = body->any.get_value<Node*>();
	Node *parent = node->parent;

	darkstar_sanity_check(node->type == Node::Type::External)
	darkstar_sanity_check_msg(parent != nullptr, "cannot remove root node")
	darkstar_sanity_check(parent->type == Node::Type::Internal)

	auto& parent_internal = std::get<InternalNode>(parent->data);

	// We remove the node from the parent, but not the body.
	// Because the body may still be inside the parent node.
	// For instance, it may have moved from TopNorthEast to TopNorthWest.
	parent_internal.remove(node->parent_pos);

	// We will not de-allocate body.node because, in case the
	// body is still inside the universe, we will re-use it
	// to re-insert the body in its new place in the tree.
	
	this->move_body_bottom_up(body, parent);
}

void BarnesHutGravitySolver::move_body_bottom_up (Body *body, Node *node)
{
	// This function assumes that node is either nullptr or an Internal node.

	if (node == nullptr) [[unlikely]] {
		// Body is outside the universe.
		// We just remove it from the tree and let it float
		// in the universe without gravity.
		deallocate_node(body->any.get_value<Node*>());
		body->any = nullptr;
	}
	else if (is_body_inside_node(body, node)) {
		// Found the node that contains the body.
		// No need to update n_bodies because the body was already inside me.
		// We just insert the body in the tree.
		darkstar_sanity_check(node->type == Node::Type::Internal)

		auto& internal_node = std::get<InternalNode>(node->data);
		const Position pos = map_position(body, node);

		if (internal_node[pos] == nullptr) {
			internal_node.insert(body->any.get_value<Node*>(), pos);
			setup_external_node(body, internal_node[pos], node, pos);
		}
		else
			insert_body(body, body->any.get_value<Node*>(), internal_node[pos]);
	}
	else {
		darkstar_sanity_check(node->n_bodies > 0)
		darkstar_sanity_check(node->type == Node::Type::Internal)

		// Body is outside the node.
		node->n_bodies--;

		Node *parent = node->parent;

		if (node->n_bodies == 0) {
			// The node has no bodies.
			// We can remove it.
			
			darkstar_sanity_check_msg(parent != nullptr, "cannot remove root node")

			auto& internal_parent_node = std::get<InternalNode>(parent->data);

			internal_parent_node.remove(node->parent_pos);
			deallocate_node(node);
		}

		this->move_body_bottom_up(body, parent);
	}
}

void BarnesHutGravitySolver::calc_center_of_mass_top_down (Node *node)
{
	if (node->type == Node::Type::External)
		calc_center_of_mass_external(node);
	else {
		InternalNode& internal_node = std::get<InternalNode>(node->data);

		#ifdef DARKSTAR_BARNES_HUT_ANALYSIS
			internal_child_n++;
		#endif
		
		for (Node *child : internal_node.node_list) {
		#ifdef DARKSTAR_BARNES_HUT_ANALYSIS
			internal_child_sum++;
		#endif
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
	
	for (const Node *child : internal_node.node_list) {
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

		for (Node *n : internal_node.node_list) {
			destroy_subtree(n);
		}
	}

	deallocate_node(node);
}

// ---------------------------------------------------

void BarnesHutGravitySolver::calc_gravity ()
{
#ifdef DARKSTAR_BARNES_HUT_ANALYSIS
	gravity_fast_path = 0;
	gravity_slow_path = 0;
	gravity_slow_path_per_child = 0;
	internal_child_sum = 0;
	internal_child_n = 0;
#endif

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

#ifdef DARKSTAR_BARNES_HUT_ANALYSIS
	dprintln("calc_gravity: fast_path=", gravity_fast_path,
		" slow_path=", gravity_slow_path,
		" ratio=", static_cast<double>(gravity_fast_path) / static_cast<double>(gravity_slow_path + gravity_fast_path),
		" slow_path_per_child=", gravity_slow_path_per_child,
		" ratio_per_child=", static_cast<double>(gravity_fast_path) / static_cast<double>(gravity_slow_path_per_child + gravity_fast_path),
		'\n', "internal child mean=", static_cast<double>(internal_child_sum) / static_cast<double>(internal_child_n)
		);
#endif
}

// ---------------------------------------------------

#if 1
void BarnesHutGravityParallelSolver::calc_gravity ()
{
	// Don't process a body where it's node is nullptr.
	// This happens when a body is outside the universe.
	// This can happen when the universe is too small.
	// Therefore, it is important to create a big universe.

	this->check_body_movement();
	this->calc_center_of_mass_top_down();
	//this->calc_center_of_mass_top_down_parallel(); // it is way slower than the sequential

	const auto n_bodies = this->bodies.size();
	const auto nt = thread_pool->get_thread_count();

	thread_pool->detach_blocks<std::size_t>(0, n_bodies,
		[this] (const std::size_t i_ini, const std::size_t i_end) -> void {
			for (uint32_t i = i_ini; i < i_end; i++) {
				Body& body = this->bodies[i];
				Node *node = body.any.get_value<Node*>();

				if (node != nullptr)
					this->BarnesHutGravitySolver::calc_gravity(&body, this->root);
			}
		}, nt * 2); // see the below comment about this number
		// We create twice the number of blocks as threads to try to
		// balance the load between the threads, since the load of
		// the blocks are different.

	thread_pool->wait();
}
#else
void BarnesHutGravityParallelSolver::calc_gravity ()
{
	// Don't process a body where it's node is nullptr.
	// This happens when a body is outside the universe.
	// This can happen when the universe is too small.
	// Therefore, it is important to create a big universe.

	this->check_body_movement();
	this->calc_center_of_mass_top_down();
	//this->calc_center_of_mass_top_down_parallel();

	const auto n = this->bodies.size();
	const auto nt = my_omp_num_threads;

	const std::size_t chunk_size = (n / nt) / 4;

	#pragma omp parallel for schedule(dynamic, chunk_size)
	for (uint32_t i = 0; i < n; i++) {
		Body& body = this->bodies[i];
		Node *node = body.any.get_value<Node*>();

		if (node != nullptr)
			this->BarnesHutGravitySolver::calc_gravity(&body, this->root);
	}
}
#endif

void BarnesHutGravityParallelSolver::calc_center_of_mass_top_down_parallel (Node *node)
{
#ifdef _OPENMP
	if (node->type == Node::Type::External)
		calc_center_of_mass_external(node);
	else {
		InternalNode& internal_node = std::get<InternalNode>(node->data);

		if (internal_node.node_list.size() > 1) {
			// first, we call all the parallel childs
			uint32_t n_parallel = 0;
			
			for (Node *child : internal_node.node_list) {
				if (child->n_bodies >= this->parallel_threshold) {
					n_parallel++;

					#pragma omp task
					this->calc_center_of_mass_top_down_parallel(child);
				}
			}

			// now, we call all the childs that make no sense to execute in parallel

			for (Node *child : internal_node.node_list) {
				if (child->n_bodies < this->parallel_threshold)
					calc_center_of_mass_top_down(child); // sequential execution
			}

			// now, we wait the parallel childs to finish their job
			if (n_parallel) {
				#pragma omp taskwait
			}
		}
		else
			calc_center_of_mass_top_down(internal_node.node_list[0]); // sequential execution

		// finally we calculate the center of mass of the current node

		calc_center_of_mass_internal(node);
	}
#else
	mylib_throw_exception_msg("OpenMP is not enabled");
#endif
}

void BarnesHutGravityParallelSolver::calc_center_of_mass_top_down_parallel ()
{
#ifdef _OPENMP
	this->parallel_threshold = (this->bodies.size() / my_omp_num_threads) * 2;

	#pragma omp parallel
	{
		#pragma omp single
		this->calc_center_of_mass_top_down_parallel(this->root);
	}
#else
	mylib_throw_exception_msg("OpenMP is not enabled");
#endif
}

// ---------------------------------------------------

} // end namespace