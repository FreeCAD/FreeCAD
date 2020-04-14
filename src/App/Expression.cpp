/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifdef __GNUC__
# include <unistd.h>
#endif

#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdelete-non-virtual-dtor"
#endif

#include <boost/algorithm/string/predicate.hpp>

#include <Base/Console.h>
#include "Base/Exception.h"
#include <Base/Interpreter.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentPy.h>
#include <App/DocumentObject.h>
#include <App/PropertyUnits.h>
#include <Base/QuantityPy.h>
#include <Base/MatrixPy.h>
#include <Base/PlacementPy.h>
#include <Base/RotationPy.h>
#include <Base/VectorPy.h>
#include <QStringList>
#include <string>
#include <sstream>
#include <math.h>
#include <stdio.h>
#include <stack>
#include <deque>
#include <algorithm>
#include "ExpressionParser.h"
#include <Base/Unit.h>
#include <App/PropertyUnits.h>
#include <App/ObjectIdentifier.h>
#include <boost/math/special_functions/round.hpp>
#include <boost/math/special_functions/trunc.hpp>

/** \defgroup Expression Expressions framework
    \ingroup APP
    \brief The expression system allows users to write expressions and formulas that produce values
*/

using namespace Base;
using namespace App;

FC_LOG_LEVEL_INIT("Expression",true,true)

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif
#ifndef M_E
#define M_E        2.71828182845904523536
#endif
#ifndef  DOUBLE_MAX
# define DOUBLE_MAX 1.7976931348623157E+308    /* max decimal value of a "double"*/
#endif
#ifndef  DOUBLE_MIN
# define DOUBLE_MIN 2.2250738585072014E-308    /* min decimal value of a "double"*/
#endif

#if defined(_MSC_VER)
#define strtoll _strtoi64
#pragma warning(disable : 4003)
#pragma warning(disable : 4065)
#endif

#define __EXPR_THROW(_e,_msg,_expr) do {\
    std::ostringstream ss;\
    ss << _msg << (_expr);\
    throw _e(ss.str().c_str());\
}while(0)

#define _EXPR_THROW(_msg,_expr) __EXPR_THROW(ExpressionError,_msg,_expr)

#define __EXPR_SET_MSG(_e,_msg,_expr) do {\
    std::ostringstream ss;\
    ss << _msg << _e.what() << (_expr);\
    _e.setMessage(ss.str());\
}while(0)

#define _EXPR_RETHROW(_e,_msg,_expr) do {\
    __EXPR_SET_MSG(_e,_msg,_expr);\
    throw;\
}while(0)

#define _EXPR_PY_THROW(_msg,_expr) do {\
    Base::PyException _e;\
    __EXPR_SET_MSG(_e,_msg,_expr);\
    _e.raiseException();\
}while(0)

#define EXPR_PY_THROW(_expr) _EXPR_PY_THROW("",_expr)

#define EXPR_THROW(_msg) _EXPR_THROW(_msg,this)

#define RUNTIME_THROW(_msg) __EXPR_THROW(Base::RuntimeError,_msg, (Expression*)0)

#define TYPE_THROW(_msg) __EXPR_THROW(Base::TypeError,_msg, (Expression*)0)

#define PARSER_THROW(_msg) __EXPR_THROW(Base::ParserError,_msg, (Expression*)0)

#define PY_THROW(_msg) __EXPR_THROW(Py::RuntimeError,_msg, (Expression*)0)

static inline std::ostream &operator<<(std::ostream &os, const App::Expression *expr) {
    if(expr) {
        os << std::endl;
        expr->toString(os);
    }
    return os;
}

template<typename T>
void copy_vector(T &dst, const T& src) {
    dst.clear();
    dst.reserve(src.size());
    for(auto &s : src) {
        if(s)
            dst.push_back(s->copy());
        else
            dst.emplace_back();
    }
}

////////////////////////////////////////////////////////////////////////////////

// WARNING! The following define enables slightly faster any type comparison which
// is not standard conforming, and may break in some rare cases (although not likely)
//
// #define USE_FAST_ANY

static inline bool is_type(const App::any &value, const std::type_info& t) {
#ifdef USE_FAST_ANY
    return &value.type() == &t;
#else
    return value.type() == t;
#endif
}

template<class T>
static inline const T &cast(const App::any &value) {
#ifdef USE_FAST_ANY
    return *value.cast<T>();
#else
    return App::any_cast<const T&>(value);
#endif
}

template<class T>
static inline T &cast(App::any &value) {
#ifdef USE_FAST_ANY
    return *value.cast<T>();
#else
    return App::any_cast<T&>(value);
#endif
}

template<class T>
static inline T &&cast(App::any &&value) {
#ifdef USE_FAST_ANY
    return std::move(*value.cast<T>());
#else
    return App::any_cast<T&&>(std::move(value));
#endif
}

std::string unquote(const std::string & input)
{
    assert(input.size() >= 4);

    std::string output;
    std::string::const_iterator cur = input.begin() + 2;
    std::string::const_iterator end = input.end() - 2;

    output.reserve(input.size());

    bool escaped = false;
    while (cur != end) {
        if (escaped) {
            switch (*cur) {
            case 't':
                output += '\t';
                break;
            case 'n':
                output += '\n';
                break;
            case 'r':
                output += '\r';
                break;
            case '\\':
                output += '\\';
                break;
            case '\'':
                output += '\'';
                break;
            case '"':
                output += '"';
                break;
            }
            escaped = false;
        }
        else {
            if (*cur == '\\')
                escaped = true;
            else
                output += *cur;
        }
        ++cur;
    }

    return output;
}

////////////////////////////////////////////////////////////////////////////////////
//
// ExpressionVistor
//
void ExpressionVisitor::getDeps(Expression &e, ExpressionDeps &deps) {
    e._getDeps(deps);
}

void ExpressionVisitor::getDepObjects(Expression &e,
        std::set<App::DocumentObject*> &deps, std::vector<std::string> *labels)
{
    e._getDepObjects(deps,labels);
}

void ExpressionVisitor::getIdentifiers(Expression &e, std::set<App::ObjectIdentifier> &ids) {
    e._getIdentifiers(ids);
}

bool ExpressionVisitor::adjustLinks(Expression &e, const std::set<App::DocumentObject*> &inList) {
    return e._adjustLinks(inList,*this);
}

void ExpressionVisitor::importSubNames(Expression &e, const ObjectIdentifier::SubNameMap &subNameMap) {
    e._importSubNames(subNameMap);
}

void ExpressionVisitor::updateLabelReference(Expression &e,
        DocumentObject *obj, const std::string &ref, const char *newLabel)
{
    e._updateLabelReference(obj,ref,newLabel);
}

bool ExpressionVisitor::updateElementReference(Expression &e, App::DocumentObject *feature, bool reverse) {
    return e._updateElementReference(feature,reverse,*this);
}

bool ExpressionVisitor::relabeledDocument(
        Expression &e, const std::string &oldName, const std::string &newName)
{
    return e._relabeledDocument(oldName,newName,*this);
}

bool ExpressionVisitor::renameObjectIdentifier(Expression &e,
        const std::map<ObjectIdentifier,ObjectIdentifier> &paths, const ObjectIdentifier &path)
{
    return e._renameObjectIdentifier(paths,path,*this);
}

void ExpressionVisitor::collectReplacement(Expression &e,
        std::map<ObjectIdentifier,ObjectIdentifier> &paths,
        const App::DocumentObject *parent, App::DocumentObject *oldObj, App::DocumentObject *newObj) const
{
    return e._collectReplacement(paths,parent,oldObj,newObj);
}

void ExpressionVisitor::moveCells(Expression &e, const CellAddress &address, int rowCount, int colCount) {
    e._moveCells(address,rowCount,colCount,*this);
}

void ExpressionVisitor::offsetCells(Expression &e, int rowOffset, int colOffset) {
    e._offsetCells(rowOffset,colOffset,*this);
}

/////////////////////////////////////////////////////////////////////////////////////
// Helper functions

/* The following definitions are from The art of computer programming by Knuth
 * (copied from http://stackoverflow.com/questions/17333/most-effective-way-for-float-and-double-comparison)
 */

/*
static bool approximatelyEqual(double a, double b, double epsilon)
{
    return fabs(a - b) <= ( (fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}
*/

template<class T>
static inline bool essentiallyEqual(T a, T b)
{
    static const T _epsilon = std::numeric_limits<T>::epsilon();
    return std::fabs(a - b) <= ( (std::fabs(a) > std::fabs(b) ? std::fabs(b) : std::fabs(a)) * _epsilon);
}

template<class T>
inline bool essentiallyZero(T a) {
    return !a;
}

template<>
inline bool essentiallyZero(double a) {
    return essentiallyEqual(a, 0.0);
}

template<>
inline bool essentiallyZero(float a) {
    return essentiallyEqual(a, 0.0f);
}

template<class T>
static inline bool definitelyGreaterThan(T a, T b)
{
    static const T _epsilon = std::numeric_limits<T>::epsilon();
    return (a - b) > ( (std::fabs(a) < std::fabs(b) ? std::fabs(b) : std::fabs(a)) * _epsilon);
}

template<class T>
static inline bool definitelyLessThan(T a, T b)
{
    static const T _epsilon = std::numeric_limits<T>::epsilon();
    return (b - a) > ( (std::fabs(a) < std::fabs(b) ? std::fabs(b) : std::fabs(a)) * _epsilon);
}

static inline int essentiallyInteger(double a, long &l, int &i) {
    double intpart;
    if(std::modf(a,&intpart) == 0.0) {
        if(intpart<0.0) {
            if(intpart >= INT_MIN) {
                i = (int)intpart;
                l = i;
                return 1;
            }
            if(intpart >= LONG_MIN) {
                l = (long)intpart;
                return 2;
            }
        }else if(intpart <= INT_MAX) {
            i = (int)intpart;
            l = i;
            return 1;
        }else if(intpart <= LONG_MAX) {
            l = (int)intpart;
            return 2;
        }
    }
    return 0;
}

static inline bool essentiallyInteger(double a, long &l) {
    double intpart;
    if(std::modf(a,&intpart) == 0.0) {
        if(intpart<0.0) {
            if(intpart >= LONG_MIN) {
                l = (long)intpart;
                return true;
            }
        }else if(intpart <= LONG_MAX) {
            l = (long)intpart;
            return true;
        }
    }
    return false;
}

// This class is intended to be contained inside App::any (via a shared_ptr)
// without holding Python global lock
struct PyObjectWrapper {
public:
    typedef std::shared_ptr<PyObjectWrapper> Pointer;

    PyObjectWrapper(PyObject *obj):pyobj(obj) {
        Py::_XINCREF(pyobj);
    }
    ~PyObjectWrapper() {
        if(pyobj) {
            Base::PyGILStateLocker lock;
            Py::_XDECREF(pyobj);
        }
    }
    PyObjectWrapper(const PyObjectWrapper &) = delete;
    PyObjectWrapper &operator=(const PyObjectWrapper &) = delete;

    Py::Object get() const {
        if(!pyobj)
            return Py::Object();
        return Py::Object(const_cast<PyObject*>(pyobj));
    }

private:
    PyObject *pyobj;
};

static inline PyObjectWrapper::Pointer pyObjectWrap(PyObject *obj) {
    return std::make_shared<PyObjectWrapper>(obj);
}

static inline bool isAnyPyObject(const App::any &value) {
    return is_type(value,typeid(PyObjectWrapper::Pointer));
}

static inline Py::Object __pyObjectFromAny(const App::any &value) {
    return cast<PyObjectWrapper::Pointer>(value)->get();
}

static Py::Object _pyObjectFromAny(const App::any &value, const Expression *e) {
    if(value.empty())
        return Py::Object();
    else if (isAnyPyObject(value))
        return __pyObjectFromAny(value);
    if (is_type(value,typeid(Quantity)))
        return Py::asObject(new QuantityPy(new Quantity(cast<Quantity>(value))));
    else if (is_type(value,typeid(double)))
        return Py::Float(cast<double>(value));
    else if (is_type(value,typeid(float)))
        return Py::Float(cast<float>(value));
    else if (is_type(value,typeid(int)))
#if PY_MAJOR_VERSION < 3
        return Py::Int(cast<int>(value));
#else
        return Py::Long(cast<int>(value));
#endif
    else if (is_type(value,typeid(long))) {
#if PY_MAJOR_VERSION < 3
        long l = cast<long>(value);
        if(std::abs(l)<=INT_MAX)
            return Py::Int(int(l));
#endif
        return Py::Long(cast<long>(value));
    } else if (is_type(value,typeid(bool)))
        return Py::Boolean(cast<bool>(value));
    else if (is_type(value,typeid(std::string)))
        return Py::String(cast<string>(value));
    else if (is_type(value,typeid(const char*)))
        return Py::String(cast<const char*>(value));

    _EXPR_THROW("Unknown type", e);
}

