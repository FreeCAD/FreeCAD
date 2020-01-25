#include "PreCompiled.h"

#include "Position.h"
#include "DualMath.h"
#include "ParameterRef.h"
#include "ValueSet.h"

using namespace FCS;
using namespace FCS::G2D;


Position::Position(const ValueSet& vals, FCS::ParameterRef x, FCS::ParameterRef y)
    : x(vals[x]), y(vals[y])
{
}

Base::DualNumber Position::operator[](int index) const
{
    assert(index >= 0 && index < 2);
    return index == 0 ? x : y;
}

PyObject* Position::getPyObject() const
{
    //for now, returning a vector would suffice.
    //#fixme: make py binding
    return Vector(*this).getPyObject();
}
