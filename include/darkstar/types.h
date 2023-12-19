#ifndef __DARKSTAR_TYPES_HEADER_H__
#define __DARKSTAR_TYPES_HEADER_H__

// ---------------------------------------------------

#include <my-lib/macros.h>
#include <my-lib/math-vector.h>

// ---------------------------------------------------

namespace DarkStar
{

// ---------------------------------------------------

using fp_t = double;
using Vector3 = Mylib::Math::Vector<fp_t, 3>;
using Vector3f = Mylib::Math::Vector<float, 3>;
using Vector = Vector3;
using Point = Vector;

// for graphics we use float, not double
using gfp_t = float;
using gVector = Vector3f;
using gPoint = Vector3f;

// ---------------------------------------------------

consteval fp_t fp (const auto v) noexcept
{
	return static_cast<fp_t>(v);
}

consteval gfp_t gfp (const auto v) noexcept   // graphics fp
{
	return static_cast<gfp_t>(v);
}

// ---------------------------------------------------

} // end namespace

#endif