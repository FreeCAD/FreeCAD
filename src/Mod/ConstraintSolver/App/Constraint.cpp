#include "Constraint.h"
#include "DualMath.h"

using namespace FCS;
using DualNumber = Base::DualNumber;

TYPESYSTEM_SOURCE_ABSTRACT(FCS::Constraint, Base::BaseClass);

double Constraint::maxStep(const ValueSet& /*dir*/) const
{
    return 1e12;
}

void Constraint::setWeight(double weight)
{
    _weight = weight;
    scale = weight;
}

void Constraint::setProblemSize(ProblemSizeInfo sz)
{
    _sz = sz;
    setWeight(_weight);//to recompute scale
}

std::vector<ParameterRef> Constraint::datumParameters() const
{
    return std::vector<ParameterRef>();
}

std::vector<double> Constraint::caluclateDatum()
{
    return std::vector<double>();
}

HConstraint Constraint::copy() const
{
    throw Py::NotImplementedError("Constraint: copy not implemented yet");
}

Base::DualNumber Constraint::netError(const ValueSet& on) const
{
    std::vector<DualNumber> buf(rank());
    error(on, buf.data());

    if (rank() == 1)
        return buf[0];

    DualNumber acc;
    for(DualNumber v : buf){
        acc = acc + sq(v);
    }
    return sqrt(acc);
}

double Constraint::netError() const
{
    if (parameters().size() == 0)
        throw Base::RuntimeError("constraint is not initialized or updated");

    return netError(parameters()[0].host()->savedValues()).re;
}

void Constraint::operator=(HConstraint other)
{
    //copy everything but _twin
    _parameters = other->_parameters;
    _weight = other->_weight;
    _sz = other->_sz;
    tag = other->tag;
    userData = other->userData;
    label = other->label;
    scale = other->scale;
}

Constraint::~Constraint()
{

}
