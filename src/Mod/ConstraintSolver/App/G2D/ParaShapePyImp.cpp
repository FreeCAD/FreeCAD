#include "PreCompiled.h"

#include "G2D/ParaShapePy.h"
#include "G2D/ParaShapePy.cpp"

#include "ParaGeometryPy.h"
#include "G2D/ParaTransformPy.h"

#include "PyUtils.h"

using namespace FCS;

PyObject *ParaShapePy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{
    return pyTryCatch([&]()->Py::Object{
        {
            if (PyArg_ParseTuple(args, "")){
                HParaShapeBase p = (new ParaShapeBase)->getHandle<ParaShapeBase>();
                if (kwd && kwd != Py_None)
                    p->initFromDict(Py::Dict(kwd));
                return p.getHandledObject();
            }
            PyErr_Clear();
        }
        {
            PyObject* tshape;
            PyObject* transform = Py_None;
            if (PyArg_ParseTuple(args, "O!|O!",&(ParaGeometryPy::Type), &tshape, &(ParaTransformPy::Type), &transform)){
                HParaShapeBase p = new ParaShapeBase(HParaGeometry(tshape,false), HParaTransform(transform,false));
                if (kwd && kwd != Py_None)
                    p->initFromDict(Py::Dict(kwd));
                return p.getHandledObject();
            }
            PyErr_Clear();
        }

        throw Py::TypeError(
            "Wrong argument count or type."
            "\n\nsupported signatures:"
            "\n() - null shape"
            "\n(<ParaGeometry>, <ParaTransform> = None, **keyword_args = {}) - creates new shape with given (or empty) transform"
            "\n(**keyword_args) - assigns attributes."
        );
    });
}

// constructor method
int ParaShapePy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ParaShapePy::representation(void) const
{
    return ParaObjectPy::representation();
}



PyObject *ParaShapePy::getCustomAttributes(const char* attr) const
{
    return ParaObjectPy::getCustomAttributes(attr);
}

int ParaShapePy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ParaObjectPy::setCustomAttributes(attr, obj);
}

