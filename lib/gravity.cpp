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

} // end namespace