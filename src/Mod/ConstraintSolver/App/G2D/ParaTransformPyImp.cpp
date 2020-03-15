#include "PreCompiled.h"

#include "G2D/ParaTransformPy.h"
#include "G2D/ParaTransformPy.cpp"

#include "PyUtils.h"

#include "G2D/ParaPlacementPy.h"
#include "ConstraintPy.h"

using namespace FCS;

PyObject* ParaTransformPy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{
    return pyTryCatch([&]()->Py::Object{
        {
            if (PyArg_ParseTuple(args, "")){
                HParaTransform p = new ParaTransform();
                if (kwd && kwd != Py_None)
                    p->initFromDict(Py::Dict(kwd));
                return p.getHandledObject();
            }
            PyErr_Clear();
        }
        {
            PyObject* pcfwchain = Py_None;
            PyObject* pcrevchain = Py_None;
            if (PyArg_ParseTuple(args, "O|O", &pcfwchain, &pcrevchain)){
                std::vector<HParaPlacement> fwvec, revvec;
                {
                    Py::Sequence fwlist(pcfwchain);
                    for (Py::Object it : fwlist){
                        if (! PyObject_TypeCheck(it.ptr(), &ParaPlacementPy::Type))
                            throw Py::TypeError("Must be ParaPlacement object, not " + it.type().repr().as_std_string());
                        fwvec.push_back(HParaPlacement(it));
                    }
                }
                if (pcrevchain != Py_None){
                    Py::Sequence rewlist(pcrevchain);
                    for (Py::Object it : rewlist){
                        if (! PyObject_TypeCheck(it.ptr(), &ParaPlacementPy::Type))
                            throw Py::TypeError("Must be ParaPlacement object, not " + it.type().repr().as_std_string());
                        revvec.push_back(HParaPlacement(it));
                    }
                }

                HParaTransform p = new ParaTransform(fwvec, revvec);
                if (kwd && kwd != Py_None)
                    p->initFromDict(Py::Dict(kwd));
                return p.getHandledObject();
            }
            PyErr_Clear();
        }

        throw Py::TypeError(
            "Wrong argument count or type."
            "\n\nsupported signatures:"
            "\n() - trivial transform"
            "\n(list_of_ParaPlacements, list_of_ParaPlacements, **keyword_args = {}) - trivial transform"
            "\n(**keyword_args) - assigns attributes."
        );
    });
}

// constructor method
int ParaTransformPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ParaTransformPy::representation(void) const
{
    return ParaObjectPy::representation();
}



Py::List ParaTransformPy::getFwChain(void) const
{
    return asPyObjectList(getParaTransformPtr()->fwchain());
}

void  ParaTransformPy::setFwChain(Py::List arg)
{
    std::vector<HParaPlacement> vec;
    for (Py::Object it : arg){
        if (! PyObject_TypeCheck(it.ptr(), &ParaPlacementPy::Type))
            throw Py::TypeError("Must be ParaPlacement object, not " + it.type().repr().as_std_string());
        vec.push_back(HParaPlacement(it));
    }
    getParaTransformPtr()->set(vec, getParaTransformPtr()->revchain());
}

Py::List ParaTransformPy::getRevChain(void) const
{
    return asPyObjectList(getParaTransformPtr()->revchain());
}

void  ParaTransformPy::setRevChain(Py::List arg)
{
    std::vector<HParaPlacement> vec;
    for (Py::Object it : arg){
        if (! PyObject_TypeCheck(it.ptr(), &ParaPlacementPy::Type))
            throw Py::TypeError("Must be ParaPlacement object, not " + it.type().repr().as_std_string());
        vec.push_back(HParaPlacement(it));
    }
    getParaTransformPtr()->set(getParaTransformPtr()->fwchain(), vec);
}




PyObject* ParaTransformPy::simplify(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    getParaTransformPtr()->simplify();
    return Py::new_reference_to(Py::None());
}

PyObject* ParaTransformPy::simplifyTransforms(PyObject* args)
{
    return pyTryCatch([&]()->Py::Object{
        PyObject* pclist;
        if (!PyArg_ParseTuple(args, "O", &pclist))
            throw Py::Exception();
        std::vector<HParaTransform> trss;
        for(Py::Object it : Py::Sequence(pclist)){
            if (!PyObject_TypeCheck(it.ptr(), &ParaTransformPy::Type))
                throw Py::TypeError("Must be ParaTransform object, not " + it.type().repr().as_std_string());
            trss.push_back(HParaTransform(it));

        }
        int ret = ParaTransform::simplifyTransforms(trss);
        return Py::Long(ret);
    });
}

PyObject* ParaTransformPy::simplifyTransformsOfConstraint(PyObject* args)
{
    return pyTryCatch([&]()->Py::Object{
        PyObject* pcconstr;
        if (!PyArg_ParseTuple(args, "O!", &ConstraintPy::Type ,&pcconstr))
            throw Py::Exception();
        int ret = ParaTransform::simplifyTransformsOfConstraint(*HConstraint(pcconstr, false));
        return Py::Long(ret);
    });
}



PyObject* ParaTransformPy::getCustomAttributes(const char* attr) const
{
    return ParaObjectPy::getCustomAttributes(attr);
}

int ParaTransformPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ParaObjectPy::setCustomAttributes(attr, obj);
}

