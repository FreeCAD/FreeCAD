#include "PreCompiled.h"

#include "Point.h"
#include "DualMath.h"
#include "ParameterRef.h"
#include "ValueSet.h"

using namespace FCS;
using namespace FCS::G2D;


Point::Point(const ValueSet& vals, FCS::ParameterRef x, FCS::ParameterRef y)
    : x(vals[x]), y(vals[y])
{
}

Base::DualNumber Point::operator[](int index) const
{
    assert(index >= 0 && index < 2);
    return index == 0 ? x : y;
}