namespace App {
Py::Object pyObjectFromAny(const App::any &value) {
    return _pyObjectFromAny(value,0);
}

App::any pyObjectToAny(Py::Object value, bool check) {

    if(value.isNone())
        return App::any();

    PyObject *pyvalue = value.ptr();

    if(!check)
        return App::any(pyObjectWrap(pyvalue));

    if (PyObject_TypeCheck(pyvalue, &Base::QuantityPy::Type)) {
        Base::QuantityPy * qp = static_cast<Base::QuantityPy*>(pyvalue);
        Base::Quantity * q = qp->getQuantityPtr();

        return App::any(*q);
    }
    if (PyFloat_Check(pyvalue))
        return App::any(PyFloat_AsDouble(pyvalue));
#if PY_MAJOR_VERSION < 3
    if (PyInt_Check(pyvalue))
        return App::any(PyInt_AsLong(pyvalue));
#endif
    if (PyLong_Check(pyvalue))
        return App::any(PyLong_AsLong(pyvalue));
#if PY_MAJOR_VERSION < 3
    else if (PyString_Check(pyvalue))
        return App::any(std::string(PyString_AsString(pyvalue)));
    else if (PyUnicode_Check(pyvalue)) {
        PyObject * s = PyUnicode_AsUTF8String(pyvalue);
        if(!s)
            FC_THROWM(Base::ValueError,"Invalid unicode string");
        Py::Object o(s,true);
        return App::any(std::string(PyString_AsString(s)));
    }
#else
    else if (PyUnicode_Check(pyvalue)) {
        const char* value = PyUnicode_AsUTF8(pyvalue);
        if (!value) {
            FC_THROWM(Base::ValueError, "Invalid unicode string");
        }
        return App::any(std::string(value));
    }
#endif
    else {
        return App::any(pyObjectWrap(pyvalue));
    }
}

bool pyToQuantity(Quantity &q, const Py::Object &pyobj) {
    if (PyObject_TypeCheck(*pyobj, &Base::QuantityPy::Type))
        q = *static_cast<Base::QuantityPy*>(*pyobj)->getQuantityPtr();
    else if (PyFloat_Check(*pyobj))
        q = Quantity(PyFloat_AsDouble(*pyobj));
#if PY_MAJOR_VERSION < 3
    else if (PyInt_Check(*pyobj))
        q = Quantity(PyInt_AsLong(*pyobj));
#endif
    else if (PyLong_Check(*pyobj))
        q = Quantity(PyLong_AsLong(*pyobj));
    else
        return false;
    return true;
}

static inline Quantity pyToQuantity(const Py::Object &pyobj,
        const Expression *e, const char *msg=0)
{
    Quantity q;
    if(!pyToQuantity(q,pyobj)) {
        if(!msg)
            msg = "Failed to convert to Quantity.";
        __EXPR_THROW(TypeError,msg,e);
    }
    return q;
}

Py::Object pyFromQuantity(const Quantity &quantity) {
    if(!quantity.getUnit().isEmpty())
        return Py::asObject(new QuantityPy(new Quantity(quantity)));
    double v = quantity.getValue();
    long l;
    int i;
    switch(essentiallyInteger(v,l,i)) {
    case 1:
#if PY_MAJOR_VERSION < 3
        return Py::Int(i);
#endif
    case 2:
        return Py::Long(l);
    default:
        return Py::Float(v);
    }
}

Quantity anyToQuantity(const App::any &value, const char *msg) {
    if (is_type(value,typeid(Quantity))) {
        return cast<Quantity>(value);
    } else if (is_type(value,typeid(bool))) {
        return Quantity(cast<bool>(value)?1.0:0.0);
    } else if (is_type(value,typeid(int))) {
        return Quantity(cast<int>(value));
    } else if (is_type(value,typeid(long))) {
        return Quantity(cast<long>(value));
    } else if (is_type(value,typeid(float))) {
        return Quantity(cast<float>(value));
    } else if (is_type(value,typeid(double))) {
        return Quantity(cast<double>(value));
    }
    if(!msg)
        msg = "Failed to convert to Quantity";
    TYPE_THROW(msg);
}

static inline bool anyToLong(long &res, const App::any &value) {
    if (is_type(value,typeid(int))) {
        res = cast<int>(value);
    } else if (is_type(value,typeid(long))) {
        res = cast<long>(value);
    } else if (is_type(value,typeid(bool)))
        res = cast<bool>(value)?1:0;
    else
        return false;
    return true;
}

static inline bool anyToDouble(double &res, const App::any &value) {
    if (is_type(value,typeid(double)))
        res = cast<double>(value);
    else if (is_type(value,typeid(float)))
        res = cast<float>(value);
    else if (is_type(value,typeid(long)))
        res = cast<long>(value);
    else if (is_type(value,typeid(int)))
        res = cast<int>(value);
    else if (is_type(value,typeid(bool)))
        res = cast<bool>(value)?1:0;
    else
        return false;
    return true;
}

bool isAnyEqual(const App::any &v1, const App::any &v2) {
    if(v1.empty())
        return v2.empty();
    else if(v2.empty())
        return false;

    if(!is_type(v1,v2.type())) {
        if(is_type(v1,typeid(Quantity)))
            return cast<Quantity>(v1) == anyToQuantity(v2);
        else if(is_type(v2,typeid(Quantity)))
            return anyToQuantity(v1) == cast<Quantity>(v2);

        long l1,l2;
        double d1,d2;
        if(anyToLong(l1,v1)) {
            if(anyToLong(l2,v2))
                return l1==l2;
            else if(anyToDouble(d2,v2))
                return essentiallyEqual((double)l1,d2);
            else
                return false;
        }else if(anyToDouble(d1,v1))
           return anyToDouble(d2,v2) && essentiallyEqual(d1,d2);

        if(is_type(v1,typeid(std::string))) {
            if(is_type(v2,typeid(const char*))) {
                auto c = cast<const char*>(v2);
                return c && cast<std::string>(v1)==c;
            }
            return false;
        }else if(is_type(v1,typeid(const char*))) {
            if(is_type(v2,typeid(std::string))) {
                auto c = cast<const char*>(v1);
                return c && cast<std::string>(v2)==c;
            }
            return false;
        }
    }

    if (is_type(v1,typeid(int)))
        return cast<int>(v1) == cast<int>(v2);
    if (is_type(v1,typeid(long)))
        return cast<long>(v1) == cast<long>(v2);
    if (is_type(v1,typeid(std::string)))
        return cast<std::string>(v1) == cast<std::string>(v2);
    if (is_type(v1,typeid(const char*))) {
        auto c1 = cast<const char*>(v1);
        auto c2 = cast<const char*>(v2);
        return c1==c2 || (c1 && c2 && strcmp(c1,c2)==0);
    }
    if (is_type(v1,typeid(bool)))
        return cast<bool>(v1) == cast<bool>(v2);
    if (is_type(v1,typeid(double)))
        return essentiallyEqual(cast<double>(v1), cast<double>(v2));
    if (is_type(v1,typeid(float)))
        return essentiallyEqual(cast<float>(v1), cast<float>(v2));

    if (is_type(v1,typeid(Quantity)))
        return cast<Quantity>(v1) == cast<Quantity>(v2);

    if (!isAnyPyObject(v1))
        throw Base::TypeError("Unknown type");

    Base::PyGILStateLocker lock;
    Py::Object o1 = __pyObjectFromAny(v1);
    Py::Object o2 = __pyObjectFromAny(v2);
    if(!o1.isType(o2.type()))
        return false;
    int res = PyObject_RichCompareBool(o1.ptr(),o2.ptr(),Py_EQ);
    if(res<0)
        PyException::ThrowException();
    return !!res;
}

Expression* expressionFromPy(const DocumentObject *owner, const Py::Object &value) {
    if (value.isNone())
        return new PyObjectExpression(owner);
    if(value.isString()) {
        return new StringExpression(owner,value.as_string());
    } else if (PyObject_TypeCheck(value.ptr(),&QuantityPy::Type)) {
        return new NumberExpression(owner,
                *static_cast<QuantityPy*>(value.ptr())->getQuantityPtr());
    } else if (value.isBoolean()) {
        if(value.isTrue())
            return new ConstantExpression(owner,"True",Quantity(1.0));
        else
            return new ConstantExpression(owner,"False",Quantity(0.0));
    } else {
        Quantity q;
        if(pyToQuantity(q,value))
            return new NumberExpression(owner,q);
    }
    return new PyObjectExpression(owner,value.ptr());
}

} // namespace App

//
// Expression component
//
Expression::Component::Component(const std::string &n)
    :comp(ObjectIdentifier::SimpleComponent(n))
    ,e1(0) ,e2(0) ,e3(0)
{}

Expression::Component::Component(Expression *_e1, Expression *_e2, Expression *_e3, bool isRange)
    :e1(_e1) ,e2(_e2) ,e3(_e3)
{
    if(isRange || e2 || e3)
        comp = ObjectIdentifier::RangeComponent(0);
}

Expression::Component::Component(const ObjectIdentifier::Component &comp)
    :comp(comp)
    ,e1(0) ,e2(0) ,e3(0)
{}

Expression::Component::Component(const Component &other)
    :comp(other.comp)
    ,e1(other.e1?other.e1->copy():0)
    ,e2(other.e2?other.e2->copy():0)
    ,e3(other.e3?other.e3->copy():0)
{}

Expression::Component::~Component()
{
    delete e1;
    delete e2;
    delete e3;
}

Expression::Component* Expression::Component::copy() const {
    return new Component(*this);
}

Expression::Component* Expression::Component::eval() const {
    auto res = new Component(comp);
    if(e1) res->e1 = e1->eval();
    if(e2) res->e2 = e2->eval();
    if(e3) res->e3 = e3->eval();
    return res;
}

Py::Object Expression::Component::get(const Expression *owner, const Py::Object &pyobj) const {
    try {
        if(!e1 && !e2 && !e3)
            return comp.get(pyobj);
        if(!comp.isRange() && !e2 && !e3) {
            auto index = e1->getPyValue();
            Py::Object res;
            if(pyobj.isMapping())
                res = Py::Mapping(pyobj).getItem(index);
            else {
                Py_ssize_t i = PyNumber_AsSsize_t(index.ptr(), PyExc_IndexError);
                if(PyErr_Occurred())
                    throw Py::Exception();
                res = Py::Sequence(pyobj).getItem(i);
            }
            if(!res.ptr())
                throw Py::Exception();
            return res;
        }else{
            Py::Object v1,v2,v3;
            if(e1) v1 = e1->getPyValue();
            if(e2) v2 = e2->getPyValue();
            if(e3) v3 = e3->getPyValue();
            PyObject *s = PySlice_New(e1?v1.ptr():nullptr,
                                      e2?v2.ptr():nullptr,
                                      e3?v3.ptr():nullptr);
            if(!s)
                throw Py::Exception();
            Py::Object slice(s,true);
            PyObject *res = PyObject_GetItem(pyobj.ptr(),slice.ptr());
            if(!res)
                throw Py::Exception();
            return Py::asObject(res);
        }
    }catch(Py::Exception &) {
        EXPR_PY_THROW(owner);
    }
    return Py::Object();
}

void Expression::Component::set(const Expression *owner, Py::Object &pyobj, const Py::Object &value) const
{
    if(!e1 && !e2 && !e3)
        return comp.set(pyobj,value);
    try {
        if(!comp.isRange() && !e2 && !e3) {
            auto index = e1->getPyValue();
            if(pyobj.isMapping())
                Py::Mapping(pyobj).setItem(index,value);
            else {
                Py_ssize_t i = PyNumber_AsSsize_t(pyobj.ptr(), PyExc_IndexError);
                if(PyErr_Occurred() || PySequence_SetItem(pyobj.ptr(),i,value.ptr())==-1)
                    throw Py::Exception();
            }
        }else{
            Py::Object v1,v2,v3;
            if(e1) v1 = e1->getPyValue();
            if(e2) v2 = e2->getPyValue();
            if(e3) v3 = e3->getPyValue();
            PyObject *s = PySlice_New(e1?v1.ptr():nullptr,
                                      e2?v2.ptr():nullptr,
                                      e3?v3.ptr():nullptr);
            if(!s)
                throw Py::Exception();
            Py::Object slice(s,true);
            if(PyObject_SetItem(pyobj.ptr(),slice.ptr(),value.ptr())<0)
                throw Py::Exception();
        }
    }catch(Py::Exception &) {
        EXPR_PY_THROW(owner);
    }
}

