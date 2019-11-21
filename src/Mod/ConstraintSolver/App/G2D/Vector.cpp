#include "PreCompiled.h"

#include "Vector.h"
#include "DualMath.h"
#include "ParameterRef.h"
#include "ValueSet.h"

using namespace FCS;
using namespace FCS::G2D;


Vector::Vector(const ValueSet& vals, FCS::ParameterRef x, FCS::ParameterRef y)
    : x(vals[x]), y(vals[y])
{
}

Base::DualNumber Vector::length() const
{
    DualNumber sql = sqlength();
    if (::abs(sql.re) < 1e-307) // for zero-length, derivative of sqrt will fail (division by zero). But we still can provide a reasonable derivative.
        return DualNumber(0.0, ::sqrt(x.du*x.du + y.du*y.du));
    return sqrt(sql);
}

Vector Vector::normalized() const
{
    return *this / length(); //#FIXME: maybe, evaluate manually and simplify to make more efficient?
}

Base::DualNumber Vector::operator[](int index) const
{
    assert(index >= 0 && index < 2);
    return index == 0 ? x : y;
}
