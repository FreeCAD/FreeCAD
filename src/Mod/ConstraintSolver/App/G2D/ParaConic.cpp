#include "PreCompiled.h"

#include "ParaConic.h"
#include "G2D/ParaConicPy.h"
#include "G2D/ParaPointPy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE_ABSTRACT(FCS::G2D::ParaConic, FCS::G2D::ParaCurve);



PyObject* ParaConic::getPyObject()
{
    if (!_twin){
        _twin = new ParaConicPy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}

Position ParaConic::getFocus2(const ValueSet& vals) const
{
    Position f1 = getFocus1(vals);
    Position c = center->value(vals);
    return 2 * c  + -1 * f1; // = c - (f1-c)
}