void Expression::Component::del(const Expression *owner, Py::Object &pyobj) const {
    try {
        if(!e1 && !e2 && !e3) {
            comp.del(pyobj);
        } if(!comp.isRange() && !e2 && !e3) {
            auto index = e1->getPyValue();
            if(pyobj.isMapping())
                Py::Mapping(pyobj).delItem(index);
            else {
                Py_ssize_t i = PyNumber_AsSsize_t(pyobj.ptr(), PyExc_IndexError);
                if(PyErr_Occurred() || PySequence_DelItem(pyobj.ptr(),i)==-1)
                    throw Py::Exception();
            }
        }else{
            Py::Object v1,v2,v3;
            if(e1) v1 = e1->getPyValue();
            if(e2) v2 = e2->getPyValue();
            if(e3) v3 = e3->getPyValue();
            PyObject *s = PySlice_New(e1?v1.ptr():nullptr,
                                      e2?v2.ptr():nullptr,
                                      e3?v3.ptr():nullptr);
            if(!s)
                throw Py::Exception();
            Py::Object slice(s,true);
            if(PyObject_DelItem(pyobj.ptr(),slice.ptr())<0)
                throw Py::Exception();
        }
    }catch(Py::Exception &) {
        EXPR_PY_THROW(owner);
    }
}

void Expression::Component::visit(ExpressionVisitor &v) {
    if(e1) e1->visit(v);
    if(e2) e2->visit(v);
    if(e3) e3->visit(v);
}

bool Expression::Component::isTouched() const {
    return (e1&&e1->isTouched()) ||
            (e2&&e2->isTouched()) ||
            (e3&&e3->isTouched());
}

void Expression::Component::toString(std::ostream &ss, bool persistent) const {
    if(!e1 && !e2 && !e3) {
        if(comp.isSimple())
            ss << '.';
        comp.toString(ss,!persistent);
        return;
    }
    ss << '[';
    if(e1)
        e1->toString(ss,persistent);
    if(e2 || comp.isRange())
        ss << ':';
    if(e2)
        e2->toString(ss,persistent);
    if(e3) {
        ss << ':';
        e3->toString(ss,persistent);
    }
    ss << ']';
}


//
// Expression base-class
//

TYPESYSTEM_SOURCE_ABSTRACT(App::Expression, Base::BaseClass)

Expression::Expression(const DocumentObject *_owner)
    : owner(const_cast<App::DocumentObject*>(_owner))
{

}

Expression::~Expression()
{
    for(auto c : components)
        delete c;
}

Expression::Component* Expression::createComponent(const std::string &n) {
    return new Component(n);
}

Expression::Component* Expression::createComponent(
        Expression* e1, Expression* e2, Expression* e3, bool isRange)
{
    return new Component(e1,e2,e3,isRange);
}


int Expression::priority() const {
    return 20;
}

Expression * Expression::parse(const DocumentObject *owner, const std::string &buffer)
{
    return ExpressionParser::parse(owner, buffer.c_str());
}

class GetDepsExpressionVisitor : public ExpressionVisitor {
public:
    GetDepsExpressionVisitor(ExpressionDeps &deps)
        :deps(deps)
    {}

    virtual void visit(Expression &e) {
        this->getDeps(e,deps);
    }

    ExpressionDeps &deps;
};

void Expression::getDeps(ExpressionDeps &deps)  const {
    GetDepsExpressionVisitor v(deps);
    const_cast<Expression*>(this)->visit(v);
}

ExpressionDeps Expression::getDeps()  const {
    ExpressionDeps deps;
    getDeps(deps);
    return deps;
}

class GetDepObjsExpressionVisitor : public ExpressionVisitor {
public:
    GetDepObjsExpressionVisitor(std::set<App::DocumentObject*> &deps, std::vector<std::string> *labels)
        :deps(deps),labels(labels)
    {}

    virtual void visit(Expression &e) {
        this->getDepObjects(e,deps,labels);
    }

    std::set<App::DocumentObject*> &deps;
    std::vector<std::string> *labels;
};

void Expression::getDepObjects(std::set<App::DocumentObject*> &deps, std::vector<std::string> *labels)  const {
    GetDepObjsExpressionVisitor v(deps,labels);
    const_cast<Expression *>(this)->visit(v);
}

std::set<App::DocumentObject*> Expression::getDepObjects(std::vector<std::string> *labels)  const {
    std::set<App::DocumentObject*> deps;
    getDepObjects(deps,labels);
    return deps;
}

class GetIdentifiersExpressionVisitor : public ExpressionVisitor {
public:
    GetIdentifiersExpressionVisitor(std::set<App::ObjectIdentifier> &deps)
        :deps(deps)
    {}

    virtual void visit(Expression &e) {
        this->getIdentifiers(e,deps);
    }

    std::set<App::ObjectIdentifier> &deps;
};

void Expression::getIdentifiers(std::set<App::ObjectIdentifier> &deps)  const {
    GetIdentifiersExpressionVisitor v(deps);
    const_cast<Expression*>(this)->visit(v);
}

std::set<App::ObjectIdentifier> Expression::getIdentifiers()  const {
    std::set<App::ObjectIdentifier> deps;
    getIdentifiers(deps);
    return deps;
}

class AdjustLinksExpressionVisitor : public ExpressionVisitor {
public:
    AdjustLinksExpressionVisitor(const std::set<App::DocumentObject*> &inList)
        :inList(inList),res(false)
    {}

    virtual void visit(Expression &e) {
        if(this->adjustLinks(e,inList))
            res = true;
    }

    const std::set<App::DocumentObject*> &inList;
    bool res;
};

bool Expression::adjustLinks(const std::set<App::DocumentObject*> &inList) {
    AdjustLinksExpressionVisitor v(inList);
    visit(v);
    return v.res;
}

class ImportSubNamesExpressionVisitor : public ExpressionVisitor {
public:
    ImportSubNamesExpressionVisitor(const ObjectIdentifier::SubNameMap &subNameMap)
        :subNameMap(subNameMap)
    {}

    virtual void visit(Expression &e) {
        this->importSubNames(e,subNameMap);
    }

    const ObjectIdentifier::SubNameMap &subNameMap;
};

ExpressionPtr Expression::importSubNames(const std::map<std::string,std::string> &nameMap) const {
    if(!owner || !owner->getDocument())
        return 0;
    ObjectIdentifier::SubNameMap subNameMap;
    for(auto &dep : getDeps()) {
        for(auto &info : dep.second) {
            for(auto &path : info.second) {
                auto obj = path.getDocumentObject();
                if(!obj)
                    continue;
                auto it = nameMap.find(obj->getExportName(true));
                if(it!=nameMap.end())
                    subNameMap.emplace(std::make_pair(obj,std::string()),it->second);
                auto key = std::make_pair(obj,path.getSubObjectName());
                if(key.second.empty() || subNameMap.count(key))
                    continue;
                std::string imported = PropertyLinkBase::tryImportSubName(
                               obj,key.second.c_str(),owner->getDocument(), nameMap);
                if(imported.size())
                    subNameMap.emplace(std::move(key),std::move(imported));
            }
        }
    }
    if(subNameMap.empty())
        return 0;
    ImportSubNamesExpressionVisitor v(subNameMap);
    auto res = copy();
    res->visit(v);
    return ExpressionPtr(res);
}

class UpdateLabelExpressionVisitor : public ExpressionVisitor {
public:
    UpdateLabelExpressionVisitor(App::DocumentObject *obj, const std::string &ref, const char *newLabel)
        :obj(obj),ref(ref),newLabel(newLabel)
    {}

    virtual void visit(Expression &e) {
        this->updateLabelReference(e,obj,ref,newLabel);
    }

    App::DocumentObject *obj;
    const std::string &ref;
    const char *newLabel;
};

ExpressionPtr Expression::updateLabelReference(
        App::DocumentObject *obj, const std::string &ref, const char *newLabel) const
{
    if(ref.size()<=2)
        return ExpressionPtr();
    std::vector<std::string> labels;
    getDepObjects(&labels);
    for(auto &label : labels) {
        // ref contains something like $label. and we need to strip '$' and '.'
        if(ref.compare(1,ref.size()-2,label)==0) {
            UpdateLabelExpressionVisitor v(obj,ref,newLabel);
            auto expr = copy();
            expr->visit(v);
            return ExpressionPtr(expr);
        }
    }
    return ExpressionPtr();
}

class ReplaceObjectExpressionVisitor : public ExpressionVisitor {
public:
    ReplaceObjectExpressionVisitor(const DocumentObject *parent,
            DocumentObject *oldObj, DocumentObject *newObj)
        : parent(parent),oldObj(oldObj),newObj(newObj)
    {
    }

    void visit(Expression &e) {
        if(collect)
            this->collectReplacement(e,paths,parent,oldObj,newObj);
        else
            this->renameObjectIdentifier(e,paths,dummy);
    }

    const DocumentObject *parent;
    DocumentObject *oldObj;
    DocumentObject *newObj;
    ObjectIdentifier dummy;
    std::map<ObjectIdentifier, ObjectIdentifier> paths;
    bool collect = true;
};

ExpressionPtr Expression::replaceObject(const DocumentObject *parent,
        DocumentObject *oldObj, DocumentObject *newObj) const
{
    ReplaceObjectExpressionVisitor v(parent,oldObj,newObj);

    // First pass, collect any changes. We have to const_cast it, as visit() is
    // not const. This is ugly...
    const_cast<Expression*>(this)->visit(v);

    if(v.paths.empty())
        return ExpressionPtr();

    // Now make a copy and do the actual replacement
    auto expr = copy();
    v.collect = false;
    expr->visit(v);
    return ExpressionPtr(expr);
}

App::any Expression::getValueAsAny() const {
    Base::PyGILStateLocker lock;
    return pyObjectToAny(getPyValue());
}

Py::Object Expression::getPyValue() const {
    try {
        Py::Object pyobj = _getPyValue();
        if(components.size()) {
            for(auto &c : components)
                pyobj = c->get(this,pyobj);
        }
        return pyobj;
    }catch(Py::Exception &) {
        EXPR_PY_THROW(this);
    }
    return Py::Object();
}

void Expression::addComponent(Component *component) {
    assert(component);
    components.push_back(component);
}

void Expression::visit(ExpressionVisitor &v) {
    _visit(v);
    for(auto &c : components)
        c->visit(v);
    v.visit(*this);
}

Expression* Expression::eval() const {
    Base::PyGILStateLocker lock;
    return expressionFromPy(owner,getPyValue());
}

bool Expression::isSame(const Expression &other) const {
    if(&other == this)
        return true;
    if(getTypeId()!=other.getTypeId())
        return false;
    return comment==other.comment && toString(true,true) == other.toString(true,true);
}

std::string Expression::toString(bool persistent, bool checkPriority, int indent) const {
    std::ostringstream ss;
    toString(ss,persistent,checkPriority,indent);
    return ss.str();
}

void Expression::toString(std::ostream &ss, bool persistent, bool checkPriority, int indent) const {
    if(components.empty()) {
        bool needsParens = checkPriority && priority()<20;
        if(needsParens)
            ss << '(';
        _toString(ss,persistent,indent);
        if(needsParens)
            ss << ')';
        return;
    }
    if(!_isIndexable()) {
        ss << '(';
        _toString(ss,persistent,indent);
        ss << ')';
    }else
        _toString(ss,persistent,indent);
    for(auto &c : components)
        c->toString(ss,persistent);
}

Expression* Expression::copy() const {
    auto expr = _copy();
    copy_vector(expr->components,components);
    expr->comment = comment;
    return expr;
}


//
// UnitExpression class
//

TYPESYSTEM_SOURCE(App::UnitExpression, App::Expression)

UnitExpression::UnitExpression(const DocumentObject *_owner, const Base::Quantity & _quantity, const std::string &_unitStr)
    : Expression(_owner)
    , quantity(_quantity)
    , unitStr(_unitStr)
{
}

