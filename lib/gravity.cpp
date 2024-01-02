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
	this->forces.resize(this->bodies.size());
	dprintln("Thread pool number of threads: ", this->tpool.get_thread_count());
}

void SimpleParallelGravitySolver::calc_gravity ()
{
	const std::size_t n = this->bodies.size();

	if (this->forces.size() != n)
		this->forces.resize(n);
	
	this->tpool.detach_blocks<std::size_t>(0, n,
		[this, n] (const std::size_t i_ini, const std::size_t i_end) -> void {
			for (std::size_t i = i_ini; i < i_end; i++) {
				Body& b1 = this->bodies[i];

				for (std::size_t j = 0; j < n; j++) {
					if (i == j) [[unlikely]]
						continue;

					Body& b2 = this->bodies[j];

					const fp_t dist = Mylib::Math::distance(b1.get_ref_pos(), b2.get_ref_pos());
					const fp_t force = calc_gravitational_force(b1.get_mass(), b2.get_mass(), dist);
					const Vector grav_force = Mylib::Math::with_length(b2.get_ref_pos() - b1.get_ref_pos(), force);

					b1.get_ref_rforce() += grav_force;
				}
			}
		});

	this->tpool.wait();
}

// ---------------------------------------------------

} // end namespace