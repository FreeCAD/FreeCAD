#ifndef FREECAD_CONSTRAINTSOLVER_PYUTILS_H
#define FREECAD_CONSTRAINTSOLVER_PYUTILS_H

#ifndef BaseExport
    #define BaseExport
#endif

#include <CXX/Objects.hxx>
#include <Base/DualNumber.h>

#include <Base/Exception.h>
#include <Base/PyObjectBase.h>

#include <functional>


namespace FCS {

///converts std::vector (or whatever container) to Py::List. The elements must have getPyObject method.
template<class Vec>
inline Py::List asPyList(Vec& vec){ //vec is usually not changed... unless getPyObject changes the object, which does happen to some
    Py::List ret;
    for (auto &v : vec) {
        ret.append(Py::Object(v.getPyObject(), true));
    }
    return ret;
}

///converts std::vector of Py::Objects to Py::List.
template<class Vec>
inline Py::List asPyObjectList(Vec& vec){ //vec is usually not changed... unless getPyObject changes the object, which does happen to some
    Py::List ret;
    for (auto &v : vec) {
        ret.append(v.getHandledObject()); // Note: A better solution is needed here, as to define that this template is only enabled for pyhandled types
    }
    return ret;
}

///can convert vector of doubles to a py list of floats for example (use Py::Fload as PyCxxConstruct argument)
template<class Vec, class PyCxxConstruct>
inline Py::List asPyList(Vec& vec){ //vec is usually not changed... unless getPyObject changes the object, which does happen to some
    Py::List ret(vec.size());
    for (decltype(vec.size()) i = 0; i < vec.size(); ++i) {
        ret[i] = PyCxxConstruct(vec[i]);
    }
    return ret;
}

//temporary replacement for PyCXX's Object:::setAttr, that doesn't absorb the original error, for until PyCXX is uptated
inline void setAttr(Py::Object obj, std::string attrname, Py::Object value)
{
    if( PyObject_SetAttrString( obj.ptr(), const_cast<char*>( attrname.c_str() ), value.ptr() ) == -1 )
    {
        throw Py::Exception();
    }
}

template<class Ty>
inline Ty* pyTypeCheck(PyObject* obj)
{
    if (! PyObject_TypeCheck(obj, &Ty::Type)){
        std::stringstream ss;
        ss << "Expected " << Ty::Type.tp_name
           << " but got " << Py::Object(obj).type().as_string();
        throw Py::TypeError(ss.str());
    }
    return static_cast<Ty*>(obj);
};

///for use in PyMake
inline PyObject* raiseBaseException(Base::Exception& e)
{
    auto pye = e.getPyExceptionType();
    if(!pye)
        pye = Base::BaseExceptionFreeCADError;
    PyErr_SetObject(pye, e.getPyObject());
    return nullptr;
}

template <typename Enum>
inline Enum str2enum(Py::String value, const char* valueNames[]){
    std::string strvalue = Py::String(value);
    for (int i = 0; valueNames[i] != nullptr; ++i) {
        if (valueNames[i] == strvalue){
            return Enum(i);
        }
    }
    throw Py::ValueError("Not recognized: " + strvalue);
}

///a non-macro replacement for try {} PY_CATCH;
PyObject* pyTryCatch(std::function<Py::Object ()> body, PyObject* errorreturn = nullptr);

///imports a python module
Py::Module import(std::string modname);

///makes a dual number from a py dual number, a float, or an integer
Base::DualNumber asDualNumber(PyObject* ob);
inline Base::DualNumber asDualNumber(Py::Object ob) {return asDualNumber(ob.ptr());}

} //namespace

#endif