UnitExpression::~UnitExpression() {
    if(cache) {
        Base::PyGILStateLocker lock;
        Py::_XDECREF(cache);
    }
}

void UnitExpression::setQuantity(const Quantity &_quantity)
{
    quantity = _quantity;
    if(cache) {
        Base::PyGILStateLocker lock;
        Py::_XDECREF(cache);
        cache = 0;
    }
}

/**
  * Set unit information.
  *
  * @param _unit    A unit object
  * @param _unitstr The unit expressed as a string
  * @param _scaler  Scale factor to convert unit into internal unit.
  */

void UnitExpression::setUnit(const Quantity &_quantity)
{
    quantity = _quantity;
    if(cache) {
        Base::PyGILStateLocker lock;
        Py::_XDECREF(cache);
        cache = 0;
    }
}

/**
  * Simplify the expression. In this case, a NumberExpression is returned,
  * as it cannot be simplified any more.
  */

Expression *UnitExpression::simplify() const
{
    return new NumberExpression(owner, quantity);
}

/**
  * Return a string representation, in this case the unit string.
  */

/**
  * Return a string representation of the expression.
  */

void UnitExpression::_toString(std::ostream &ss, bool,int) const
{
    ss << unitStr;
}

/**
  * Return a copy of the expression.
  */

Expression *UnitExpression::_copy() const
{
    return new UnitExpression(owner, quantity, unitStr);
}

Py::Object UnitExpression::_getPyValue() const {
    if(!cache)
        cache = Py::new_reference_to(pyFromQuantity(quantity));
    return Py::Object(cache);
}

//
// NumberExpression class
//

TYPESYSTEM_SOURCE(App::NumberExpression, App::Expression)

NumberExpression::NumberExpression(const DocumentObject *_owner, const Quantity &_quantity)
    : UnitExpression(_owner, _quantity)
{
}

/**
  * Simplify the expression. For NumberExpressions, we return a copy(), as it cannot
  * be simplified any more.
  */

Expression *NumberExpression::simplify() const
{
    return copy();
}

/**
  * Create and return a copy of the expression.
  */

Expression *NumberExpression::_copy() const
{
    return new NumberExpression(owner, getQuantity());
}

/**
  * Negate the stored value.
  */

void NumberExpression::negate()
{
    setQuantity(-getQuantity());
}

void NumberExpression::_toString(std::ostream &ss, bool,int) const
{
    // Restore the old implementation because using digits10 + 2 causes
    // undesired side-effects:
    // https://forum.freecadweb.org/viewtopic.php?f=3&t=44057&p=375882#p375882
    // See also:
    // https://en.cppreference.com/w/cpp/types/numeric_limits/digits10
    // https://en.cppreference.com/w/cpp/types/numeric_limits/max_digits10
    // https://www.boost.org/doc/libs/1_63_0/libs/multiprecision/doc/html/boost_multiprecision/tut/limits/constants.html
    ss << std::setprecision(std::numeric_limits<double>::digits10 + 1) << getValue();

    /* Trim of any extra spaces */
    //while (s.size() > 0 && s[s.size() - 1] == ' ')
//        s.erase(s.size() - 1);
}

bool NumberExpression::isInteger(long *l) const {
    long _l;
    if(!l)
        l = &_l;
    return essentiallyInteger(getValue(),*l);
}

//
// OperatorExpression class
//

TYPESYSTEM_SOURCE(App::OperatorExpression, App::Expression)

OperatorExpression::OperatorExpression(const App::DocumentObject *_owner, Expression * _left, Operator _op, Expression * _right)
    : UnitExpression(_owner)
    , op(_op)
    , left(_left)
    , right(_right)
{

}

OperatorExpression::~OperatorExpression()
{
    delete left;
    delete right;
}

/**
  * Determine whether the expression is touched or not, i.e relies on properties that are touched.
  */

bool OperatorExpression::isTouched() const
{
    return left->isTouched() || right->isTouched();
}

