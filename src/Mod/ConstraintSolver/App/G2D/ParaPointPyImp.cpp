#include "PreCompiled.h"

#include "G2D/ParaPointPy.h"
#include "G2D/ParaPointPy.cpp"

#include "ParameterStorePy.h"
#include "ParameterRefPy.h"
#include "ValueSetPy.h"
#include "PyUtils.h"
#include "G2D/VectorPy.h"

#include <Base/Vector3D.h>
#include <Base/VectorPy.h>

using namespace FCS;

PyObject *ParaPointPy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{
    return pyTryCatch([&]()->Py::Object{
        {
            if (PyArg_ParseTuple(args, "")){
                HParaPoint p = (new ParaPoint)->getHandle<ParaPoint>();
                if (kwd && kwd != Py_None)
                    p->initFromDict(Py::Dict(kwd));
                return p.getHandledObject();
            }
            PyErr_Clear();
        }
        {
            PyObject* x;
            PyObject* y;
            if (PyArg_ParseTuple(args, "O!O!",&(ParameterRefPy::Type), &x, &(ParameterRefPy::Type), &y)){
                HParaPoint p = (new ParaPoint(*HParameterRef(x,false), *HParameterRef(y,false)))->getHandle<ParaPoint>();
                if (kwd && kwd != Py_None)
                    p->initFromDict(Py::Dict(kwd));
                return p.getHandledObject();
            }
            PyErr_Clear();
        }

        throw Py::TypeError(
            "Wrong argument count or type."
            "\n\nsupported signatures:"
            "\n() - all parameters set to null references"
            "\n(<ParameterRef object>, <ParameterRef object>, **keyword_args = {}) - assigns x and y parameter refs"
            "\n(**keyword_args) - assigns attributes."
        );

    });
}

// constructor method
int ParaPointPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ParaPointPy::representation(void) const
{
    return ParaGeometryPy::representation();
}


PyObject* ParaPointPy::value(PyObject *args)
{
    PyObject* pcvs = nullptr;
    if (!PyArg_ParseTuple(args, "O!",&(ValueSetPy::Type), &pcvs))
        return nullptr;
    getParaPointPtr()->throwIfIncomplete();
    return getParaPointPtr()->value(*HValueSet(pcvs, false)).getPyObject();
}

PyObject* ParaPointPy::setValue(PyObject *args)
{
    PyObject* pcvs = nullptr;
    PyObject* pcval = nullptr;
    if (!PyArg_ParseTuple(args, "O!O",&(ValueSetPy::Type), &pcvs, &pcval))
        return nullptr;
    Position pos;
    if (PyObject_TypeCheck(pcval, &VectorPy::Type)) {
        pos = Position(static_cast<VectorPy*>(pcval)->value);
    }
    else if (PyObject_TypeCheck(pcval, &Base::VectorPy::Type)) {
        Base::Vector3d v = *(static_cast<Base::VectorPy*>(pcval)->getVectorPtr());
        pos = Position(v.x, v.y);
    }
    else if (PySequence_Check(pcval)){
        Py::Sequence seq(pcval);
        if (seq.size() != 2)
            throw Py::ValueError("value is a sequence but not of two values");
        pos = Position(asDualNumber(seq[0]), asDualNumber(seq[1]));
    }
    else {
        throw Py::ValueError("new_value must be a tuple, an App.Vector or a G2D.Vector; got object of type '" + Py::Object(pcval).type().as_string() + "' instead");
    }
    getParaPointPtr()->setValue(*HValueSet(pcvs, false), pos);
    return Py::new_reference_to(Py::None());
}

PyObject *ParaPointPy::getCustomAttributes(const char* attr) const
{
    return ParaGeometryPy::getCustomAttributes(attr);
}

int ParaPointPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ParaGeometryPy::setCustomAttributes(attr, obj);
}

