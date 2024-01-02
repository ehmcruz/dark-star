#include <cmath>

#include <darkstar/types.h>
#include <darkstar/debug.h>
#include <darkstar/config.h>
#include <darkstar/constants.h>
#include <darkstar/body.h>
#include <darkstar/n-body.h>
#include <darkstar/gravity.h>
#include <darkstar/lib.h>

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
		// balence the load between the threads, since the load of
		// the blocks are different.
	
	this->tpool.wait();
	
	for (std::size_t tid = 0; tid < nt; tid++) {
		for (std::size_t i = 0; i < n; i++)
			this->bodies[i].get_ref_rforce() += this->forces[n*tid + i];
	}
}

// ---------------------------------------------------

} // end namespace