static Py::Object calc(const Expression *expr, int op,
                 const Expression *left, const Expression *right, bool inplace)
{
    Py::Object l = left->getPyValue();

    // For security reason, restrict supported types
    if(!PyObject_TypeCheck(l.ptr(),&PyObjectBase::Type)
            && !l.isNumeric() && !l.isString() && !l.isList() && !l.isDict())
    {
        __EXPR_THROW(Base::TypeError,"Unsupported operator", expr);
    }

    // check possible unary operation first
    switch(op) {
    case OperatorExpression::POS:{
        PyObject *res = PyNumber_Positive(l.ptr());
        if(!res) EXPR_PY_THROW(expr);
        return Py::asObject(res);
    }
    case OperatorExpression::NEG:{
        PyObject *res = PyNumber_Negative(l.ptr());
        if(!res) EXPR_PY_THROW(expr);
        return Py::asObject(res);
    } default:
        break;
    }

    Py::Object r = right->getPyValue();
    // For security reason, restrict supported types
    if((op!=OperatorExpression::MOD || !l.isString())
            && !PyObject_TypeCheck(r.ptr(),&PyObjectBase::Type)
                && !r.isNumeric()
                && !r.isString()
                && !r.isList()
                && !r.isDict())
    {
        __EXPR_THROW(Base::TypeError,"Unsupported operator", expr);
    }

    switch(op) {
#define RICH_COMPARE(_op,_pyop) \
    case OperatorExpression::_op: {\
        int res = PyObject_RichCompareBool(l.ptr(),r.ptr(),Py_##_pyop);\
        if(res<0) EXPR_PY_THROW(expr);\
        return Py::Boolean(!!res);\
    }
    RICH_COMPARE(LT,LT)
    RICH_COMPARE(LTE,LE)
    RICH_COMPARE(GT,GT)
    RICH_COMPARE(GTE,GE)
    RICH_COMPARE(EQ,EQ)
    RICH_COMPARE(NEQ,NE)

#define _BINARY_OP(_pyop) \
        res = inplace?PyNumber_InPlace##_pyop(l.ptr(),r.ptr()):\
                       PyNumber_##_pyop(l.ptr(),r.ptr());\
        if(!res) EXPR_PY_THROW(expr);\
        return Py::asObject(res);

#define BINARY_OP(_op,_pyop) \
    case OperatorExpression::_op: {\
        PyObject *res;\
        _BINARY_OP(_pyop);\
    }

    BINARY_OP(SUB,Subtract)
    BINARY_OP(MUL,Multiply)
    BINARY_OP(UNIT,Multiply)
    BINARY_OP(DIV,TrueDivide)
    case OperatorExpression::ADD: {
        PyObject *res;
#if PY_MAJOR_VERSION < 3
        if (PyString_CheckExact(*l) && PyString_CheckExact(*r)) {
            Py_ssize_t v_len = PyString_GET_SIZE(*l);
            Py_ssize_t w_len = PyString_GET_SIZE(*r);
            Py_ssize_t new_len = v_len + w_len;
            if (new_len < 0)
                __EXPR_THROW(OverflowError, "strings are too large to concat", expr);

            if (l.ptr()->ob_refcnt==1 && !PyString_CHECK_INTERNED(l.ptr())) {
                res = Py::new_reference_to(l);
                // Must make sure ob_refcnt is still 1
                l = Py::Object();
                if (_PyString_Resize(&res, new_len) != 0)
                    EXPR_PY_THROW(expr);
                memcpy(PyString_AS_STRING(res) + v_len, PyString_AS_STRING(*r), w_len);
            }else{
                res = Py::new_reference_to(l);
                l = Py::Object();
                PyString_Concat(&res,*r);
                if(!res) EXPR_PY_THROW(expr);
            }
            return Py::asObject(res);
        }
#else
        if (PyUnicode_CheckExact(*l) && PyUnicode_CheckExact(*r)) {
            if(inplace) {
                res = Py::new_reference_to(l);
                // Try to make sure ob_refcnt is 1, although unlike
                // PyString_Resize above, PyUnicode_Append can handle other
                // cases.
                l = Py::Object();
                PyUnicode_Append(&res, r.ptr());
            }else
                res = PyUnicode_Concat(l.ptr(),r.ptr());
            if(!res) EXPR_PY_THROW(expr);
            return Py::asObject(res);
        }
#endif
        _BINARY_OP(Add);
    }
    case OperatorExpression::POW: {
        PyObject *res;
        if(inplace)
            res = PyNumber_InPlacePower(l.ptr(),r.ptr(),Py::None().ptr());
        else
            res = PyNumber_Power(l.ptr(),r.ptr(),Py::None().ptr());
        if(!res) EXPR_PY_THROW(expr);
        return Py::asObject(res);
    }
    case OperatorExpression::MOD: {
        PyObject *res;
#if PY_MAJOR_VERSION < 3
        if (PyString_CheckExact(l.ptr()) &&
                (!PyString_Check(r.ptr()) || PyString_CheckExact(r.ptr())))
            res = PyString_Format(l.ptr(), r.ptr());
#else
        if (PyUnicode_CheckExact(l.ptr()) &&
                (!PyUnicode_Check(r.ptr()) || PyUnicode_CheckExact(r.ptr())))
            res = PyUnicode_Format(l.ptr(), r.ptr());
#endif
        else if(inplace)
            res = PyNumber_InPlaceRemainder(l.ptr(),r.ptr());
        else
            res = PyNumber_InPlaceRemainder(l.ptr(),r.ptr());
        if(!res) EXPR_PY_THROW(expr);
        return Py::asObject(res);
    }
    default:
        __EXPR_THROW(RuntimeError,"Unsupported operator",expr);
    }
}

Py::Object OperatorExpression::_getPyValue() const {
    return calc(this,op,left,right,false);
}

/**
  * Simplify the expression. For OperatorExpressions, we return a NumberExpression if
  * both the left and right side can be simplified to NumberExpressions. In this case
  * we can calculate the final value of the expression.
  *
  * @returns Simplified expression.
  */

Expression *OperatorExpression::simplify() const
{
    Expression * v1 = left->simplify();
    Expression * v2 = right->simplify();

    // Both arguments reduced to numerics? Then evaluate and return answer
    if (freecad_dynamic_cast<NumberExpression>(v1) && freecad_dynamic_cast<NumberExpression>(v2)) {
        delete v1;
        delete v2;
        return eval();
    }
    else
        return new OperatorExpression(owner, v1, op, v2);
}

/**
  * Create a string representation of the expression.
  *
  * @returns A string representing the expression.
  */

void OperatorExpression::_toString(std::ostream &s, bool persistent,int) const
{
    bool needsParens;
    Operator leftOperator(NONE), rightOperator(NONE);

    needsParens = false;
    if (freecad_dynamic_cast<OperatorExpression>(left))
        leftOperator = static_cast<OperatorExpression*>(left)->op;
    if (left->priority() < priority()) // Check on operator priority first
        needsParens = true;
    else if (leftOperator == op) { // Equal priority?
        if (!isLeftAssociative())
            needsParens = true;
        //else if (!isCommutative())
        //    needsParens = true;
    }

    switch (op) {
    case NEG:
        s << "-" << (needsParens ? "(" : "") << left->toString(persistent) << (needsParens ? ")" : "");
        return;
    case POS:
        s << "+" << (needsParens ? "(" : "") << left->toString(persistent) << (needsParens ? ")" : "");
        return;
    default:
        break;
    }

    if (needsParens)
        s << "(" << left->toString(persistent) << ")";
    else
        s << left->toString(persistent);

    switch (op) {
    case ADD:
        s << " + ";
        break;
    case SUB:
        s << " - ";
        break;
    case MUL:
        s << " * ";
        break;
    case DIV:
        s << " / ";
        break;
    case MOD:
        s << " % ";
        break;
    case POW:
        s << " ^ ";
        break;
    case EQ:
        s << " == ";
        break;
    case NEQ:
        s << " != ";
        break;
    case LT:
        s << " < ";
        break;
    case GT:
        s << " > ";
        break;
    case LTE:
        s << " <= ";
        break;
    case GTE:
        s << " >= ";
        break;
    case UNIT:
        break;
    default:
        assert(0);
    }

    needsParens = false;
    if (freecad_dynamic_cast<OperatorExpression>(right))
        rightOperator = static_cast<OperatorExpression*>(right)->op;
    if (right->priority() < priority()) // Check on operator priority first
        needsParens = true;
    else if (rightOperator == op) { // Equal priority?
        if (!isRightAssociative())
            needsParens = true;
        else if (!isCommutative())
            needsParens = true;
    }
    else if (right->priority() == priority()) {
        if (!isRightAssociative())
            needsParens = true;
    }

    if (needsParens) {
        s << "(";
        right->toString(s,persistent);
        s << ")";
    }else
        right->toString(s,persistent);
}

/**
  * A deep copy of the expression.
  */

Expression *OperatorExpression::_copy() const
{
    return new OperatorExpression(owner, left->copy(), op, right->copy());
}

/**
  * Return the operators priority. This is used to add parentheses where
  * needed when creating a string representation of the expression.
  *
  * @returns The operator's priority.
  */

int OperatorExpression::priority() const
{
    switch (op) {
    case EQ:
    case NEQ:
    case LT:
    case GT:
    case LTE:
    case GTE:
        return 1;
    case ADD:
    case SUB:
        return 3;
    case MUL:
    case DIV:
    case MOD:
        return 4;
    case POW:
        return 5;
    case UNIT:
    case NEG:
    case POS:
        return 6;
    default:
        assert(false);
        return 0;
    }
}

void OperatorExpression::_visit(ExpressionVisitor &v)
{
    if (left)
        left->visit(v);
    if (right)
        right->visit(v);
}

bool OperatorExpression::isCommutative() const
{
    switch (op) {
    case EQ:
    case NEQ:
    case ADD:
    case MUL:
        return true;
    default:
        return false;
    }
}

bool OperatorExpression::isLeftAssociative() const
{
    return true;
}

bool OperatorExpression::isRightAssociative() const
{
    switch (op) {
    case ADD:
    case MUL:
        return true;
    default:
        return false;
    }
}

//
// FunctionExpression class. This class handles functions with one or two parameters.
//

TYPESYSTEM_SOURCE(App::FunctionExpression, App::UnitExpression)

FunctionExpression::FunctionExpression(const DocumentObject *_owner, Function _f, std::vector<Expression *> _args)
    : UnitExpression(_owner)
    , f(_f)
    , args(_args)
{
    switch (f) {
    case ACOS:
    case ASIN:
    case ATAN:
    case ABS:
    case EXP:
    case LOG:
    case LOG10:
    case SIN:
    case SINH:
    case TAN:
    case TANH:
    case SQRT:
    case COS:
    case COSH:
    case ROUND:
    case TRUNC:
    case CEIL:
    case FLOOR:
    case MINVERT:
        if (args.size() != 1)
            EXPR_THROW("Invalid number of arguments: exactly one required.");
        break;
    case MOD:
    case ATAN2:
    case POW:
        if (args.size() != 2)
            EXPR_THROW("Invalid number of arguments: exactly two required.");
        break;
    case HYPOT:
    case CATH:
        if (args.size() < 2 || args.size() > 3)
            EXPR_THROW("Invalid number of arguments: exactly two, or three required.");
        break;
    case STDDEV:
    case SUM:
    case AVERAGE:
    case COUNT:
    case MIN:
    case MAX:
    case CREATE:
    case MSCALE:
        if (args.size() == 0)
            EXPR_THROW("Invalid number of arguments: at least one required.");
        break;
    case LIST:
    case TUPLE:
        break;
    case NONE:
    case AGGREGATES:
    case LAST:
    default:
        EXPR_THROW("Unknown function");
        break;
    }
}

FunctionExpression::~FunctionExpression()
{
    std::vector<Expression*>::iterator i = args.begin();

    while (i != args.end()) {
        delete *i;
        ++i;
    }
}

/**
  * Determine whether the expressions is considered touched, i.e one or both of its arguments
  * are touched.
  *
  * @return True if touched, false if not.
  */

bool FunctionExpression::isTouched() const
{
    std::vector<Expression*>::const_iterator i = args.begin();

    while (i != args.end()) {
        if ((*i)->isTouched())
            return true;
        ++i;
    }
    return false;
}

/* Various collectors for aggregate functions */

class Collector {
public:
    Collector() : first(true) { }
    virtual void collect(Quantity value) {
        if (first)
            q.setUnit(value.getUnit());
    }
    virtual Quantity getQuantity() const {
        return q;
    }
protected:
    bool first;
    Quantity q;
};

class SumCollector : public Collector {
public:
    SumCollector() : Collector() { }

    void collect(Quantity value) {
        Collector::collect(value);
        q += value;
        first = false;
    }

};

class AverageCollector : public Collector {
public:
    AverageCollector() : Collector(), n(0) { }

    void collect(Quantity value) {
        Collector::collect(value);
        q += value;
        ++n;
        first = false;
    }

    virtual Quantity getQuantity() const { return q/(double)n; }

private:
    unsigned int n;
};

class StdDevCollector : public Collector {
public:
    StdDevCollector() : Collector(), n(0) { }

    void collect(Quantity value) {
        Collector::collect(value);
        if (first) {
            M2 = Quantity(0, value.getUnit() * value.getUnit());
            mean = Quantity(0, value.getUnit());
            n = 0;
        }

        const Quantity delta = value - mean;
        ++n;
        mean = mean + delta / n;
        M2 = M2 + delta * (value - mean);
        first = false;
    }

    virtual Quantity getQuantity() const {
        if (n < 2)
            throw ExpressionError("Invalid number of entries: at least two required.");
        else
            return Quantity((M2 / (n - 1.0)).pow(Quantity(0.5)).getValue(), mean.getUnit());
    }

private:
    unsigned int n;
    Quantity mean;
    Quantity M2;
};

class CountCollector : public Collector {
public:
    CountCollector() : Collector(), n(0) { }

    void collect(Quantity value) {
        Collector::collect(value);
        ++n;
        first = false;
    }

    virtual Quantity getQuantity() const { return Quantity(n); }

private:
    unsigned int n;
};

class MinCollector : public Collector {
public:
    MinCollector() : Collector() { }

    void collect(Quantity value) {
        Collector::collect(value);
        if (first || value < q)
            q = value;
        first = false;
    }
};

class MaxCollector : public Collector {
public:
    MaxCollector() : Collector() { }

    void collect(Quantity value) {
        Collector::collect(value);
        if (first || value > q)
            q = value;
        first = false;
    }
};

Py::Object FunctionExpression::evalAggregate(
        const Expression *owner, int f, const std::vector<Expression*> &args)
{
    std::unique_ptr<Collector> c;

    switch (f) {
    case SUM:
        c.reset(new SumCollector);
        break;
    case AVERAGE:
        c.reset(new AverageCollector);
        break;
    case STDDEV:
        c.reset(new StdDevCollector);
        break;
    case COUNT:
        c.reset(new CountCollector);
        break;
    case MIN:
        c.reset(new MinCollector);
        break;
    case MAX:
        c.reset(new MaxCollector);
        break;
    default:
        assert(false);
    }

    for (auto &arg : args) {
        if (arg->isDerivedFrom(RangeExpression::getClassTypeId())) {
            Range range(static_cast<const RangeExpression&>(*arg).getRange());

            do {
                Property * p = owner->getOwner()->getPropertyByName(range.address().c_str());
                PropertyQuantity * qp;
                PropertyFloat * fp;
                PropertyInteger * ip;

                if (!p)
                    continue;

                if ((qp = freecad_dynamic_cast<PropertyQuantity>(p)) != 0)
                    c->collect(qp->getQuantityValue());
                else if ((fp = freecad_dynamic_cast<PropertyFloat>(p)) != 0)
                    c->collect(Quantity(fp->getValue()));
                else if ((ip = freecad_dynamic_cast<PropertyInteger>(p)) != 0)
                    c->collect(Quantity(ip->getValue()));
                else
                    _EXPR_THROW("Invalid property type for aggregate.", owner);
            } while (range.next());
        }
        else {
            Quantity q;
            if(pyToQuantity(q,arg->getPyValue()))
                c->collect(q);
        }
    }

    return pyFromQuantity(c->getQuantity());
}

Py::Object FunctionExpression::evaluate(const Expression *expr, int f, const std::vector<Expression*> &args)
{
    if(!expr || !expr->getOwner())
        _EXPR_THROW("Invalid owner.", expr);

    // Handle aggregate functions
    if (f > AGGREGATES)
        return evalAggregate(expr, f, args);

    if(f == LIST) {
        if(args.size() == 1 && args[0]->isDerivedFrom(RangeExpression::getClassTypeId()))
            return args[0]->getPyValue();
        Py::List list(args.size());
        int i=0;
        for(auto &arg : args)
            list.setItem(i++,arg->getPyValue());
        return list;
    } else if (f == TUPLE) {
        if(args.size() == 1 && args[0]->isDerivedFrom(RangeExpression::getClassTypeId()))
            return Py::Tuple(args[0]->getPyValue());
        Py::Tuple tuple(args.size());
        int i=0;
        for(auto &arg : args)
            tuple.setItem(i++,arg->getPyValue());
        return tuple;
    } else if (f == MSCALE) {
        if(args.size() < 2)
            _EXPR_THROW("Function requires at least two arguments.",expr);
        Py::Object pymat = args[0]->getPyValue();
        Py::Object pyscale;
        if(PyObject_TypeCheck(pymat.ptr(),&Base::MatrixPy::Type)) {
            if(args.size() == 2) {
                Py::Object obj = args[1]->getPyValue();
                if(obj.isSequence() && PySequence_Size(obj.ptr())==3)
                    pyscale = Py::Tuple(Py::Sequence(obj));
            } else if(args.size() == 4) {
                Py::Tuple tuple(3);
                tuple.setItem(0,args[1]->getPyValue());
                tuple.setItem(1,args[2]->getPyValue());
                tuple.setItem(2,args[3]->getPyValue());
                pyscale = tuple;
            }
        }
        if(!pyscale.isNone()) {
            Base::Vector3d vec;
            if (!PyArg_ParseTuple(pyscale.ptr(), "ddd", &vec.x,&vec.y,&vec.z))
                PyErr_Clear();
            else {
                auto mat = static_cast<Base::MatrixPy*>(pymat.ptr())->value();
                mat.scale(vec);
                return Py::asObject(new Base::MatrixPy(mat));
            }
        }
        _EXPR_THROW("Function requires arguments to be either "
                "(matrix,vector) or (matrix,number,number,number).", expr);
    }

    if(args.empty())
        _EXPR_THROW("Function requires at least one argument.",expr);

    if (f == MINVERT) {
        Py::Object pyobj = args[0]->getPyValue();
        Py::Tuple args;
        if (PyObject_TypeCheck(pyobj.ptr(),&Base::MatrixPy::Type)) {
            auto m = static_cast<Base::MatrixPy*>(pyobj.ptr())->value();
            if (fabs(m.determinant()) <= DBL_EPSILON)
                _EXPR_THROW("Cannot invert singular matrix.",expr);
            m.inverseGauss();
            return Py::asObject(new Base::MatrixPy(m));

        } else if (PyObject_TypeCheck(pyobj.ptr(),&Base::PlacementPy::Type)) {
            const auto &pla = *static_cast<Base::PlacementPy*>(pyobj.ptr())->getPlacementPtr();
            return Py::asObject(new Base::PlacementPy(pla.inverse()));

        } else if (PyObject_TypeCheck(pyobj.ptr(),&Base::RotationPy::Type)) {
            const auto &rot = *static_cast<Base::RotationPy*>(pyobj.ptr())->getRotationPtr();
            return Py::asObject(new Base::RotationPy(rot.inverse()));
        }
         _EXPR_THROW("Function requires the first argument to be either Matrix, Placement or Rotation.",expr);

    } else if (f == CREATE) {
        Py::Object pytype = args[0]->getPyValue();
        if(!pytype.isString())
            _EXPR_THROW("Function requires the first argument to be a string.",expr);
        std::string type(pytype.as_string());
        Py::Object res;
        if(boost::iequals(type,"matrix"))
            res = Py::asObject(new Base::MatrixPy(Base::Matrix4D()));
        else if(boost::iequals(type,"vector"))
            res = Py::asObject(new Base::VectorPy(Base::Vector3d()));
        else if(boost::iequals(type,"placement"))
            res = Py::asObject(new Base::PlacementPy(Base::Placement()));
        else if(boost::iequals(type,"rotation"))
            res = Py::asObject(new Base::RotationPy(Base::Rotation()));
        else
            _EXPR_THROW("Unknown type '" << type << "'.",expr);
        if(args.size()>1) {
            Py::Tuple tuple(args.size()-1);
            for(unsigned i=1;i<args.size();++i)
                tuple.setItem(i-1,args[i]->getPyValue());
            Py::Dict dict;
            PyObjectBase::__PyInit(res.ptr(),tuple.ptr(),dict.ptr());
        }
        return res;
    }

    Py::Object e1 = args[0]->getPyValue();
    Quantity v1 = pyToQuantity(e1,expr,"Invalid first argument.");
    Py::Object e2;
    Quantity v2;
    if(args.size()>1) {
        e2 = args[1]->getPyValue();
        v2 = pyToQuantity(e2,expr,"Invalid second argument.");
    }
    Py::Object e3;
    Quantity v3;
    if(args.size()>2) {
        e3 = args[2]->getPyValue();
        v3 = pyToQuantity(e3,expr,"Invalid third argument.");
    }

    double output;
    Unit unit;
    double scaler = 1;

    double value = v1.getValue();

    /* Check units and arguments */
    switch (f) {
    case COS:
    case SIN:
    case TAN:
        if (!(v1.getUnit() == Unit::Angle || v1.getUnit().isEmpty()))
            _EXPR_THROW("Unit must be either empty or an angle.",expr);

        // Convert value to radians
        value *= M_PI / 180.0;
        unit = Unit();
        break;
    case ACOS:
    case ASIN:
    case ATAN:
        if (!v1.getUnit().isEmpty())
            _EXPR_THROW("Unit must be empty.",expr);
        unit = Unit::Angle;
        scaler = 180.0 / M_PI;
        break;
    case EXP:
    case LOG:
    case LOG10:
    case SINH:
    case TANH:
    case COSH:
        if (!v1.getUnit().isEmpty())
            _EXPR_THROW("Unit must be empty.",expr);
        unit = Unit();
        break;
    case ROUND:
    case TRUNC:
    case CEIL:
    case FLOOR:
    case ABS:
        unit = v1.getUnit();
        break;
    case SQRT: {
        unit = v1.getUnit();

        // All components of unit must be either zero or dividable by 2
        UnitSignature s = unit.getSignature();
        if ( !((s.Length % 2) == 0) &&
              ((s.Mass % 2) == 0) &&
              ((s.Time % 2) == 0) &&
              ((s.ElectricCurrent % 2) == 0) &&
              ((s.ThermodynamicTemperature % 2) == 0) &&
              ((s.AmountOfSubstance % 2) == 0) &&
              ((s.LuminousIntensity % 2) == 0) &&
              ((s.Angle % 2) == 0))
            _EXPR_THROW("All dimensions must be even to compute the square root.",expr);

        unit = Unit(s.Length /2,
                    s.Mass / 2,
                    s.Time / 2,
                    s.ElectricCurrent / 2,
                    s.ThermodynamicTemperature / 2,
                    s.AmountOfSubstance / 2,
                    s.LuminousIntensity / 2,
                    s.Angle);
        break;
    }
    case ATAN2:
        if (e2.isNone())
            _EXPR_THROW("Invalid second argument.",expr);

        if (v1.getUnit() != v2.getUnit())
            _EXPR_THROW("Units must be equal.",expr);
        unit = Unit::Angle;
        scaler = 180.0 / M_PI;
        break;
    case MOD:
        if (e2.isNone())
            _EXPR_THROW("Invalid second argument.",expr);
        unit = v1.getUnit() / v2.getUnit();
        break;
    case POW: {
        if (e2.isNone())
            _EXPR_THROW("Invalid second argument.",expr);

        if (!v2.getUnit().isEmpty())
            _EXPR_THROW("Exponent is not allowed to have a unit.",expr);

        // Compute new unit for exponentiation
        double exponent = v2.getValue();
        if (!v1.getUnit().isEmpty()) {
            if (exponent - boost::math::round(exponent) < 1e-9)
                unit = v1.getUnit().pow(exponent);
            else
                _EXPR_THROW("Exponent must be an integer when used with a unit.",expr);
        }
        break;
    }
    case HYPOT:
    case CATH:
        if (e2.isNone())
            _EXPR_THROW("Invalid second argument.",expr);
        if (v1.getUnit() != v2.getUnit())
            _EXPR_THROW("Units must be equal.",expr);

        if (args.size() > 2) {
            if (e3.isNone())
                _EXPR_THROW("Invalid second argument.",expr);
            if (v2.getUnit() != v3.getUnit())
                _EXPR_THROW("Units must be equal.",expr);
        }
        unit = v1.getUnit();
        break;
    default:
        _EXPR_THROW("Unknown function: " << f,expr);
    }

    /* Compute result */
    switch (f) {
    case ACOS:
        output = acos(value);
        break;
    case ASIN:
        output = asin(value);
        break;
    case ATAN:
        output = atan(value);
        break;
    case ABS:
        output = fabs(value);
        break;
    case EXP:
        output = exp(value);
        break;
    case LOG:
        output = log(value);
        break;
    case LOG10:
        output = log(value) / log(10.0);
        break;
    case SIN:
        output = sin(value);
        break;
    case SINH:
        output = sinh(value);
        break;
    case TAN:
        output = tan(value);
        break;
    case TANH:
        output = tanh(value);
        break;
    case SQRT:
        output = sqrt(value);
        break;
    case COS:
        output = cos(value);
        break;
    case COSH:
        output = cosh(value);
        break;
    case MOD: {
        output = fmod(value, v2.getValue());
        break;
    }
    case ATAN2: {
        output = atan2(value, v2.getValue());
        break;
    }
    case POW: {
        output = pow(value, v2.getValue());
        break;
    }
    case HYPOT: {
        output = sqrt(pow(v1.getValue(), 2) + pow(v2.getValue(), 2) + (!e3.isNone() ? pow(v3.getValue(), 2) : 0));
        break;
    }
    case CATH: {
        output = sqrt(pow(v1.getValue(), 2) - pow(v2.getValue(), 2) - (!e3.isNone() ? pow(v3.getValue(), 2) : 0));
        break;
    }
    case ROUND:
        output = boost::math::round(value);
        break;
    case TRUNC:
        output = boost::math::trunc(value);
        break;
    case CEIL:
        output = ceil(value);
        break;
    case FLOOR:
        output = floor(value);
        break;
    default:
        _EXPR_THROW("Unknown function: " << f,expr);
    }

    return Py::asObject(new QuantityPy(new Quantity(scaler * output, unit)));
}

Py::Object FunctionExpression::_getPyValue() const {
    return evaluate(this,f,args);
}

/**
  * Try to simplify the expression, i.e calculate all constant expressions.
  *
  * @returns A simplified expression.
  */

Expression *FunctionExpression::simplify() const
{
    size_t numerics = 0;
    std::vector<Expression*> a;

    // Try to simplify each argument to function
    for (auto it = args.begin(); it != args.end(); ++it) {
        Expression * v = (*it)->simplify();

        if (freecad_dynamic_cast<NumberExpression>(v))
            ++numerics;
        a.push_back(v);
    }

    if (numerics == args.size()) {
        // All constants, then evaluation must also be constant

        // Clean-up
        for (auto it = args.begin(); it != args.end(); ++it)
            delete *it;

        return eval();
    }
    else
        return new FunctionExpression(owner, f, a);
}

/**
  * Create a string representation of the expression.
  *
  * @returns A string representing the expression.
  */

void FunctionExpression::_toString(std::ostream &ss, bool persistent,int) const
{
    switch (f) {
    case ACOS:
        ss << "acos("; break;;
    case ASIN:
        ss << "asin("; break;;
    case ATAN:
        ss << "atan("; break;;
    case ABS:
        ss << "abs("; break;;
    case EXP:
        ss << "exp("; break;;
    case LOG:
        ss << "log("; break;;
    case LOG10:
        ss << "log10("; break;;
    case SIN:
        ss << "sin("; break;;
    case SINH:
        ss << "sinh("; break;;
    case TAN:
        ss << "tan("; break;;
    case TANH:
        ss << "tanh("; break;;
    case SQRT:
        ss << "sqrt("; break;;
    case COS:
        ss << "cos("; break;;
    case COSH:
        ss << "cosh("; break;;
    case MOD:
        ss << "mod("; break;;
    case ATAN2:
        ss << "atan2("; break;;
    case POW:
        ss << "pow("; break;;
    case HYPOT:
        ss << "hypot("; break;;
    case CATH:
        ss << "cath("; break;;
    case ROUND:
        ss << "round("; break;;
    case TRUNC:
        ss << "trunc("; break;;
    case CEIL:
        ss << "ceil("; break;;
    case FLOOR:
        ss << "floor("; break;;
    case SUM:
        ss << "sum("; break;;
    case COUNT:
        ss << "count("; break;;
    case AVERAGE:
        ss << "average("; break;;
    case STDDEV:
        ss << "stddev("; break;;
    case MIN:
        ss << "min("; break;;
    case MAX:
        ss << "max("; break;;
    case LIST:
        ss << "list("; break;;
    case TUPLE:
        ss << "tuple("; break;;
    case MSCALE:
        ss << "mscale("; break;;
    case MINVERT:
        ss << "minvert("; break;;
    case CREATE:
        ss << "create("; break;;
    default:
        assert(0);
    }
    for (size_t i = 0; i < args.size(); ++i) {
        ss << args[i]->toString(persistent);
        if (i != args.size() - 1)
            ss << "; ";
    }
    ss << ')';
}

/**
  * Create a copy of the expression.
  *
  * @returns A deep copy of the expression.
  */

Expression *FunctionExpression::_copy() const
{
    std::vector<Expression*>::const_iterator i = args.begin();
    std::vector<Expression*> a;

    while (i != args.end()) {
        a.push_back((*i)->copy());
        ++i;
    }
    return new FunctionExpression(owner, f, a);
}

void FunctionExpression::_visit(ExpressionVisitor &v)
{
    std::vector<Expression*>::const_iterator i = args.begin();

    while (i != args.end()) {
        (*i)->visit(v);
        ++i;
    }
}

//
// VariableExpression class
//

TYPESYSTEM_SOURCE(App::VariableExpression, App::UnitExpression)

VariableExpression::VariableExpression(const DocumentObject *_owner, ObjectIdentifier _var)
    : UnitExpression(_owner)
    , var(_var)
{
}

VariableExpression::~VariableExpression()
{
}

/**
  * Determine if the expression is touched or not, i.e whether the Property object it
  * refers to is touched().
  *
  * @returns True if the Property object is touched, false if not.
  */

bool VariableExpression::isTouched() const
{
    return var.isTouched();
}

/**
  * Find the property this expression referse to.
  *
  * Unqualified names (i.e the name only without any dots) are resolved in the owning DocumentObjects.
  * Qualified names are looked up in the owning Document. It is first looked up by its internal name.
  * If not found, the DocumentObjects' labels searched.
  *
  * If something fails, an exception is thrown.
  *
  * @returns The Property object if it is derived from either PropertyInteger, PropertyFloat, or PropertyString.
  */

const Property * VariableExpression::getProperty() const
{
    const Property * prop = var.getProperty();

    if (prop)
        return prop;
    else
        throw Expression::Exception(var.resolveErrorString().c_str());
}

void VariableExpression::addComponent(Component *c) {
    do {
        if(components.size())
            break;
        if(!c->e1 && !c->e2) {
            var << c->comp;
            return;
        }
        long l1=0,l2=0,l3=1;
        if(c->e3) {
            auto n3 = freecad_dynamic_cast<NumberExpression>(c->e3);
            if(!n3 || !essentiallyEqual(n3->getValue(),(double)l3))
                break;
        }
        if(c->e1) {
            auto n1 = freecad_dynamic_cast<NumberExpression>(c->e1);
            if(!n1) {
                if(c->e2 || c->e3)
                    break;
                auto s = freecad_dynamic_cast<StringExpression>(c->e1);
                if(!s)
                    break;
                var << ObjectIdentifier::MapComponent(
                        ObjectIdentifier::String(s->getText(),true));
                return;
            }
            if(!essentiallyInteger(n1->getValue(),l1))
                break;
            if(!c->comp.isRange()) {
                var << ObjectIdentifier::ArrayComponent(l1);
                return;
            } else if(!c->e2) {
                var << ObjectIdentifier::RangeComponent(l1,l2,l3);
                return;
            }
        }
        auto n2 = freecad_dynamic_cast<NumberExpression>(c->e2);
        if(n2 && essentiallyInteger(n2->getValue(),l2)) {
            var << ObjectIdentifier::RangeComponent(l1,l2,l3);
            return;
        }
    }while(0);

    Expression::addComponent(c);
}

bool VariableExpression::_isIndexable() const {
    return true;
}

Py::Object VariableExpression::_getPyValue() const {
    return var.getPyValue(true);
}

void VariableExpression::_toString(std::ostream &ss, bool persistent,int) const {
    if(persistent)
        ss << var.toPersistentString();
    else
        ss << var.toString();
}

/**
  * Simplify the expression. Simplification of VariableExpression objects is
  * not possible (if it is instantiated it would be an evaluation instead).
  *
  * @returns A copy of the expression.
  */

Expression *VariableExpression::simplify() const
{
    return copy();
}

/**
  * Return a copy of the expression.
  */

Expression *VariableExpression::_copy() const
{
    return new VariableExpression(owner, var);
}

void VariableExpression::_getDeps(ExpressionDeps &deps) const
{
    auto dep = var.getDep();
    if(dep.first)
        deps[dep.first][dep.second].push_back(var);
}

void VariableExpression::_getDepObjects(
        std::set<App::DocumentObject*> &deps, std::vector<std::string> *labels) const
{
    auto dep = var.getDep(labels);
    if(dep.first)
        deps.insert(dep.first);
}

void VariableExpression::_getIdentifiers(std::set<App::ObjectIdentifier> &deps) const
{
    deps.insert(var);
}

bool VariableExpression::_relabeledDocument(const std::string &oldName,
        const std::string &newName, ExpressionVisitor &v)
{
    return var.relabeledDocument(v, oldName, newName);
}

bool VariableExpression::_adjustLinks(
        const std::set<App::DocumentObject *> &inList, ExpressionVisitor &v)
{
    return var.adjustLinks(v,inList);
}

void VariableExpression::_importSubNames(const ObjectIdentifier::SubNameMap &subNameMap)
{
    var.importSubNames(subNameMap);
}

void VariableExpression::_updateLabelReference(
        App::DocumentObject *obj, const std::string &ref, const char *newLabel)
{
    var.updateLabelReference(obj,ref,newLabel);
}

bool VariableExpression::_updateElementReference(
        App::DocumentObject *feature, bool reverse, ExpressionVisitor &v)
{
    return var.updateElementReference(v,feature,reverse);
}

bool VariableExpression::_renameObjectIdentifier(
        const std::map<ObjectIdentifier,ObjectIdentifier> &paths,
        const ObjectIdentifier &path, ExpressionVisitor &v)
{
    const auto &oldPath = var.canonicalPath();
    auto it = paths.find(oldPath);
    if (it != paths.end()) {
        v.aboutToChange();
        if(path.getOwner())
            var = it->second.relativeTo(path);
        else
            var = it->second;
        return true;
    }
    return false;
}

void VariableExpression::_collectReplacement(
        std::map<ObjectIdentifier,ObjectIdentifier> &paths,
        const App::DocumentObject *parent,
        App::DocumentObject *oldObj,
        App::DocumentObject *newObj) const
{
    ObjectIdentifier path;
    if(var.replaceObject(path,parent,oldObj,newObj))
        paths[var.canonicalPath()] = std::move(path);
}

void VariableExpression::_moveCells(const CellAddress &address,
        int rowCount, int colCount, ExpressionVisitor &v)
{
    if(var.hasDocumentObjectName(true))
        return;

    int idx = 0;
    const auto &comp = var.getPropertyComponent(0,&idx);
    CellAddress addr = stringToAddress(comp.getName().c_str(),true);
    if(!addr.isValid())
        return;

    int thisRow = addr.row();
    int thisCol = addr.col();
    if (thisRow >= address.row() || thisCol >= address.col()) {
        v.aboutToChange();
        addr.setRow(thisRow + rowCount);
        addr.setCol(thisCol + colCount);
        var.setComponent(idx,ObjectIdentifier::SimpleComponent(addr.toString()));
    }
}

void VariableExpression::_offsetCells(int rowOffset, int colOffset, ExpressionVisitor &v) {
    if(var.hasDocumentObjectName(true))
        return;

    int idx = 0;
    const auto &comp = var.getPropertyComponent(0,&idx);
    CellAddress addr = stringToAddress(comp.getName().c_str(),true);
    if(!addr.isValid() || (addr.isAbsoluteCol() && addr.isAbsoluteRow()))
        return;

    v.aboutToChange();
    if(!addr.isAbsoluteCol())
        addr.setCol(addr.col()+colOffset);
    if(!addr.isAbsoluteRow())
        addr.setRow(addr.row()+rowOffset);
    var.setComponent(idx,ObjectIdentifier::SimpleComponent(addr.toString()));
}

void VariableExpression::setPath(const ObjectIdentifier &path)
{
     var = path;
}

//
// PyObjectExpression class
//

TYPESYSTEM_SOURCE(App::PyObjectExpression, App::Expression)

PyObjectExpression::~PyObjectExpression() {
    if(pyObj) {
        Base::PyGILStateLocker lock;
        Py::_XDECREF(pyObj);
    }
}

Py::Object PyObjectExpression::_getPyValue() const {
    if(!pyObj)
        return Py::Object();
    return Py::Object(pyObj);
}

void PyObjectExpression::setPyValue(Py::Object obj) {
    Py::_XDECREF(pyObj);
    pyObj = obj.ptr();
    Py::_XINCREF(pyObj);
}

void PyObjectExpression::setPyValue(PyObject *obj, bool owned) {
    if(pyObj == obj)
        return;
    Py::_XDECREF(pyObj);
    pyObj = obj;
    if(!owned)
        Py::_XINCREF(pyObj);
}

void PyObjectExpression::_toString(std::ostream &ss, bool,int) const
{
    if(!pyObj)
        ss << "None";
    else {
        Base::PyGILStateLocker lock;
        ss << Py::Object(pyObj).as_string();
    }
}

Expression* PyObjectExpression::_copy() const
{
    return new PyObjectExpression(owner,pyObj,false);
}

//
// StringExpression class
//

TYPESYSTEM_SOURCE(App::StringExpression, App::Expression)

StringExpression::StringExpression(const DocumentObject *_owner, const std::string &_text)
    : Expression(_owner)
    , text(_text)
{
}

StringExpression::~StringExpression() {
    if(cache) {
        Base::PyGILStateLocker lock;
        Py::_XDECREF(cache);
    }
}

/**
  * Simplify the expression. For strings, this is a simple copy of the object.
  */

Expression *StringExpression::simplify() const
{
    return copy();
}

void StringExpression::_toString(std::ostream &ss, bool,int) const
{
    ss << quote(text);
}

/**
  * Return a copy of the expression.
  */

Expression *StringExpression::_copy() const
{
    return new StringExpression(owner, text);
}

Py::Object StringExpression::_getPyValue() const {
    return Py::String(text);
}

TYPESYSTEM_SOURCE(App::ConditionalExpression, App::Expression)

ConditionalExpression::ConditionalExpression(const DocumentObject *_owner, Expression *_condition, Expression *_trueExpr, Expression *_falseExpr)
    : Expression(_owner)
    , condition(_condition)
    , trueExpr(_trueExpr)
    , falseExpr(_falseExpr)
{
}

ConditionalExpression::~ConditionalExpression()
{
    delete condition;
    delete trueExpr;
    delete falseExpr;
}

bool ConditionalExpression::isTouched() const
{
    return condition->isTouched() || trueExpr->isTouched() || falseExpr->isTouched();
}

Py::Object ConditionalExpression::_getPyValue() const {
    if(condition->getPyValue().isTrue())
        return trueExpr->getPyValue();
    else
        return falseExpr->getPyValue();
}

Expression *ConditionalExpression::simplify() const
{
    std::unique_ptr<Expression> e(condition->simplify());
    NumberExpression * v = freecad_dynamic_cast<NumberExpression>(e.get());

    if (v == 0)
        return new ConditionalExpression(owner, condition->simplify(), trueExpr->simplify(), falseExpr->simplify());
    else {
        if (fabs(v->getValue()) > 0.5)
            return trueExpr->simplify();
        else
            return falseExpr->simplify();
    }
}

void ConditionalExpression::_toString(std::ostream &ss, bool persistent,int) const
{
    condition->toString(ss,persistent);
    ss << " ? ";
    if (trueExpr->priority() <= priority()) {
        ss << '(';
        trueExpr->toString(ss,persistent);
        ss << ')';
    } else
        trueExpr->toString(ss,persistent);

    ss << " : ";

    if (falseExpr->priority() <= priority()) {
        ss << '(';
        falseExpr->toString(ss,persistent);
        ss << ')';
    } else
        falseExpr->toString(ss,persistent);
}

Expression *ConditionalExpression::_copy() const
{
    return new ConditionalExpression(owner, condition->copy(), trueExpr->copy(), falseExpr->copy());
}

int ConditionalExpression::priority() const
{
    return 2;
}

void ConditionalExpression::_visit(ExpressionVisitor &v)
{
    condition->visit(v);
    trueExpr->visit(v);
    falseExpr->visit(v);
}

TYPESYSTEM_SOURCE(App::ConstantExpression, App::NumberExpression)

ConstantExpression::ConstantExpression(const DocumentObject *_owner,
        const char *_name, const Quantity & _quantity)
    : NumberExpression(_owner, _quantity)
    , name(_name)
{
}

Expression *ConstantExpression::_copy() const
{
    return new ConstantExpression(owner, name, getQuantity());
}

void ConstantExpression::_toString(std::ostream &ss, bool,int) const
{
    ss << name;
}

Py::Object ConstantExpression::_getPyValue() const {
    if(!cache) {
        if(strcmp(name,"None")==0)
            cache = Py::new_reference_to(Py::None());
        else if(strcmp(name,"True")==0)
            cache = Py::new_reference_to(Py::True());
        else if(strcmp(name, "False")==0)
            cache = Py::new_reference_to(Py::False());
        else
            return NumberExpression::_getPyValue();
    }
    return Py::Object(cache);
}

bool ConstantExpression::isNumber() const {
    return strcmp(name,"None")
        && strcmp(name,"True")
        && strcmp(name, "False");
}

TYPESYSTEM_SOURCE(App::RangeExpression, App::Expression)

RangeExpression::RangeExpression(const DocumentObject *_owner, const std::string &begin, const std::string &end)
    : Expression(_owner), begin(begin), end(end)
{
}

bool RangeExpression::isTouched() const
{
    Range i(getRange());

    do {
        Property * prop = owner->getPropertyByName(i.address().c_str());

        if (prop && prop->isTouched())
            return true;
    } while (i.next());

    return false;
}

Py::Object RangeExpression::_getPyValue() const {
    Py::List list;
    Range range(getRange());
    do {
        Property * p = owner->getPropertyByName(range.address().c_str());
        if(p)
            list.append(Py::asObject(p->getPyObject()));
    } while (range.next());
    return list;
}

void RangeExpression::_toString(std::ostream &ss, bool,int) const
{
    ss << begin << ":" << end;
}

Expression *RangeExpression::_copy() const
{
    return new RangeExpression(owner, begin, end);
}

Expression *RangeExpression::simplify() const
{
    return copy();
}

void RangeExpression::_getDeps(ExpressionDeps &deps) const
{
    assert(owner);

    Range i(getRange());

    auto &dep = deps[owner];
    do {
        std::string address = i.address();
        dep[address].push_back(ObjectIdentifier(owner,address));
    } while (i.next());
}

Range RangeExpression::getRange() const
{
    auto c1 = stringToAddress(begin.c_str(),true);
    auto c2 = stringToAddress(end.c_str(),true);
    if(c1.isValid() && c1.isValid())
        return Range(c1,c2);

    Base::PyGILStateLocker lock;
    static const std::string attr("getCellFromAlias");
    Py::Object pyobj(owner->getPyObject(),true);
    if(!pyobj.hasAttr(attr))
        EXPR_THROW("Invalid cell range " << begin << ':' << end);
    Py::Callable callable(pyobj.getAttr(attr));
    if(!c1.isValid()) {
        try {
            Py::Tuple arg(1);
            arg.setItem(0,Py::String(begin));
            c1 = CellAddress(callable.apply(arg).as_string().c_str());
        } catch(Py::Exception &) {
            _EXPR_PY_THROW("Invalid cell address '" << begin << "': ",this);
        } catch(Base::Exception &e) {
            _EXPR_RETHROW(e,"Invalid cell address '" << begin << "': ",this);
        }
    }
    if(!c2.isValid()) {
        try {
            Py::Tuple arg(1);
            arg.setItem(0,Py::String(end));
            c2 = CellAddress(callable.apply(arg).as_string().c_str());
        } catch(Py::Exception &) {
            _EXPR_PY_THROW("Invalid cell address '" << end << "': ", this);
        } catch(Base::Exception &e) {
            _EXPR_RETHROW(e,"Invalid cell address '" << end << "': ", this);
        }
    }
    return Range(c1,c2);
}

bool RangeExpression::_renameObjectIdentifier(
        const std::map<ObjectIdentifier,ObjectIdentifier> &paths,
        const ObjectIdentifier &path, ExpressionVisitor &v)
{
    (void)path;
    bool touched =false;
    auto it = paths.find(ObjectIdentifier(owner,begin));
    if (it != paths.end()) {
        v.aboutToChange();
        begin = it->second.getPropertyName();
        touched = true;
    }
    it = paths.find(ObjectIdentifier(owner,end));
    if (it != paths.end()) {
        v.aboutToChange();
        end = it->second.getPropertyName();
        touched = true;
    }
    return touched;
}

void RangeExpression::_moveCells(const CellAddress &address,
        int rowCount, int colCount, ExpressionVisitor &v)
{
    CellAddress addr = stringToAddress(begin.c_str(),true);
    if(addr.isValid()) {
        int thisRow = addr.row();
        int thisCol = addr.col();
        if (thisRow >= address.row() || thisCol >= address.col()) {
            v.aboutToChange();
            addr.setRow(thisRow+rowCount);
            addr.setCol(thisCol+colCount);
            begin = addr.toString();
        }
    }
    addr = stringToAddress(end.c_str(),true);
    if(addr.isValid()) {
        int thisRow = addr.row();
        int thisCol = addr.col();
        if (thisRow >= address.row() || thisCol >= address.col()) {
            v.aboutToChange();
            addr.setRow(thisRow + rowCount);
            addr.setCol(thisCol + colCount);
            end = addr.toString();
        }
    }
}

void RangeExpression::_offsetCells(int rowOffset, int colOffset, ExpressionVisitor &v)
{
    CellAddress addr = stringToAddress(begin.c_str(),true);
    if(addr.isValid() && (!addr.isAbsoluteRow() || !addr.isAbsoluteCol())) {
        v.aboutToChange();
        if(!addr.isAbsoluteRow())
            addr.setRow(addr.row()+rowOffset);
        if(!addr.isAbsoluteCol())
            addr.setCol(addr.col()+colOffset);
        begin = addr.toString();
    }
    addr = stringToAddress(end.c_str(),true);
    if(addr.isValid() && (!addr.isAbsoluteRow() || !addr.isAbsoluteCol())) {
        v.aboutToChange();
        if(!addr.isAbsoluteRow())
            addr.setRow(addr.row()+rowOffset);
        if(!addr.isAbsoluteCol())
            addr.setCol(addr.col()+colOffset);
        end = addr.toString();
    }
}


////////////////////////////////////////////////////////////////////////////////////

static Base::XMLReader *_Reader = 0;
ExpressionParser::ExpressionImporter::ExpressionImporter(Base::XMLReader &reader) {
    assert(!_Reader);
    _Reader = &reader;
}

ExpressionParser::ExpressionImporter::~ExpressionImporter() {
    assert(_Reader);
    _Reader = 0;
}

Base::XMLReader *ExpressionParser::ExpressionImporter::reader() {
    return _Reader;
}

namespace App {

namespace ExpressionParser {

bool isModuleImported(PyObject *module) {
    (void)module;
    return false;
}

/**
 * Error function for parser. Throws a generic Base::Exception with the parser error.
 */

void ExpressionParser_yyerror(const char *errorinfo)
{
    (void)errorinfo;
}

/* helper function for tuning number strings with groups in a locale agnostic way... */
double num_change(char* yytext,char dez_delim,char grp_delim)
{
    double ret_val;
    char temp[40];
    int i = 0;
    for(char* c=yytext;*c!='\0';c++){
        // skip group delimiter
        if(*c==grp_delim) continue;
        // check for a dez delimiter other then dot
        if(*c==dez_delim && dez_delim !='.')
             temp[i++] = '.';
        else
            temp[i++] = *c;
        // check buffer overflow
        if (i>39) return 0.0;
    }
    temp[i] = '\0';

    errno = 0;
    ret_val = strtod( temp, NULL );
    if (ret_val == 0 && errno == ERANGE)
        throw Base::UnderflowError("Number underflow.");
    if (ret_val == HUGE_VAL || ret_val == -HUGE_VAL)
        throw Base::OverflowError("Number overflow.");

    return ret_val;
}

static Expression * ScanResult = 0;                    /**< The resulting expression after a successful parsing */
static const App::DocumentObject * DocumentObject = 0; /**< The DocumentObject that will own the expression */
static bool unitExpression = false;                    /**< True if the parsed string is a unit only */
static bool valueExpression = false;                   /**< True if the parsed string is a full expression */
static std::stack<std::string> labels;                /**< Label string primitive */
static std::map<std::string, FunctionExpression::Function> registered_functions;                /**< Registered functions */
static int last_column;
static int column;

// show the parser the lexer method
#define yylex ExpressionParserlex
int ExpressionParserlex(void);

// Parser, defined in ExpressionParser.y
# define YYTOKENTYPE
#include "ExpressionParser.tab.c"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
// Scanner, defined in ExpressionParser.l
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wsign-compare"
# pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
#elif defined (__GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wsign-compare"
#endif
#include "lex.ExpressionParser.c"
#if defined(__clang__)
# pragma clang diagnostic pop
#elif defined (__GNUC__)
# pragma GCC diagnostic pop
#endif
#endif // DOXYGEN_SHOULD_SKIP_THIS
#ifdef _MSC_VER
# define strdup _strdup
#endif

static void initParser(const App::DocumentObject *owner)
{
    static bool has_registered_functions = false;

    using namespace App::ExpressionParser;

    ScanResult = 0;
    App::ExpressionParser::DocumentObject = owner;
    labels = std::stack<std::string>();
    column = 0;
    unitExpression = valueExpression = false;

    if (!has_registered_functions) {
        registered_functions["acos"] = FunctionExpression::ACOS;
        registered_functions["asin"] = FunctionExpression::ASIN;
        registered_functions["atan"] = FunctionExpression::ATAN;
        registered_functions["abs"] = FunctionExpression::ABS;
        registered_functions["exp"] = FunctionExpression::EXP;
        registered_functions["log"] = FunctionExpression::LOG;
        registered_functions["log10"] = FunctionExpression::LOG10;
        registered_functions["sin"] = FunctionExpression::SIN;
        registered_functions["sinh"] = FunctionExpression::SINH;
        registered_functions["tan"] = FunctionExpression::TAN;
        registered_functions["tanh"] = FunctionExpression::TANH;
        registered_functions["sqrt"] = FunctionExpression::SQRT;
        registered_functions["cos"] = FunctionExpression::COS;
        registered_functions["cosh"] = FunctionExpression::COSH;
        registered_functions["atan2"] = FunctionExpression::ATAN2;
        registered_functions["mod"] = FunctionExpression::MOD;
        registered_functions["pow"] = FunctionExpression::POW;
        registered_functions["round"] = FunctionExpression::ROUND;
        registered_functions["trunc"] = FunctionExpression::TRUNC;
        registered_functions["ceil"] = FunctionExpression::CEIL;
        registered_functions["floor"] = FunctionExpression::FLOOR;
        registered_functions["hypot"] = FunctionExpression::HYPOT;
        registered_functions["cath"] = FunctionExpression::CATH;
        registered_functions["list"] = FunctionExpression::LIST;
        registered_functions["tuple"] = FunctionExpression::TUPLE;
        registered_functions["mscale"] = FunctionExpression::MSCALE;
        registered_functions["minvert"] = FunctionExpression::MINVERT;
        registered_functions["create"] = FunctionExpression::CREATE;

        // Aggregates
        registered_functions["sum"] = FunctionExpression::SUM;
        registered_functions["count"] = FunctionExpression::COUNT;
        registered_functions["average"] = FunctionExpression::AVERAGE;
        registered_functions["stddev"] = FunctionExpression::STDDEV;
        registered_functions["min"] = FunctionExpression::MIN;
        registered_functions["max"] = FunctionExpression::MAX;

        has_registered_functions = true;
    }
}

std::vector<boost::tuple<int, int, std::string> > tokenize(const std::string &str)
{
    ExpressionParser::YY_BUFFER_STATE buf = ExpressionParser_scan_string(str.c_str());
    std::vector<boost::tuple<int, int, std::string> > result;
    int token;

    column = 0;
    try {
        while ( (token  = ExpressionParserlex()) != 0)
            result.push_back(boost::make_tuple(token, ExpressionParser::last_column, yytext));
    }
    catch (...) {
        // Ignore all exceptions
    }

    ExpressionParser_delete_buffer(buf);
    return result;
}

}

}

/**
  * Parse the expression given by \a buffer, and use \a owner as the owner of the
  * returned expression. If the parser fails for some reason, and exception is thrown.
  *
  * @param owner  The DocumentObject that will own the expression.
  * @param buffer The string buffer to parse.
  *
  * @returns A pointer to an expression.
  *
  */

Expression * App::ExpressionParser::parse(const App::DocumentObject *owner, const char* buffer)
{
    // parse from buffer
    ExpressionParser::YY_BUFFER_STATE my_string_buffer = ExpressionParser::ExpressionParser_scan_string (buffer);

    initParser(owner);

    // run the parser
    int result = ExpressionParser::ExpressionParser_yyparse ();

    // free the scan buffer
    ExpressionParser::ExpressionParser_delete_buffer (my_string_buffer);

    if (result != 0)
        throw ParserError("Failed to parse expression.");

    if (ScanResult == 0)
        throw ParserError("Unknown error in expression");

    if (valueExpression)
        return ScanResult;
    else {
        delete ScanResult;
        throw Expression::Exception("Expression can not evaluate to a value.");
        return 0;
    }
}

UnitExpression * ExpressionParser::parseUnit(const App::DocumentObject *owner, const char* buffer)
{
    // parse from buffer
    ExpressionParser::YY_BUFFER_STATE my_string_buffer = ExpressionParser::ExpressionParser_scan_string (buffer);

    initParser(owner);

    // run the parser
    int result = ExpressionParser::ExpressionParser_yyparse ();

    // free the scan buffer
    ExpressionParser::ExpressionParser_delete_buffer (my_string_buffer);

    if (result != 0)
        throw ParserError("Failed to parse expression.");

    if (ScanResult == 0)
        throw ParserError("Unknown error in expression");

    // Simplify expression
    Expression * simplified = ScanResult->simplify();

    if (!unitExpression) {
        OperatorExpression * fraction = freecad_dynamic_cast<OperatorExpression>(ScanResult);

        if (fraction && fraction->getOperator() == OperatorExpression::DIV) {
            NumberExpression * nom = freecad_dynamic_cast<NumberExpression>(fraction->getLeft());
            UnitExpression * denom = freecad_dynamic_cast<UnitExpression>(fraction->getRight());

            // If not initially a unit expression, but value is equal to 1, it means the expression is something like 1/unit
            if (denom && nom && essentiallyEqual(nom->getValue(), 1.0))
                unitExpression = true;
        }
    }
    delete ScanResult;

    if (unitExpression) {
        NumberExpression * num = freecad_dynamic_cast<NumberExpression>(simplified);

        if (num) {
           simplified = new UnitExpression(num->getOwner(), num->getQuantity());
            delete num;
        }
        return freecad_dynamic_cast<UnitExpression>(simplified);
    }
    else {
        delete simplified;
        throw Expression::Exception("Expression is not a unit.");
        return 0;
    }
}

bool ExpressionParser::isTokenAnIndentifier(const std::string & str)
{
    ExpressionParser::YY_BUFFER_STATE buf = ExpressionParser_scan_string(str.c_str());
    int token = ExpressionParserlex();
    int status = ExpressionParserlex();
    ExpressionParser_delete_buffer(buf);

    if (status == 0 && (token == IDENTIFIER || token == CELLADDRESS ))
        return true;
    else
        return false;
}

bool ExpressionParser::isTokenAUnit(const std::string & str)
{
    ExpressionParser::YY_BUFFER_STATE buf = ExpressionParser_scan_string(str.c_str());
    int token = ExpressionParserlex();
    int status = ExpressionParserlex();
    ExpressionParser_delete_buffer(buf);

    if (status == 0 && token == UNIT)
        return true;
    else
        return false;
}

#if defined(__clang__)
# pragma clang diagnostic pop
#endif
