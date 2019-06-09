/***************************************************************************
 *   Copyright (c) Eivind Kvedalen (eivind@kvedalen.name) 2015             *
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

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/Sequencer.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentPy.h>
#include <App/DocumentObject.h>
#include <App/PropertyUnits.h>
#include <Base/QuantityPy.h>
#include <Base/Tools.h>
#include <QStringList>
#include <string>
#include <sstream>
#include <math.h>
#include <stdio.h>
#include <stack>
#include <deque>
#include <cctype>
#include <algorithm>
#include <climits>
#include "Expression.h"
#include "ExpressionParser.h"
#include <Base/Unit.h>
#include <App/PropertyUnits.h>
#include <App/ObjectIdentifier.h>
#include <boost/math/special_functions/round.hpp>
#include <boost/math/special_functions/trunc.hpp>
#include "ExpressionPy.h"
#include "DocumentObjectPy.h"

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
    ss << _msg;\
    if(_expr) ss << std::endl << (_expr)->toStr();\
    throw _e(ss.str().c_str());\
}while(0)

#define _EXPR_THROW(_msg,_expr) __EXPR_THROW(ExpressionError,_msg,_expr)

#define __EXPR_SET_MSG(_e,_msg,_expr) do {\
    std::ostringstream ss;\
    ss << _msg << _e.what();\
    if(_expr) ss << std::endl << (_expr)->toStr();\
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

#define EXPR_NEW(_t,...) \
    ExpressionPtr(::new(ExpressionFastAlloc(_t)().allocate(1)) _t(__VA_ARGS__))

#define _EXPR_NEW(_v,_t,...) \
    ExpressionPtr _##_v;\
    {\
        Timing(exprNew);\
        _##_v = ExpressionPtr(::new(ExpressionFastAlloc(_t)().allocate(1)) _t(__VA_ARGS__));\
    }\
    auto _v = static_cast<_t*>(_##_v.get())

#define EXPR_TYPESYSTEM_SOURCE(_t,_p) \
    TYPESYSTEM_SOURCE_ABSTRACT(_t,_p);\
    void _t::operator delete(void *p) {\
        if(p) ExpressionFastAlloc(_t)().deallocate(reinterpret_cast<_t*>(p),1);\
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

// #define DO_TIMING

struct Stats {
#define DEFINE_STATS \
    DEFINE_STAT(exprNew) \
    DEFINE_STAT(getVar) \
    DEFINE_STAT(cond) \
    DEFINE_STAT(setVarInfo) \
    DEFINE_STAT(_pyObjectFromAny) \
    DEFINE_STAT(_pyObjectFromAny2) \
    DEFINE_STAT(pyObjectToAny) \
    DEFINE_STAT(anyFromQuantity) \
    DEFINE_STAT(anyToQuantity) \
    DEFINE_STAT(pyToQuantity) \
    DEFINE_STAT(pyFromQuantity) \
    DEFINE_STAT(anyToDouble) \
    DEFINE_STAT(operatorExpression1) \
    DEFINE_STAT(operatorExpression2) \
    DEFINE_STAT(operatorExpression3) \
    DEFINE_STAT(calc) \

#define DEFINE_STAT(_name) \
    FC_DURATION_DECLARE(_name);\
    int _name##_count;

    DEFINE_STATS
    
    void init() {
#undef DEFINE_STAT
#define DEFINE_STAT(_name) \
        FC_DURATION_INIT(_name);\
        _name##_count = 0;

        DEFINE_STATS
    }

    void print() {
#undef DEFINE_STAT
#define DEFINE_STAT(_name) FC_DURATION_MSG(_name, #_name " count: " << _name##_count);
        DEFINE_STATS
    }

#undef DEFINE_STAT
#define DEFINE_STAT(_name) \
    void time_##_name(FC_TIME_POINT &t) {\
        ++_name##_count;\
        FC_DURATION_PLUS(_name,t);\
    }

    DEFINE_STATS
};

static Stats _Stats;

struct TimingInfo {
    FC_TIME_POINT t;
    FC_DURATION &d;
    TimingInfo(FC_DURATION &d)
        :d(d)
    {
        _FC_TIME_INIT(t);
    }
    ~TimingInfo() {
        FC_DURATION_PLUS(d,t);
    }
};

#ifdef DO_TIMING
#define _Timing(_idx,_name) ++_Stats._name##_count; TimingInfo _tt##_idx(_Stats._name)
#define Timing(_name) _Timing(0,_name)
#define TimingInit() _Stats.init();
#define TimingPrint() _Stats.print();
#else
#define _Timing(...) do{}while(0)
#define Timing(...) do{}while(0)
#define TimingInit() do{}while(0)
#define TimingPrint() do{}while(0)
#endif

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


////////////////////////////////////////////////////////////////////////////////////

enum ExpressionInternalOption {
    ExpOpInternal = 64,
};

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
        std::map<ObjectIdentifier,ObjectIdentifier> &pathes,
        const App::DocumentObject *parent, App::DocumentObject *oldObj, App::DocumentObject *newObj) const
{
    return e._collectReplacement(pathes,parent,oldObj,newObj);
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
    return std::allocate_shared<PyObjectWrapper>(ExpressionFastAlloc(PyObjectWrapper)(),obj);
}

static inline bool isAnyPyObject(const App::any &value) {
    return is_type(value,typeid(PyObjectWrapper::Pointer));
}

static inline Py::Object __pyObjectFromAny(const App::any &value) {
    Timing(_pyObjectFromAny);
    return cast<PyObjectWrapper::Pointer>(value)->get();
}

static Py::Object _pyObjectFromAny(const App::any &value, const Expression *e) {
    Timing(_pyObjectFromAny2);
    if(value.empty())
        return Py::Object();
    else if (isAnyPyObject(value))
        return __pyObjectFromAny(value);
    if (is_type(value,typeid(Quantity)))
        return Py::Object(new QuantityPy(new Quantity(cast<Quantity>(value))));
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
        long l = cast<long>(value);
#if PY_MAJOR_VERSION < 3
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
    Timing(pyObjectToAny);

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
#endif
    else if (PyUnicode_Check(pyvalue)) {
        PyObject * s = PyUnicode_AsUTF8String(pyvalue);
        if(!s) 
            FC_THROWM(Base::ValueError,"Invalid unicode string");
        Py::Object o(s,true);

#if PY_MAJOR_VERSION >= 3
        return App::any(std::string(PyUnicode_AsUTF8(s)));
#else
        return App::any(std::string(PyString_AsString(s)));
#endif
    }
    else {
        return App::any(pyObjectWrap(pyvalue));
    }
}

bool pyToQuantity(Quantity &q, const Py::Object &pyobj) {
    Timing(pyToQuantity);
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

ExpressionPtr expressionFromPy(const DocumentObject *owner, const Py::Object &value) {
    if (value.isNone()) {
        return PyObjectExpression::create(owner);
    }
    if(value.isString()) {
        return StringExpression::create(owner,value.as_string());
    } else if (PyObject_TypeCheck(value.ptr(),&QuantityPy::Type)) {
        return NumberExpression::create(owner,
                *static_cast<QuantityPy*>(value.ptr())->getQuantityPtr());
    } else if (value.isBoolean()) {
        return BooleanExpression::create(owner,value.isTrue());
    } else {
        Quantity q;
        if(pyToQuantity(q,value))
            return NumberExpression::create(owner,q);
    }
    return PyObjectExpression::create(owner,value.ptr());
}

Py::Object pyFromQuantity(const Quantity &quantity) {
    Timing(pyFromQuantity);
    if(!quantity.getUnit().isEmpty())
        return Py::Object(new QuantityPy(new Quantity(quantity)));
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
    Timing(anyToQuantity);
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
    Timing(anyToDouble);
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

} // namespace App

//
// Expression component
//
Expression::Component::Component(const std::string &n)
    :comp(ObjectIdentifier::SimpleComponent(n))
{}

Expression::Component::Component(ExpressionPtr &&_e1, ExpressionPtr &&_e2, ExpressionPtr &&_e3, bool isRange)
    :e1(std::move(_e1)) ,e2(std::move(_e2)) ,e3(std::move(_e3))
{
    if(isRange || e2 || e3) 
        comp = ObjectIdentifier::RangeComponent(0);
}

Expression::Component::Component(const ObjectIdentifier::Component &comp)
    :comp(comp)
{}

Expression::Component::Component(const Component &other)
    :comp(other.comp)
    ,e1(other.e1?other.e1->copy():0)
    ,e2(other.e2?other.e2->copy():0)
    ,e3(other.e3?other.e3->copy():0)
{}

Expression::Component::~Component() 
{
}

void *Expression::Component::operator new(std::size_t) {
    return ExpressionFastAlloc(Component)().allocate(1);
}

Expression::ComponentPtr Expression::Component::copy() const {
    return ComponentPtr(new Component(*this));
}

Expression::ComponentPtr Expression::Component::eval() const {
    ComponentPtr res(new Component(comp));
    if(e1) res->e1 = e1->eval(ExpOpInternal);
    if(e2) res->e2 = e2->eval(ExpOpInternal);
    if(e3) res->e3 = e3->eval(ExpOpInternal);
    return res;
}

Py::Object Expression::Component::get(const Expression *owner, const Py::Object &pyobj) const {
    try {
        if(!e1 && !e2 && !e3)
            return comp.get(pyobj);
        if(!comp.isRange() && !e2 && !e3) {
            auto index = e1->getPyValue();
            if(pyobj.isMapping())
                return Py::Mapping(pyobj).getItem(index);
            else {
                Py_ssize_t i = PyNumber_AsSsize_t(index.ptr(), PyExc_IndexError);
                if(PyErr_Occurred())
                    throw Py::Exception();
                return Py::Sequence(pyobj).getItem(i);
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

void Expression::Component::operator delete(void *p) { 
    if(!p) ExpressionFastAlloc(Component)().deallocate(reinterpret_cast<Component*>(p),1);
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
        ss << e1->toStr(persistent);
    if(e2 || comp.isRange())
        ss << ':';
    if(e2)
        ss << e2->toStr(persistent);
    if(e3)
        ss << ':' << e3->toStr(persistent);
    ss << ']';
}

//////////////////////////////////////////////////////////////////////////////

typedef std::set<std::string,
                 std::less<std::string>,
                 ExpressionFastAlloc(std::string) > StringSet;

struct EvalFrame;
static EvalFrame *_EvalCallFrame;
static std::vector<EvalFrame*> _EvalStack;
#define CHECK_STACK(_type, _e) do{\
    if(_EvalStack.empty())\
        _EXPR_THROW(#_type " can only be used inside 'eval' or 'func'",_e);\
}while(0)

enum BindType {
    BindLocal,
    BindExist,
    BindNonExist,
    BindLocalOnly,
    BindQuery,

    BindNonLocal,
    BindGlobal,
};

typedef std::pair<const char *, int> EvalWarn;
std::unordered_set<EvalWarn, boost::hash<EvalWarn> > _EvalWarned;

struct EvalFrame {
    struct Binding {
        Py::Object obj;
        EvalFrame *owner;
        std::set<EvalFrame*, std::less<EvalFrame*>, ExpressionFastAlloc(EvalFrame*) > refs;

        Binding(EvalFrame *frame, Py::Object obj = Py::Object())
            :obj(obj),owner(frame)
        {
            ref(frame);
        }

        void ref(EvalFrame *frame) {
            refs.insert(frame);
        }
        void unref(EvalFrame *frame) {
            refs.erase(frame);
            if(refs.size()==0)
                delete this;
        }
        void operator delete(void *p) {
            if(p) ExpressionFastAlloc(Binding)().deallocate(reinterpret_cast<Binding*>(p),1);
        }
        static void *operator new(std::size_t) {
            return ExpressionFastAlloc(Binding)().allocate(1);
        }
    };
    typedef std::pair<const std::string, Binding*> Var;
    std::map<std::string,
            Binding*, 
            std::less<std::string>, 
            ExpressionFastAlloc(Var) > vars;

    Py::Object lastException;
    struct Exception {
        Exception(Base::Exception &e, PyObject *pyobj) {
            _EvalStack.back()->lastException = 
                Py::Object(PyObject_CallFunctionObjArgs(pyobj,Py::String(e.what()).ptr(),0),true);
        }
        ~Exception() {
            _EvalStack.back()->lastException = Py::Object();
        }
    };

    EvalFrame *lastCallFrame = 0;
    const char *funcName;
    const App::DocumentObject *funcOwner;

    std::bitset<32> flags;

#define EVAL_FRAME_FLAGS \
    EVAL_FRAME_FLAG(Warn1) \
    EVAL_FRAME_FLAG(Warn2) \
    EVAL_FRAME_FLAG(WarnNone) \
    EVAL_FRAME_FLAG(PythonMode) \
    EVAL_FRAME_FLAG(Pushed) \

    enum Flags{
#define EVAL_FRAME_FLAG(_name) F_##_name,
        EVAL_FRAME_FLAGS
    };

#undef EVAL_FRAME_FLAG
#define EVAL_FRAME_FLAG(_name) \
    bool _name() const {return flags.test(F_##_name);} \
    void set##_name(bool v=true) {flags.set(F_##_name,v);}

    EVAL_FRAME_FLAGS;

    EvalFrame(const char *name=0, const App::DocumentObject *owner=0)
        :funcName(name),funcOwner(owner)
    {}

    ~EvalFrame() {
        if(Pushed()) {
            assert(_EvalStack.size() && _EvalStack.back()==this);
            _EvalStack.pop_back();
            if(funcName)
                _EvalCallFrame = lastCallFrame;
        }
        for(auto &v : vars) {
            if(v.second)
                v.second->unref(this);
        }
    }

    bool isDocumentObjectVar(Py::Object &obj) {
        return PyObject_TypeCheck(obj.ptr(),&DocumentObjectPy::Type);
    }

    void push() {
        assert(!Pushed());
        setPushed();
        _EvalStack.push_back(this);
        if(funcName) {
            lastCallFrame = _EvalCallFrame;
            _EvalCallFrame = this;
        }
    }

    void erase(const Expression *owner, const std::string &name) {
        auto it = vars.find(name);
        if(it==vars.end())
            __EXPR_THROW(NameError,"Name '" << name << "' not found", owner);
        for(auto ref : it->second->refs) {
            if(ref!=this)
                ref->vars.erase(name);
        }
        delete it->second;
        vars.erase(it);
    }

    static PyObject *getBuiltin(const std::string &name) {
        static std::unordered_map<std::string,PyObject*> builtins;
        if(builtins.empty()) {
#if PY_MAJOR_VERSION < 3
            PyObject *pymod = PyImport_ImportModule("__builtin__");
#else
            PyObject *pymod = PyImport_ImportModule("builtins");
#endif
            if(!pymod) 
                Base::PyException::ThrowException();
            Py::Object mod(pymod,true);
            Py::Dict dict(mod.getAttr("__dict__"));
            for(auto it=dict.begin();it!=dict.end();++it) {
                const auto &value = *it;
                // WARNING! since these are built-in objects, we don't
                // keep a reference here, assuming they'll live till
                // the interpreter instance dies
                builtins[value.first.as_string()] = value.second.ptr();
            }
        }
        auto it = builtins.find(name);
        if(it == builtins.end())
            return nullptr;
        return it->second;
    }

    Py::Object *getLocalVar(const std::string &name) {
        auto iter = vars.find(name);
        if(iter == vars.end())
            return 0;
        return &iter->second->obj;
    }

    Py::Object *getVar(const Expression *owner, const std::string &name, BindType type) {
        Timing(getVar);
        auto &binding = vars[name];
        if(binding) {
            if(type>=BindNonLocal)
                __EXPR_THROW(NameError,"Invalid variable binding", owner);
            if(type == BindNonExist)
                return nullptr;
            if(type==BindLocalOnly && binding->owner!=this) {
                binding->refs.erase(this);
                binding = new Binding(this);
            }
            return &binding->obj;
        }
        switch(type) {
        case BindNonExist:
        case BindLocalOnly:
        case BindLocal:
            binding = new Binding(this);
            break;
        case BindExist:
        case BindQuery:
            assert(_EvalStack.size());
            for(size_t i=_EvalStack.size()-1;i>0;--i) {
                auto &frame = *_EvalStack[i-1];
                auto iter = frame.vars.find(name);
                if(iter!=frame.vars.end()) {
                    binding = new Binding(this,iter->second->obj);
                    break;
                }
            }
            if(_EvalStack.front()->PythonMode()) {
                PyObject *builtin = getBuiltin(name);
                if(builtin) {
                    // TODO: is it a good idea to bring built-in objects to
                    // local bindings? Python doesn't do that, but it will make
                    // next searching faster.
                    binding = new Binding(this,Py::Object(builtin));
                }
            }
            if(!binding) {
                if(type == BindExist)
                    __EXPR_THROW(NameError,"Name '" << name << "' not defined", owner);
                vars.erase(name);
                return nullptr;
            }
            break;
        case BindGlobal:
            if(_EvalStack.size()>1) {
                auto &top = *_EvalStack.front();
                auto &top_binding = top.vars[name];
                if(!top_binding) 
                    top_binding = new Binding(&top);
                top_binding->ref(this);
                binding = top_binding;
                break;
            }
            binding = new Binding(this);
            break;
        case BindNonLocal:
            for(size_t i=_EvalStack.size()-1;i>1;--i) {
                auto &frame = *_EvalStack[i-1];
                auto iter = frame.vars.find(name);
                if(iter == frame.vars.end())
                    continue;
                iter->second->ref(this);
                binding = iter->second;
                break;
            }
            if(!binding)
                __EXPR_THROW(NameError, "NonLocal binding of '" << name << "' not found.", owner);
            break;
        default:
            assert(0);
        }
        return &binding->obj;
    }

    void setVar(const Expression *owner, VarInfo &info) {
        if(info.component)
            info.component->set(owner,info.prefix,info.rhs);
        else if(info.lhs)
            *info.lhs = info.rhs;
    }

    bool canWarn(int code) {
        auto value = std::make_pair(funcName,code);
        auto it = _EvalWarned.find(value);
        if(it!=_EvalWarned.end())
           return false;
       _EvalWarned.insert(it,value); 
       return true;
    }

    static void warn(const Expression *expr, int code, PyObject *pyobj=0) {
        if(!_EvalCallFrame)
            return;
        assert(code>0 && code<=F_WarnNone);
        if(_EvalCallFrame->WarnNone())
            return;
        --code;

        if(_EvalCallFrame->flags.test(code))
            return;

        switch(code) {
        case F_Warn1:
            if(_EvalCallFrame->funcOwner
                    && _EvalCallFrame->canWarn(code))
                FC_WARN("Object property reference in function has no dependency tracking"
                        << std::endl << "in function: " 
                        << _EvalCallFrame->funcOwner->getFullName() << '.' 
                        << _EvalCallFrame->funcName << std::endl 
                        << "expression: " << expr->toStr());
            break;
        case F_Warn2:
            if(_EvalCallFrame->funcOwner 
                    && pyobj && PyObject_TypeCheck(pyobj,&DocumentObjectPy::Type)
                    && _EvalCallFrame->canWarn(code))
            {
                FC_WARN("Object property assignment may break dependency tracking"
                        << std::endl << "in function: " 
                        << _EvalCallFrame->funcOwner->getFullName() << '.' 
                        << _EvalCallFrame->funcName << std::endl 
                        << "expression: " << expr->toStr());
            }
            break;
        default:
            break;
        }
    }

    static void pragmaNoWarn(int code, bool enable) {
        if(!_EvalCallFrame)
            return;
        if(code==0)
            _EvalCallFrame->setWarnNone(enable);
        else if(code>0 && code<=F_WarnNone)
            _EvalCallFrame->flags.set(code-1,enable);
    }

#define EVAL_DEFAULT_LOOPCHECK 100
    int loopCheck = EVAL_DEFAULT_LOOPCHECK;

    static void pragmaLoopCheck(int v) {
        if(_EvalCallFrame)
            _EvalCallFrame->loopCheck = v;
    }

    static int getLookCheck() {
        return _EvalCallFrame?_EvalCallFrame->loopCheck:EVAL_DEFAULT_LOOPCHECK;
    }
};

//
// Expression base-class
//

TYPESYSTEM_SOURCE_ABSTRACT(App::Expression, Base::BaseClass);

Expression::Expression(const DocumentObject *_owner) 
    :owner(const_cast<App::DocumentObject*>(_owner)) 
{}

Expression::~Expression()
{
}

Expression::ComponentPtr Expression::createComponent(const std::string &n) {
    return ComponentPtr(new Component(n));
}

Expression::ComponentPtr Expression::createComponent(
        ExpressionPtr &&e1, ExpressionPtr &&e2, ExpressionPtr &&e3, bool isRange) 
{
    return ComponentPtr(new Component(std::move(e1),std::move(e2),std::move(e3),isRange));
}

int Expression::priority() const {
    return 20;
}

ExpressionPtr Expression::parse(const DocumentObject *owner, 
        const char *buffer, std::size_t len, bool verbose, bool pythonMode)
{
    return ExpressionParser::parse(owner, buffer, len, verbose, pythonMode);
}

ExpressionPtr Expression::parseUnit(const DocumentObject *owner, const char *buffer, std::size_t len)
{
    return ExpressionParser::parseUnit(owner, buffer, len);
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
        return ExpressionPtr();
    ImportSubNamesExpressionVisitor v(subNameMap);
    auto res = copy();
    res->visit(v);
    return res;
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
        return 0;
    std::vector<std::string> labels;
    getDepObjects(&labels);
    for(auto &label : labels) {
        // ref contains something like $label. and we need to strip '$' and '.'
        if(ref.compare(1,ref.size()-2,label)==0) {
            UpdateLabelExpressionVisitor v(obj,ref,newLabel);
            ExpressionPtr expr(copy());
            expr->visit(v);
            return expr;
        }
    }
    return 0;
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
            this->collectReplacement(e,pathes,parent,oldObj,newObj);
        else 
            this->renameObjectIdentifier(e,pathes,dummy);
    }

    const DocumentObject *parent;
    DocumentObject *oldObj;
    DocumentObject *newObj;
    ObjectIdentifier dummy;
    std::map<ObjectIdentifier, ObjectIdentifier> pathes;
    bool collect = true;
};

ExpressionPtr Expression::replaceObject(const DocumentObject *parent, 
        DocumentObject *oldObj, DocumentObject *newObj) const 
{
    ReplaceObjectExpressionVisitor v(parent,oldObj,newObj);

    // First pass, collect any changes. We have to const_cast it, as visit() is
    // not const. This is ugly...
    const_cast<Expression*>(this)->visit(v);

    if(v.pathes.empty())
        return 0;

    // Now make a copy and do the actual replacement
    ExpressionPtr expr(copy());
    v.collect = false;
    expr->visit(v);
    return expr;
}

App::any Expression::getValueAsAny(int options) const {
    Base::PyGILStateLocker lock;
    return pyObjectToAny(getPyValue(options));
}

Py::Object Expression::getPyValue(int options, int *jumpCode) const {
    if(options & OptionCallFrame) {
        options &= ~OptionCallFrame;
        EvalFrame frame;
        if(options & OptionPythonMode) {
            options &= ~OptionPythonMode;
            frame.setPythonMode();
        }
        frame.push();
        return getPyValue(options,jumpCode);
    }

    try {
        Py::Object pyobj = _getPyValue(jumpCode);
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

void Expression::addComponent(ComponentPtr &&component) {
    assert(component);
    components.push_back(std::move(component));
}

void Expression::visit(ExpressionVisitor &v) {
    _visit(v);
    for(auto &c : components)
        c->visit(v);
    v.visit(*this);
}

ExpressionPtr Expression::eval(int options) const {
    Base::PyGILStateLocker lock;
    return expressionFromPy(owner,getPyValue(options));
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

ExpressionPtr Expression::copy() const {
    auto expr = _copy();
    copy_vector(expr->components,components);
    expr->comment = comment;
    return expr;
}

//
// UnitExpression class
//

EXPR_TYPESYSTEM_SOURCE(App::UnitExpression, App::Expression);

UnitExpression::UnitExpression(const App::DocumentObject *_owner, 
        const Quantity &q, std::string &&unit)
    :Expression(_owner)
    ,quantity(q)
    ,unitStr(std::move(unit))
{}

UnitExpression::~UnitExpression() {
    if(cache) {
        Base::PyGILStateLocker lock;
        Py::_XDECREF(cache);
    }
}

ExpressionPtr UnitExpression::create(const App::DocumentObject *owner, 
        const Base::Quantity &quantity, std::string &&unitStr)
{
    return EXPR_NEW(UnitExpression,owner,quantity,std::move(unitStr));
}

ExpressionPtr UnitExpression::create(const App::DocumentObject *owner, 
        const Base::Quantity &quantity, const char *unit)
{
    return EXPR_NEW(UnitExpression,owner,quantity,std::string(unit?unit:""));
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
  * Simplify the expression. In this case, a NumberExpression is returned,
  * as it cannot be simplified any more.
  */

ExpressionPtr UnitExpression::simplify() const
{
    return NumberExpression::create(owner, quantity);
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

ExpressionPtr UnitExpression::_copy() const
{
    return UnitExpression::create(owner, quantity, std::string(unitStr));
}

Py::Object UnitExpression::_getPyValue(int *) const {
    if(!cache) 
        cache = Py::new_reference_to(pyFromQuantity(quantity));
    return Py::Object(cache);
}

//
// NumberExpression class
//

EXPR_TYPESYSTEM_SOURCE(App::NumberExpression, App::UnitExpression);

ExpressionPtr NumberExpression::create(const DocumentObject *owner, const Quantity &quantity) {
    return EXPR_NEW(NumberExpression,owner,quantity);
}

ExpressionPtr NumberExpression::create(const DocumentObject *owner, double fvalue) {
    return EXPR_NEW(NumberExpression,owner,Quantity(fvalue));
}

/**
  * Create and return a copy of the expression.
  */

ExpressionPtr NumberExpression::_copy() const
{
    return NumberExpression::create(owner, getQuantity());
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
    ss << std::setprecision(std::numeric_limits<double>::digits10 + 2) << getValue();

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

EXPR_TYPESYSTEM_SOURCE(App::OperatorExpression, App::Expression);

enum Operator {
    OP_NONE,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_FDIV,
    OP_MOD,
    OP_POW,
    OP_POW2,
    OP_EQ,
    OP_NEQ,
    OP_NE = OP_NEQ,
    OP_LT,
    OP_GT,
    OP_LTE,
    OP_LE = OP_LTE,
    OP_GTE,
    OP_GE = OP_GTE,
    OP_UNIT,
    OP_NEG,
    OP_POS,
    OP_AND,
    OP_OR,
    OP_IS,
    OP_IS_NOT,
    OP_NOT,
    OP_IN,
    OP_NOT_IN,
};

ExpressionPtr OperatorExpression::create(const App::DocumentObject *owner, 
        ExpressionPtr &&left, int op, ExpressionPtr &&right)
{
    _EXPR_NEW(res,OperatorExpression,owner);
    res->op = op;
    res->left = std::move(left);
    res->right = std::move(right);
    return _res;
}

/**
  * Determine whether the expression is touched or not, i.e relies on properties that are touched.
  */

bool OperatorExpression::isTouched() const
{
    return left->isTouched() || right->isTouched();
}

static Py::Object calc(const Expression *expr, int op,
                 Py::Object &l, const Expression *right, bool inplace) 
{
    Timing(calc);
    // check possible unary operation first
    switch(op) {
    case OP_NOT:
        return Py::Boolean(!l.isTrue());
    case OP_AND:
        if(!l.isTrue())
            return Py::False();
        break;
    case OP_OR: 
        if(l.isTrue())
            return Py::True();
        break;
    case OP_POS:{
        PyObject *res = PyNumber_Positive(l.ptr());
        if(!res) EXPR_PY_THROW(expr);
        return Py::asObject(res);
    }
    case OP_NEG:{
        PyObject *res = PyNumber_Negative(l.ptr());
        if(!res) EXPR_PY_THROW(expr);
        return Py::asObject(res);
    } default:
        break;
    }

    Py::Object r;
    {
        Timing(operatorExpression2);
        r = right->getPyValue();
    }

    _Timing(1,operatorExpression3);

    if(op==OP_AND || op==OP_OR)
        return Py::Boolean(r.isTrue());

    switch(op) {
    case OP_IS:
        return Py::Boolean(l.is(r));
    case OP_IS_NOT:
        return Py::Boolean(!l.is(r));
    case OP_IN:
    case OP_NOT_IN: {
        int res = PySequence_Contains(l.ptr(),r.ptr());
        if(res<0)
            EXPR_PY_THROW(expr);
        if(op==OP_NOT_IN)
            res = !res;
        return Py::Boolean(!!res);
    }
#define RICH_COMPARE(_op) \
    case OP_##_op: {\
        int res = PyObject_RichCompareBool(l.ptr(),r.ptr(),Py_##_op);\
        if(res<0) EXPR_PY_THROW(expr);\
        return Py::Boolean(!!res);\
    }
    RICH_COMPARE(LT)
    RICH_COMPARE(LE)
    RICH_COMPARE(GT)
    RICH_COMPARE(GE)
    RICH_COMPARE(EQ)
    RICH_COMPARE(NE)

#define _BINARY_OP(_op,_pyop) \
        res = inplace?PyNumber_InPlace##_pyop(l.ptr(),r.ptr()):\
                       PyNumber_##_pyop(l.ptr(),r.ptr());\
        if(!res) EXPR_PY_THROW(expr);\
        return Py::asObject(res);

#define BINARY_OP(_op,_pyop) \
    case OP_##_op: {\
        PyObject *res;\
        _BINARY_OP(_op,_pyop);\
    }

    BINARY_OP(SUB,Subtract)
    BINARY_OP(MUL,Multiply)
    BINARY_OP(UNIT,Multiply)
    BINARY_OP(DIV,TrueDivide)
    BINARY_OP(FDIV,FloorDivide)
    case OP_ADD: {
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
        _BINARY_OP(ADD,Add);
    }
    case OP_POW:
    case OP_POW2: {
        PyObject *res;
        if(inplace)
            res = PyNumber_InPlacePower(l.ptr(),r.ptr(),Py::None().ptr());
        else
            res = PyNumber_Power(l.ptr(),r.ptr(),Py::None().ptr());
        if(!res) EXPR_PY_THROW(expr);
        return Py::asObject(res);
    }
    case OP_MOD: {
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

/**
  * Evaluate the expression. Returns a new Expression with the result, or throws
  * an exception if something is wrong, i.e the expression cannot be evaluated.
  */

Py::Object OperatorExpression::_getPyValue(int *) const
{
    Py::Object value;
    {
        Timing(operatorExpression1);
        value = left->getPyValue();
    }
    return calc(this,op,value,right.get(),false);
}


/**
  * Simplify the expression. For OperatorExpressions, we return a NumberExpression if
  * both the left and right side can be simplified to NumberExpressions. In this case
  * we can calculate the final value of the expression.
  *
  * @returns Simplified expression.
  */

ExpressionPtr OperatorExpression::simplify() const
{
    auto v1 = left->simplify();
    auto v2 = right->simplify();

    // Both arguments reduced to numerics? Then evaluate and return answer
    if (freecad_dynamic_cast<NumberExpression>(v1.get()) && freecad_dynamic_cast<NumberExpression>(v2.get()))
        return eval();
    else
        return OperatorExpression::create(owner, std::move(v1), op, std::move(v2));
}

/**
  * Create a string representation of the expression.
  *
  * @returns A string representing the expression.
  */

void OperatorExpression::_toString(std::ostream &s, bool persistent,int) const
{
    bool needsParens;
    int leftOperator(OP_NONE), rightOperator(OP_NONE);

    needsParens = false;
    if (freecad_dynamic_cast<OperatorExpression>(left.get()))
        leftOperator = static_cast<OperatorExpression*>(left.get())->op;
    if (left->priority() < priority()) // Check on operator priority first
        needsParens = true;
    else if (leftOperator == op) { // Equal priority?
        if (!isLeftAssociative())
            needsParens = true;
        //else if (!isCommutative())
        //    needsParens = true;
    }

    switch (op) {
    case OP_NEG:
        s << "-" << (needsParens ? "(" : "") << left->toStr(persistent) << (needsParens ? ")" : "");
        return;
    case OP_POS:
        s << "+" << (needsParens ? "(" : "") << left->toStr(persistent) << (needsParens ? ")" : "");
        return;
    case OP_NOT:
        s << " not " << (needsParens ? "(" : "") << left->toStr(persistent) << (needsParens ? ")" : "");
        return;
    default:
        break;
    }

    if (needsParens)
        s << "(" << left->toStr(persistent) << ")";
    else
        s << left->toStr(persistent);

    switch (op) {
    case OP_AND:
        s << " and ";
        break;
    case OP_OR:
        s << " or ";
        break;
    case OP_ADD:
        s << " + ";
        break;
    case OP_SUB:
        s << " - ";
        break;
    case OP_MUL:
        s << " * ";
        break;
    case OP_DIV:
        s << " / ";
        break;
    case OP_FDIV:
        s << " // ";
        break;
    case OP_MOD:
        s << " % ";
        break;
    case OP_POW:
        s << " ^ ";
        break;
    case OP_POW2:
        s << " ** ";
        break;
    case OP_IS:
        s << " is ";
        break;
    case OP_IS_NOT:
        s << " is not ";
        break;
    case OP_IN:
        s << " in ";
        break;
    case OP_NOT_IN:
        s << " not in ";
        break;
    case OP_EQ:
        s << " == ";
        break;
    case OP_NEQ:
        s << " != ";
        break;
    case OP_LT:
        s << " < ";
        break;
    case OP_GT:
        s << " > ";
        break;
    case OP_LTE:
        s << " <= ";
        break;
    case OP_GTE:
        s << " >= ";
        break;
    case OP_UNIT:
        break;
    default:
        assert(0);
    }

    needsParens = false;
    if (freecad_dynamic_cast<OperatorExpression>(right.get()))
        rightOperator = static_cast<OperatorExpression*>(right.get())->op;
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

    if (needsParens)
        s << "(" << right->toStr(persistent) << ")";
    else
        s << right->toStr(persistent);
}

/**
  * A deep copy of the expression.
  */

ExpressionPtr OperatorExpression::_copy() const
{
    return OperatorExpression::create(owner, left->copy(), op, right->copy());
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
    case OP_OR:
        return 1;
        break;
    case OP_AND:
        return 2;
        break;
    case OP_IS:
    case OP_IS_NOT:
    case OP_IN:
    case OP_NOT_IN:
    case OP_EQ:
    case OP_NEQ:
        return 3;
        break;
    case OP_LT:
    case OP_GT:
    case OP_LTE:
    case OP_GTE:
        return 4;
    case OP_ADD:
    case OP_SUB:
        return 5;
    case OP_MOD:
    case OP_MUL:
    case OP_DIV:
    case OP_FDIV:
        return 6;
    case OP_UNIT:
        return 7;
    case OP_POW:
    case OP_POW2:
        return 8;
    case OP_NEG:
    case OP_POS:
    case OP_NOT:
        return 9;
    default:
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
    case OP_IS:
    case OP_IS_NOT:
    case OP_EQ:
    case OP_NEQ:
    case OP_ADD:
    case OP_MUL:
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
    case OP_ADD:
    case OP_MUL:
        return true;
    default:
        return false;
    }
}

//
// Internal class to manager user defined import modules
//
class ImportModules: public ParameterGrp::ObserverType {
public:
    ImportModules() {
        handle = GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/Expression/PyModules");
        handle->Attach(this);
        for(auto &m : handle->GetBoolMap()) {
            if(m.second)
                modules.emplace(m.first,nullptr);
        }
    }

    void OnChange(Base::Subject<const char*> &, const char* sReason) {
        if(!sReason)
            return;
        if(!handle->GetBool(sReason,false)) {
            auto it = modules.find(sReason);
            if(it!=modules.end()) {
                imports.erase(it->second);
                modules.erase(it);
            }
        }else
            modules.emplace(sReason,nullptr);
    }

    Py::Object getModule(const std::string &name, const Expression *e) {
        auto it = modules.find(name);
        if(it == modules.end())
            __EXPR_THROW(ImportError, "Python module '" << name << "' access denied.", e);
        if(it->second)
            return Py::Object(it->second);
        it->second = PyImport_ImportModule(name.c_str());
        if(!it->second) 
            EXPR_PY_THROW(e);
        imports.insert(it->second);
        // Not ref counted inside modules or ipmorts
        return Py::asObject(it->second);
    }

    bool isImported(PyObject *module) const {
        return imports.count(module);
    }

    static ImportModules *instance() {
        static ImportModules *inst;
        if(!inst)
            inst = new ImportModules;
        return inst;
    }

    ParameterGrp::handle getHandle() {
        return handle;
    }

private:
    std::unordered_map<std::string,PyObject*> modules;
    std::set<PyObject*> imports;
    ParameterGrp::handle handle;
};

class ImportParamLock: public ParameterLock {
public:
    ImportParamLock()
        :ParameterLock(ImportModules::instance()->getHandle())
    {}
};

//
// FunctionExpression class. This class handles functions with one or two parameters.
//

enum ExpressionFunctionType {
    FUNC_NONE,

    // Normal functions taking one or two arguments
    ACOS,
    ASIN,
    ATAN,
    ABS,
    EXP,
    LOG,
    LOG10,
    SIN,
    SINH,
    TAN,
    TANH,
    SQRT,
    COS,
    COSH,
    ATAN2,
    FMOD,
    FPOW,
    ROUND,
    TRUNC,
    CEIL,
    FLOOR,
    HYPOT,
    CATH,

    GET_VAR,
    HAS_VAR,
    IMPORT_PY,
    PRAGMA,

    CALLABLE_START,

    FUNC,
    FUNC_D,
    FUNC_PARSED,
    EVAL,

    CALLABLE_END,

    // Aggregates
    AGGREGATES,

    SUM,
    AVERAGE,
    STDDEV,
    COUNT,
    MIN,
    MAX,

    // Last one
    LAST,
};

static std::unordered_map<std::string, int> registered_functions; 
static std::unordered_map<int, std::string> registered_function_names; 

void init_functions() {
    bool inited = false;
    if(inited)
        return;
    inited = true;
    registered_functions["acos"] = ACOS;
    registered_functions["asin"] = ASIN;
    registered_functions["atan"] = ATAN;
    registered_functions["abs"] = ABS;
    registered_functions["exp"] = EXP;
    registered_functions["log"] = LOG;
    registered_functions["log10"] = LOG10;
    registered_functions["sin"] = SIN;
    registered_functions["sinh"] = SINH;
    registered_functions["tan"] = TAN;
    registered_functions["tanh"] = TANH;
    registered_functions["sqrt"] = SQRT;
    registered_functions["cos"] = COS;
    registered_functions["cosh"] = COSH;
    registered_functions["atan2"] = ATAN2;
    registered_functions["mod"] = FMOD;
    registered_functions["pow"] = FPOW;
    registered_functions["round"] = ROUND;
    registered_functions["trunc"] = TRUNC;
    registered_functions["ceil"] = CEIL;
    registered_functions["floor"] = FLOOR;
    registered_functions["hypot"] = HYPOT;
    registered_functions["cath"] = CATH;

    registered_functions["eval"] = EVAL;
    registered_functions["func"] = FUNC;
    registered_functions["func_d"] = FUNC_D;
    registered_functions["getvar"] = GET_VAR;
    registered_functions["hasvar"] = HAS_VAR;
    registered_functions["import_py"] = IMPORT_PY;
    registered_functions["pragma"] = PRAGMA;

    // Aggregates
    registered_functions["sum"] = SUM;
    registered_functions["count"] = COUNT;
    registered_functions["average"] = AVERAGE;
    registered_functions["stddev"] = STDDEV;
    registered_functions["min"] = MIN;
    registered_functions["max"] = MAX;

    for(auto &v : registered_functions)
        registered_function_names[v.second] = v.first;
}

EXPR_TYPESYSTEM_SOURCE(App::FunctionExpression, App::Expression);

ExpressionPtr FunctionExpression::create(const DocumentObject *owner, int f, ExpressionList &&args) {
    _EXPR_NEW(res,FunctionExpression,owner);
    res->args = std::move(args);
    res->f = f;
    return _res;
}

/**
  * Determinte whether the expressions is considered touched, i.e one or both of its arguments
  * are touched.
  *
  * @return True if touched, false if not.
  */

bool FunctionExpression::isTouched() const
{
    for(auto &a : args) {
        if(a->isTouched())
            return true;
    }
    return false;
}

/* Various collectors for aggregate functions */

class Collector {
public:
    Collector() : first(true) { }
    virtual ~Collector() {}

    virtual void collect(Quantity value) {
    ObjectIdentifier dummy;
        if (first)
            q.setUnit(value.getUnit());
    }
    virtual Quantity getQuantity() const {
        return q;
    }

#define COLLECTOR_ALLOC(_t) \
    static void *operator new(std::size_t) {\
        return ExpressionFastAlloc(_t)().allocate(1);\
    }\
    void operator delete(void *p){\
        if(p) ExpressionFastAlloc(_t)().deallocate(reinterpret_cast<_t*>(p),1);\
    }

    COLLECTOR_ALLOC(Collector)

protected:
    bool first;
    Quantity q;
};

class SumCollector : public Collector {
public:
    SumCollector() : Collector() { }
    COLLECTOR_ALLOC(SumCollector)

    void collect(Quantity value) {
        Collector::collect(value);
        q += value;
        first = false;
    }

};

class AverageCollector : public Collector {
public:
    AverageCollector() : Collector(), n(0) { }
    COLLECTOR_ALLOC(AverageCollector)

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
    COLLECTOR_ALLOC(StdDevCollector)

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
    COLLECTOR_ALLOC(CountCollector)

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
    COLLECTOR_ALLOC(MinCollector)

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
    COLLECTOR_ALLOC(MaxCollector)

    void collect(Quantity value) {
        Collector::collect(value);
        if (first || value > q)
            q = value;
        first = false;
    }
};

Py::Object FunctionExpression::evalAggregate(const Expression *owner, int f, const ExpressionList &args)
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

Py::Object FunctionExpression::evaluate(const Expression *expr, int f, const ExpressionList &args) 
{
    if(!expr || !expr->getOwner())
        _EXPR_THROW("Invalid owner.", expr);

    if(args.empty())
        _EXPR_THROW("Function requires at least one argument.",expr);

    // Handle aggregate functions
    if (f > AGGREGATES)
        return evalAggregate(expr, f, args);

    switch(f) {
    case GET_VAR: 
        if(args.size()<2)
            _EXPR_THROW("Function expects 2 or 3 arguments.",expr);
        // fall through
    case HAS_VAR: {
        Py::Object value = args[0]->getPyValue();
        if(!value.isString())
            _EXPR_THROW("Expects the first argument evaluating to a string.",expr);
        Py::Object pyobj;
        bool found = Base::Interpreter().getVariable(value.as_string().c_str(),pyobj);
        if(f == HAS_VAR)
            return Py::Boolean(found);
#if 1
        // getvar() may pose as a security problem. Disable it for now.
        __EXPR_THROW(Base::NotImplementedError, "getvar() is disabled.", expr);
#else
        if(!found) {
            if(args.size()==2)
                return args[1]->getPyValue();
            _EXPR_THROW("Variable not found.",expr);
        }
        return pyobj;
#endif
    } default:
        break;
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
    case FMOD:
        if (e2.isNone())
            _EXPR_THROW("Invalid second argument.",expr);
        unit = v1.getUnit() / v2.getUnit();
        break;
    case FPOW: {
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
    case FMOD: {
        output = fmod(value, v2.getValue());
        break;
    }
    case ATAN2: {
        output = atan2(value, v2.getValue());
        break;
    }
    case FPOW: {
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

    return Py::Object(new QuantityPy(new Quantity(scaler * output, unit)));
}

Py::Object FunctionExpression::_getPyValue(int *) const {
    return evaluate(this,f,args);
}

/**
  * Try to simplify the expression, i.e calculate all constant expressions.
  *
  * @returns A simplified expression.
  */

ExpressionPtr FunctionExpression::simplify() const
{
    size_t numerics = 0;
    ExpressionList a;

    // Try to simplify each argument to function
    for (auto &arg : args) {
        auto v = arg->simplify();
        if (freecad_dynamic_cast<NumberExpression>(v.get()))
            ++numerics;
        a.push_back(std::move(v));
    }

    if (numerics == args.size()) {
        // All constants, then evaluation must also be constant
        return eval();
    }
    else
        return create(owner, f, std::move(a));
}

/**
  * Create a string representation of the expression.
  *
  * @returns A string representing the expression.
  */

void FunctionExpression::_toString(std::ostream &ss, bool persistent,int) const
{
    init_functions();
    auto it = registered_function_names.find(f);
    assert(it!=registered_function_names.end());
    ss << it->second << '(';
    for (size_t i = 0; i < args.size(); ++i) {
        ss << args[i]->toStr(persistent);
        if (i != args.size() - 1)
            ss << ", ";
    }
    ss << ')';
}

/**
  * Create a copy of the expression.
  *
  * @returns A deep copy of the expression.
  */

ExpressionPtr FunctionExpression::_copy() const
{
    _EXPR_NEW(res, FunctionExpression,owner);
    res->f = f;
    copy_vector(res->args,args);
    return _res;
}

void FunctionExpression::_visit(ExpressionVisitor &v) {
    for(auto &arg:args)
        arg->visit(v);
}

///////////////////////////////////////////////////////////////////////////////////////

EXPR_TYPESYSTEM_SOURCE(App::AssignmentExpression, App::Expression);

ExpressionPtr AssignmentExpression::create(const App::DocumentObject *owner, 
        int catchAll, ExpressionList &&left, ExpressionList &&rights, int op)
{
    assert(left.size() && rights.size());
    if(op && catchAll>=0)
        PARSER_THROW("Invalid catch all target");

    _EXPR_NEW(res,AssignmentExpression,owner);
    res->catchAll = catchAll;
    res->left = std::move(left);
    res->op = op;
    if(rights.size()==1) {
        res->right = std::move(rights[0]);
    } else {
        res->right = TupleExpression::create(owner,std::move(rights));
        res->rightTuple = true;
    }
    return _res;
}

ExpressionPtr AssignmentExpression::create(const App::DocumentObject *owner, 
        ExpressionPtr &&left, ExpressionPtr &&right, int op)
{
    ExpressionList lefts;
    lefts.push_back(std::move(left));
    ExpressionList rights;
    rights.push_back(std::move(right));
    return create(owner,-1,std::move(lefts),std::move(rights),op);
}

ExpressionPtr AssignmentExpression::_copy() const {
    _EXPR_NEW(res,AssignmentExpression,owner);
    res->op = op;
    copy_vector(res->left,left);
    res->right = right->copy();
    res->rightTuple = rightTuple;
    res->catchAll = catchAll;
    return _res;
}

void AssignmentExpression::_visit(ExpressionVisitor &v) {
    for(auto &e : left)
        e->visit(v);
    right->visit(v);
}

bool AssignmentExpression::isTouched() const {
    for(auto &e : left) {
        if(e->isTouched())
            return true;
    }
    if(right->isTouched())
        return true;
    return false;
}

void AssignmentExpression::_toString(std::ostream &ss, bool persistent,int) const {
    bool first = true;
    int i=0;
    for(auto &e : left) {
        if(first)
            first = false;
        else
            ss << ", ";
        if(i++ == catchAll)
            ss << '*';
        ss << e->toStr(persistent);
    }
    switch(op) {
    case OP_ADD:
        ss << " += ";
        break;
    case OP_SUB:
        ss << " -= ";
        break;
    case OP_MUL:
        ss << " *= ";
        break;
    case OP_DIV:
        ss << " /= ";
        break;
    case OP_MOD:
        ss << " %= ";
        break;
    case OP_POW:
        ss << " **= ";
        break;
    case OP_FDIV:
        ss << " //= ";
        break;
    default:
        ss << " = ";
    }
    if(rightTuple) {
        assert(right->isDerivedFrom(TupleExpression::getClassTypeId()));
        static_cast<TupleExpression*>(right.get())->printItems(ss,persistent);
    }else
        ss << right->toStr(persistent);
}

class PyIterable : public Py::Object {
public:
    PyIterable(const Py::Object &obj, const Expression *expr, bool toSeq)
        :Py::Object() ,i(-1)
    {
        const char *msg = "Object not iterable.";
        if(obj.isSequence()) {
            set(PySequence_Fast(obj.ptr(),msg));
            i = 0;
        }else if(!PyIter_Check(obj.ptr())) {
            __EXPR_THROW(TypeError,msg,expr);
        } else if(toSeq) {
            set(PySequence_Fast(obj.ptr(),msg),true);
            i = 0;
        }else
            set(PyObject_GetIter(obj.ptr()),true);
    }
    bool next(Py::Object &obj,Expression *expr) {
        if(i>=0) {
            if(i>=(int)PySequence_Fast_GET_SIZE(ptr()))
                return false;
            obj = PySequence_Fast_GET_ITEM(ptr(),i++);
            return true;
        }
        PyObject *item = PyIter_Next(ptr());
        if(!item) {
            if(PyErr_Occurred())
                EXPR_PY_THROW(expr);
            return false;
        }
        obj = Py::asObject(item);
        return true;
    }
private:
    int i;
};

void AssignmentExpression::assign(const Expression *owner, const Expression *left, PyObject *right) {
    auto &frame = *_EvalStack.back();
    auto list = freecad_dynamic_cast<ListExpression>(left);
    if(list) {
        int catchAll = -1;
        int i=-1;
        for(bool flag : list->flags) {
            ++i;
            if(!flag) continue;
            if(catchAll >= 0)
                _EXPR_THROW("Too many catch all", list);
            catchAll = i;
        }
        auto r = PyObjectExpression::create(owner->getOwner(),right);
        AssignmentExpression::apply(owner,catchAll,list->items,r.get());
    }else if(left->isDerivedFrom(VariableExpression::getClassTypeId())) {
        auto e = static_cast<const VariableExpression*>(left);
        VarInfo info(e->push(owner,false));
        info.rhs = right;
        frame.setVar(owner,info);
    }else if(left->isDerivedFrom(CallableExpression::getClassTypeId())) {
        auto e = static_cast<const CallableExpression*>(left);
        VarInfo info = e->getVarInfo(false);
        info.rhs = right;
        frame.setVar(owner,info);
    }else
        _EXPR_THROW("Invalid left expression in assignment",owner);
}

Py::Object AssignmentExpression::apply(const Expression *owner, int _catchAll, 
        const ExpressionList &left, const Expression *right, int op)
{
    CHECK_STACK(AssignmentExpression, owner);

    auto &frame = *_EvalStack.back();
    if(left.size()==1) {
        VarInfo info;
        if(left[0]->isDerivedFrom(VariableExpression::getClassTypeId())) {
            auto e = static_cast<VariableExpression*>(left[0].get());
            info = e->push(owner,!!op);
        }else if(left[0]->isDerivedFrom(CallableExpression::getClassTypeId())) {
            auto e = static_cast<CallableExpression*>(left[0].get());
            info = e->getVarInfo(!!op);
        }else
            _EXPR_THROW("Invalid assignement",owner);
        if(!op) 
            info.rhs = right->getPyValue();
        else {
            if(info.lhs && info.lhs->ptr()==info.rhs.ptr()) {
                // release reference for possible faster in place operation
                *info.lhs = Py::Object();
            }
            info.rhs = calc(owner, op, info.rhs, right, true);
        }
        frame.setVar(owner,info);
        return info.rhs;
    }

    if(op)
        _EXPR_THROW("Invalid argumented assignement",owner);

    PyIterable value(right->getPyValue(),owner,true);
    Py::Sequence seq(value);
    size_t catchAll;
    if(_catchAll < 0) {
        catchAll = left.size();
        if(seq.size()>left.size())
            _EXPR_THROW("Too many values to unpack",owner);
        if(seq.size()<left.size())
            _EXPR_THROW("Too few values to unpack",owner);
    }else if(seq.size()+1<left.size())
        _EXPR_THROW("Too few values to unpack",owner);
    else
        catchAll = (size_t)_catchAll;
    int j=0;
    for(size_t i=0;i<catchAll;++i)
        assign(owner,left[i].get(),seq[j++].ptr());

    if(catchAll < left.size()) {
        Py::List list(seq.size()-left.size()+1);
        for(size_t i=0;i<list.size();++i) 
            list.setItem(i,seq[j++]);
        auto e = freecad_dynamic_cast<VariableExpression>(left[catchAll].get());
        if(!e) _EXPR_THROW("Invalid catch all in assignment",owner);
        VarInfo info(e->push(owner,false));
        info.rhs = list;
        frame.setVar(owner,info);

        for(size_t i=catchAll+1;i<left.size();++i) 
            assign(owner,left[i].get(),seq[j++].ptr());
    }
    return value;
}

Py::Object AssignmentExpression::_getPyValue(int *) const {
    return apply(this,catchAll,left,right.get(),op);
}

//
// VariableExpression class
//

EXPR_TYPESYSTEM_SOURCE(App::VariableExpression, App::Expression);

ExpressionPtr VariableExpression::create(const DocumentObject *owner, ObjectIdentifier &&var) {
    _EXPR_NEW(res,VariableExpression,owner);
    res->var = std::move(var);
    return _res;
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
  * Return a copy of the expression.
  */

ExpressionPtr VariableExpression::_copy() const
{
    _EXPR_NEW(res, VariableExpression,owner);
    res->var = var;
    return _res;
}

std::string VariableExpression::name() const {
    return var.getPropertyName();
}

const ObjectIdentifier &VariableExpression::getPath() const {
    return var;
}

Py::Object VariableExpression::_getPyValue(int *) const {
    if(_EvalStack.size() && !var.isLocalProperty() && !var.hasDocumentObjectName(true)) {
        auto &frame = *_EvalStack.back();
        const auto &comps = var.getComponents();
        assert(comps.size());
        auto v = frame.getVar(this,comps[0].getName(),BindQuery);
        if(v) {
            if(comps.size()==1 && components.empty())
                return *v;
            VarInfo info(*v);
            setVarInfo(info,true,true);
            return info.rhs;
        }
    }

    bool isPseudoProperty = false;
    auto res = var.getPyValue(true, &isPseudoProperty);

    if(!var.isLocalProperty() || !isPseudoProperty)
        EvalFrame::warn(this,1);
    return res;
}

void VariableExpression::addComponent(ComponentPtr &&c) {
    do {
        if(components.size())
            break;
        if(!c->e1 && !c->e2) {
            var << c->comp;
            return;
        }
        long l1=0,l2=0,l3=1;
        if(c->e3) {
            auto n3 = freecad_dynamic_cast<NumberExpression>(c->e3.get());
            if(!n3 || !essentiallyEqual(n3->getValue(),(double)l3))
                break;
        }
        if(c->e1) {
            auto n1 = freecad_dynamic_cast<NumberExpression>(c->e1.get());
            if(!n1) {
                if(c->e2 || c->e3)
                    break;
                auto s = freecad_dynamic_cast<StringExpression>(c->e1.get());
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
        auto n2 = freecad_dynamic_cast<NumberExpression>(c->e2.get());
        if(n2 && essentiallyInteger(n2->getValue(),l2)) {
            var << ObjectIdentifier::RangeComponent(l1,l2,l3);
            return;
        }
    }while(0);

    Expression::addComponent(std::move(c));
}

bool VariableExpression::_isIndexable() const {
    return true;
}

void VariableExpression::_toString(std::ostream &ss, bool persistent,int) const {
    if(persistent)
        ss << var.toPersistentString();
    else
        ss << var.toString();
}

VarInfo VariableExpression::push(const Expression *owner, bool mustExist, std::string *name) const 
{
    (void)owner;
    assert(_EvalStack.size());
    auto &frame = *_EvalStack.back();
    const auto &comps = var.getComponents();
    assert(comps.size());

    bool hasComponents = comps.size()>1 || components.size();
    BindType type = mustExist||hasComponents?BindExist:BindLocal;

    if(name)
        *name = comps[0].getName();

    VarInfo info(*frame.getVar(this,comps[0].getName(),type));
    setVarInfo(info,mustExist);
    return info;
}

void VariableExpression::setVarInfo(VarInfo &info, bool mustExist, bool noassign) const{
    Timing(setVarInfo);
    const auto &comps = var.getComponents();
    if(comps.size()==1 && components.empty())
        return;
    size_t count = comps.size();
    if(components.empty())
        --count;
    size_t i=1;
    for(;i<count;++i) {
        if(!noassign)
            EvalFrame::warn(this, 2, *info.rhs);
        info.rhs = comps[i].get(info.rhs);
    }
    if(components.empty()) {
        info.prefix = info.rhs;
        if(!noassign)
            EvalFrame::warn(this, 2, *info.rhs);
        if(mustExist)
            info.rhs = comps[i].get(info.rhs);
        if(!noassign)
            info.component.reset(new Component(comps[i]));
    }else{
        count = components.size()-1;
        for(i=0;i<count;++i) {
            if(!noassign)
                EvalFrame::warn(this, 2, *info.rhs);
            info.rhs = components[i]->get(this,info.rhs);
        }
        info.prefix = info.rhs;
        if(!noassign)
            EvalFrame::warn(this, 2, *info.rhs);
        if(mustExist)
            info.rhs = components[i]->get(this,info.rhs);
        if(!noassign)
            info.component.reset(new Component(*components[i]));
    }
}

// To disable dependency tracking inside function statement
static int _FunctionDepth;
class FunctionDepth {
public:
    FunctionDepth() {
        ++_FunctionDepth;
    }
    ~FunctionDepth() {
        --_FunctionDepth;
    }
};

void VariableExpression::_getDeps(ExpressionDeps &deps) const
{
    if(_FunctionDepth)
        return;
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
        std::map<ObjectIdentifier,ObjectIdentifier> &pathes,
        const App::DocumentObject *parent, 
        App::DocumentObject *oldObj, 
        App::DocumentObject *newObj) const
{
    ObjectIdentifier path;
    if(var.replaceObject(path,parent,oldObj,newObj))
        pathes[var.canonicalPath()] = std::move(path);
}

void VariableExpression::_moveCells(const CellAddress &address, 
        int rowCount, int colCount, ExpressionVisitor &v) 
{
    if(var.hasDocumentObjectName(true))
        return;

    auto &comp = var.getPropertyComponent(0);
    CellAddress addr = stringToAddress(comp.getName().c_str(),true);
    if(!addr.isValid())
        return;

    int thisRow = addr.row();
    int thisCol = addr.col();
    if (thisRow >= address.row() || thisCol >= address.col()) {
        v.aboutToChange();
        addr.setRow(thisRow + rowCount);
        addr.setCol(thisCol + colCount);
        comp = ObjectIdentifier::SimpleComponent(addr.toString());
    }
}

void VariableExpression::_offsetCells(int rowOffset, int colOffset, ExpressionVisitor &v) {
    if(var.hasDocumentObjectName(true))
        return;

    auto &comp = var.getPropertyComponent(0);
    CellAddress addr = stringToAddress(comp.getName().c_str(),true);
    if(!addr.isValid() || (addr.isAbsoluteCol() && addr.isAbsoluteRow()))
        return;

    v.aboutToChange();
    if(!addr.isAbsoluteCol())
        addr.setCol(addr.col()+colOffset);
    if(!addr.isAbsoluteRow())
        addr.setRow(addr.row()+rowOffset);
    comp = ObjectIdentifier::SimpleComponent(addr.toString());
}

//
// CallableExpression class
//

EXPR_TYPESYSTEM_SOURCE(App::CallableExpression, App::Expression);

ExpressionPtr CallableExpression::create(const App::DocumentObject *owner, std::string &&name,
        StringList &&names, ExpressionList &&args)
{
    assert(names.size() == args.size());
    ObjectIdentifier path(owner);
    path << ObjectIdentifier::SimpleComponent(std::move(name));
    return create(owner,std::move(path),std::move(names),std::move(args));
}

ExpressionPtr CallableExpression::create(const DocumentObject *owner, 
        ObjectIdentifier &&path, StringList &&names, ExpressionList &&args)
{
    assert(names.size() == args.size());
    if(path.numComponents()!=1 || path.hasDocumentObjectName())
        return CallableExpression::create(owner,
                VariableExpression::create(owner,std::move(path)),std::move(names),std::move(args));

    const std::string &name = path.getComponents()[0].getName();
    init_functions();
    auto it = registered_functions.find(name);

    if(it == registered_functions.end()) {
        return CallableExpression::create(owner,
                VariableExpression::create(owner,std::move(path)), std::move(names),std::move(args));
    }
    if(it->second < CALLABLE_START || it->second > CALLABLE_END) {
        for(auto &n : names) {
            if(n.size()) {
                PARSER_THROW("Function '" << name << "' does not support named argument.");
            }
        }
    }
    return create(owner,ExpressionPtr(),std::move(names),std::move(args),
            it->second,std::string(it->first));

}

ExpressionPtr CallableExpression::create(const DocumentObject *owner, ExpressionPtr &&expr, 
        StringList &&names, ExpressionList &&args, int ftype, std::string &&name, bool checkArgs)
{
#define FUNC_NAME "'" << (name.size()?name.c_str():"unnamed") << "'"
    assert(names.size() == args.size());
    if(checkArgs) {
        StringSet nameSet;
        bool hasKeyword = false;
        for(auto &n : names) {
            if(n.size() && n!="*")  {
                hasKeyword = true;
                if(n!="**" && !nameSet.insert(n).second)
                    PARSER_THROW("Duplicate keyword arg '" << n << "'");
            } else if(hasKeyword)
                PARSER_THROW("None keyword arg '" << n << "' after keyword arg");
        }
    }
    _EXPR_NEW(res,CallableExpression,owner);
    res->expr = std::move(expr);
    res->names = std::move(names);
    res->args = std::move(args);
    res->ftype = ftype;
    res->name = std::move(name);
    return _res;
}

VarInfo CallableExpression::getVarInfo(bool mustExist) const {
    VarInfo info;
    info.rhs = _getPyValue();
    if(components.size()) {
        std::size_t i;
        std::size_t count = components.size()-1;
        for(i=0;i<count;++i) {
            EvalFrame::warn(this, 2, *info.rhs);
            info.rhs = components[i]->get(this,info.rhs);
        }
        EvalFrame::warn(this, 2, *info.rhs);
        info.prefix = info.rhs;
        if(mustExist)
            info.rhs = components[i]->get(this,info.rhs);
        info.component.reset(new Component(*components[i]));
    }
    return info;
}

void CallableExpression::_toString(std::ostream &ss, bool persistent,int) const {
    if(expr)
        ss << expr->toStr(persistent);
    else 
        ss << name;
    ss << '(';
    bool first = true;
    int i=0;
    for(auto &arg : args) {
        auto &name = names[i++];
        if(first)
            first = false;
        else
            ss << ", ";
        if(name.size()) {
            ss << name;
            if(name != "*" && name!="**")
                ss << '=';
        }
        ss << arg->toStr(persistent);
    }
    ss << ')';
}

std::string CallableExpression::getDocString() const {
    if(ftype==FUNC_PARSED && 
       expr && expr->isDerivedFrom(SimpleStatement::getClassTypeId()))
    {
        auto e = static_cast<SimpleStatement*>(expr.get());
        if(e->getSize()) {
            auto estr = freecad_dynamic_cast<const StringExpression>(e->getExpr(0));
            if(estr)
                return estr->getText();
        }
    }
    return std::string();
}

bool CallableExpression::isTouched() const
{
    if(expr && expr->isTouched())
        return true;
    for(auto &arg : args) {
        if(arg->isTouched())
            return true;
    }
    return false;
}

void CallableExpression::_visit(ExpressionVisitor &v) {
    for(auto &arg : args)
        arg->visit(v);
    if(expr)
        expr->visit(v);
}

static inline std::string varName(int index) {
    return std::string("_") + std::to_string(index) + "_";
}

static Expression::StringList prepareCommands(const Expression *owner, const Py::Object &arg0) {
    Expression::StringList cmds;
    if(arg0.isString()) 
        cmds.push_back(arg0.as_string());
    else {
        PyIterable pyobj(arg0,owner,true);
        Py::Sequence seq(pyobj);
        if(!seq.size())
            return cmds;
        cmds.reserve(seq.size());
        for(size_t i=0;i<seq.size();++i) {
            Py::Object item(seq[i]);
            if(!item.isString())
                _EXPR_THROW("Non string command in sequence",owner);
            cmds.emplace_back(item.as_string());
        }
    }
    return cmds;
}

static void prepareArguments(const Expression *owner, EvalFrame &frame, 
        const Expression::StringList &names, const Expression::ExpressionList &args, size_t start=1)
{
    (void)owner;
    int idx = 1;
    for(size_t j=start;j<args.size();++j) {
        auto &name = names[j];
        auto arg = args[j].get();
        if(!arg)
            continue;
        if(name == "*") {
            PyIterable pyobj(arg->getPyValue(),owner,true);
            Py::Sequence seq(pyobj);
            for(size_t i=0;i<seq.size();++i)
                *frame.getVar(owner,varName(idx++),BindLocalOnly) = Py::Object(seq[i]);
        }else if(name == "**") {
            Py::Object pyobj = arg->getPyValue();
            if(!pyobj.isMapping())
                _EXPR_THROW("Expects Python mapping.", arg);
            Py::Mapping mapping(pyobj);
            for(auto it=mapping.begin();it!=mapping.end();++it) {
                const auto &value = *it;
                if(!value.first.isString())
                    _EXPR_THROW("Only accepts string as key.", arg);
                *frame.getVar(owner,value.first.as_string(),BindLocalOnly) = Py::Object(value.second);
            }
        } else if(name.size()) {
            if(name[0] == '*')
                return;
            auto var = frame.getVar(owner,name,BindNonExist);
            if(var)
                *var = arg->getPyValue();
        } else {
            auto var = frame.getVar(owner,varName(idx++),BindNonExist);
            if(var)
                *var = arg->getPyValue();
        }
    }
}

enum JumpType {
    JUMP_NONE,
    JUMP_RETURN,
    JUMP_BREAK,
    JUMP_CONTINUE,
    JUMP_RAISE,
};

Py::Object CallableExpression::evaluate(PyObject *pyargs, PyObject *pykwds) {

    if(ftype!=FUNC_PARSED)
        PY_THROW("Unexpected callable expression type: " << ftype);
    if(!expr)
        PY_THROW("Invalid callable expression");

    EvalFrame frame(name.c_str(),getOwner());

    const char *tupleName = 0;
    const char *dictName = 0;
    if(names.size()>=1) {
        auto &lastName = names[names.size()-1];
        if(lastName[0] == '*') {
            if(lastName[1] != '*')
                tupleName = lastName.c_str()+1;
            else {
                dictName = lastName.c_str()+2;
                if(names.size()>1 && names[names.size()-2][0] == '*')
                    tupleName = names[names.size()-2].c_str()+1;
            }
        }
    }
    Py::Tuple tupleArgs;
    Py::Dict dictArgs;

    size_t i=0;
    if(pyargs) {
        Py::Sequence seq(pyargs);
        if(names.empty() && seq.size()) {
            PY_THROW("Function " << FUNC_NAME << " does accept any arg");
        }
        for(i=0;i<seq.size();++i) {
            if(i>=names.size())
                PY_THROW("Too many args when calling " << FUNC_NAME);
            auto &name = names[i];
            if(name[0] != '*') {
                *frame.getVar(this,name,BindLocalOnly) = Py::Object(seq[i]);
                continue;
            }
            if(name[1]=='*')
                PY_THROW("Too many args when calling " << FUNC_NAME);
            tupleArgs = Py::Tuple(seq.size()-i);
            for(size_t j=0;i<seq.size();++i,++j)
                tupleArgs.setItem(j,seq[i]);
            break;
        }
    }

    if(!pykwds) {
        if(i<args.size() && !args[i])
            PY_THROW("Missing arg '" << names[i] << "' when calling " << FUNC_NAME);
        prepareArguments(this,frame,names,args,i);
    }else{
        Py::Dict dict(pykwds);
        for(size_t j=0;j<i;++j) {
            if(dict.hasKey(names[j]))
                PY_THROW("Multiple value of keyword arg '" 
                        << names[j] << "' when calling " << FUNC_NAME);
        }
        for(;i<args.size();++i) {
            if(names[i][0]=='*')
                break;
            if(dict.hasKey(names[i]))
                frame.getVar(this,names[i],BindLocalOnly);
            else if(!args[i]) 
                PY_THROW("Missing arg '" << names[i] << "' when calling " << FUNC_NAME);
            else
                *frame.getVar(this,names[i],BindLocalOnly) = args[i]->getPyValue();
        }
        for(auto it=dict.begin();it!=dict.end();++it){
            const auto &value = (*it);
            std::string key = value.first.as_string();
            auto itV = frame.vars.find(key);
            if(itV != frame.vars.end()) {
                itV->second->obj = Py::Object(value.second);
                continue;
            }
            if(!dictName)
                PY_THROW("Unknown keyword arg '" << key << "' when calling " << FUNC_NAME);
            dictArgs.setItem(key,value.second);
        }
    }
    if(dictName)
        *frame.getVar(this,dictName,BindLocalOnly) = dictArgs;
    if(tupleName) 
        *frame.getVar(this,tupleName,BindLocalOnly) = tupleArgs;

    frame.push();

    int jumpCode = 0;
    auto res = expr->getPyValue(0,&jumpCode);
    switch(jumpCode) {
    case JUMP_RETURN:
    case JUMP_NONE:
        break;
    case JUMP_BREAK:
            PY_THROW("Unmatched 'break' statement in function " << FUNC_NAME);
        case JUMP_CONTINUE:
            PY_THROW("Unmatched 'continue' statement in function" << FUNC_NAME);
        default:
            PY_THROW("Unexpected end of function " << FUNC_NAME);
    }
    return res;
}

Py::Object CallableExpression::_getPyValue(int *) const {
    if(!expr && ftype == EVAL) {
        if(args.size()<1)
            EXPR_THROW("Expects at least one arg");

        EvalFrame frame;
        prepareArguments(this,frame,names,args,names[0].empty()?1:0);
        frame.push();

        StringList cmds;
        if(names[0].empty())
            cmds = prepareCommands(this,args[0]->getPyValue());
        else {
            auto pycmd = frame.getLocalVar("cmd");
            if(!pycmd) 
                EXPR_THROW("eval() excepts the command argument to be named as 'cmd'");
            cmds = prepareCommands(this,*pycmd);
        }
        Py::Object res;
        for(auto &cmd : cmds) {
            int jumpCode = 0;
            res = Expression::parse(owner,cmd,true)->getPyValue(0,&jumpCode);
            switch(jumpCode) {
            case JUMP_RETURN:
                return res;
            case JUMP_NONE:
                continue;
            case JUMP_BREAK:
                EXPR_THROW("Unmatched 'break' statement.");
            case JUMP_CONTINUE:
                EXPR_THROW("Unmatched 'continue' statement.");
            default:
                EXPR_THROW("Unexpected end of 'eval'.");
            }
        }
        return res;
    }

    Py::Object *pyCallable = 0;
    if(!expr && ftype!=EVAL && _EvalStack.size() && _EvalStack.front()->PythonMode()) {
        auto &frame = *_EvalStack.back();
        pyCallable = frame.getVar(this,name.c_str(),BindQuery);
        if(pyCallable && !pyCallable->isCallable()) 
            pyCallable = 0;
    }
    if(!expr && !pyCallable) {
        switch(ftype) {
        case FUNC: 
        case FUNC_D: {

            if(args.size()<1)
                EXPR_THROW("Expects at least one argument");

            // parse but not evaluate the expressions, and then wrap them
            // inside a python callable 

            auto statement = Statement::create(owner,ExpressionPtr());
            for(auto &cmd : prepareCommands(this,args[0]->getPyValue()))
                static_cast<Statement&>(*statement).add(parse(owner,cmd,true));

            _EXPR_NEW(res,CallableExpression,owner);
            res->ftype = FUNC_PARSED;
            res->names.reserve(args.size());
            res->args.reserve(args.size());
            res->expr = std::move(statement);
            if(ftype == FUNC_D) {
                // Delay argument evaluation until the function is evaluated.
                // Just wrap them as it is now.
                for(size_t i=1;i<args.size();++i) {
                    res->names.push_back(names[i]);
                    res->args.push_back(args[i]->copy());
                }
            }else {
                // Evaluate the arguments now
                EvalFrame frame;
                prepareArguments(this,frame,names,args);
                for(auto &v : frame.vars) {
                    res->names.push_back(v.first);
                    res->args.push_back(PyObjectExpression::create(owner,*v.second->obj));
                }
            }
            return Py::Object(new ExpressionPy(_res.release()));

        } case IMPORT_PY: {
            Py::Object value(args[0]->getPyValue());
            if(!value.isString())
                EXPR_THROW("Function expects the first argument to be a string.");
            return ImportModules::instance()->getModule(value.as_string(),this);
        } case PRAGMA: {
            Py::Object value(args[0]->getPyValue());
            if(!value.isString())
                EXPR_THROW("Function expects the first argument to be a string.");

            bool hasArg2 = false;
            long arg2;
            if(args.size() > 1) {
                Py::Object value(args[1]->getPyValue());
                if(value.isNumeric()) {
                    arg2 = Py::Long(value);
                    hasArg2 = true;
                }
            }
            if(!hasArg2)
                EXPR_THROW("Function expects the second argument to be an integer.");
            std::string name = value.as_string();
            if(boost::equals(name,"nowarn"))
                EvalFrame::pragmaNoWarn(arg2,true);
            else if(boost::equals(name,"warn")) 
                EvalFrame::pragmaNoWarn(arg2,false);
            else if(boost::equals(name,"loop_check"))
                EvalFrame::pragmaLoopCheck(arg2);
            return Py::Object();
        } default:
            return FunctionExpression::evaluate(this,ftype,args);
        }
    }

    Py::Object pyobj;
    if(pyCallable) {
       pyobj = *pyCallable;
    }else {
       pyobj = expr->getPyValue();
       if(!pyobj.isCallable())
           EXPR_THROW("Expects Python callable.");
    }

    static std::set<PyObject*> blockedObjs;
    if(blockedObjs.empty()) {
#define BLOCK_VAR "__blockobj"
#define BLOCK_CMD(_name) BLOCK_VAR "=" _name
        const char *cmds[] = {
            BLOCK_CMD("Gui.doCommand"),
        };
        for(auto cmd : cmds) {
            try {
                PyObject * pyvalue = Base::Interpreter().getValue(cmd, BLOCK_VAR);
                blockedObjs.insert(pyvalue);
                Py::_XDECREF(pyvalue);
            }catch(Base::Exception &e) {
                FC_LOG("Exception on blocking " << cmd << ": " << e.what());
            }
        }
        Base::Interpreter().removeVariable(BLOCK_VAR);
        blockedObjs.insert(EvalFrame::getBuiltin("eval"));
        blockedObjs.insert(EvalFrame::getBuiltin("execfile"));
        blockedObjs.insert(EvalFrame::getBuiltin("exec"));
        blockedObjs.insert(EvalFrame::getBuiltin("__import__"));
        blockedObjs.insert(EvalFrame::getBuiltin("file"));
        blockedObjs.insert(EvalFrame::getBuiltin("open"));
        blockedObjs.insert(EvalFrame::getBuiltin("input"));
    }
    if(blockedObjs.find(pyobj.ptr()) != blockedObjs.end())
        EXPR_THROW("Python built-in blocked");

    int count=0;
    std::vector<Py::Sequence,ExpressionAllocator(Py::Sequence) > seqs;
    int i = 0;
    for(auto &arg : args) {
        auto &name = names[i++];
        if(name.empty())
            ++count;
        else if(name == "*") {
            PyIterable pyobj(arg->getPyValue(),this,true);
            seqs.push_back(pyobj);
            count += (int)seqs.back().size();
        }
    }
    Py::Tuple tuple(count);
    Py::Dict dict;
    auto it = seqs.begin();
    i = 0;
    int k=0;
    for(auto &arg : args) {
        auto &name = names[i++];
        if(name.empty())
            tuple.setItem(k++,arg->getPyValue());
        else if(name == "*") {
            Py::Sequence seq = *it++;
            for(size_t j=0;j<seq.size();++j)
                tuple.setItem(k++,Py::Object(seq[j].ptr()));
        }else if(name == "**") {
            Py::Object pyobj(arg->getPyValue());
            if(!pyobj.isMapping()) 
                EXPR_THROW("Expects Python mapping.");
            Py::Mapping map(pyobj);
            for(auto it=map.begin();it!=map.end();++it) {
                const auto &value = *it;
                if(!value.first.isString())
                    EXPR_THROW("Expects only string key.");
                dict.setItem(value.first, value.second);
            }
        }else
            dict.setItem(name, arg->getPyValue());
    }
    try {
        ImportParamLock lock;
        return Py::Callable(pyobj).apply(tuple,dict);
    }catch (Py::Exception&) {
        EXPR_PY_THROW(this);
    }
    return Py::Object();
}

ExpressionPtr CallableExpression::_copy() const {
    _EXPR_NEW(res,CallableExpression,owner);
    if(expr) res->expr = expr->copy();
    res->names = names;
    copy_vector(res->args,args);
    res->name = name;
    res->ftype = ftype;
    return _res;
}

//
// PyObjectExpression class
//

EXPR_TYPESYSTEM_SOURCE(App::PyObjectExpression, App::Expression);

ExpressionPtr PyObjectExpression::create(const DocumentObject *owner, PyObject *obj) {
    _EXPR_NEW(res,PyObjectExpression,owner);
    Py::_XINCREF(obj);
    res->pyObj = obj;
    return _res;
}

PyObjectExpression::~PyObjectExpression() {
    if(pyObj) {
        Base::PyGILStateLocker lock;
        Py::_XDECREF(pyObj);
    }
}

Py::Object PyObjectExpression::getPyObject() const {
    if(!pyObj)
        return Py::Object();
    return Py::Object(pyObj);
}

void PyObjectExpression::setPyObject(Py::Object obj) {
    Py::_XDECREF(pyObj);
    pyObj = obj.ptr();
    Py::_XINCREF(pyObj);
}

void PyObjectExpression::setPyObject(PyObject *obj, bool owned) {
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

ExpressionPtr PyObjectExpression::_copy() const
{
    if(!pyObj)
        return PyObjectExpression::create(owner);
    Base::PyGILStateLocker lock;
    return PyObjectExpression::create(owner, pyObj);
}

Py::Object PyObjectExpression::_getPyValue(int *) const {
    if(!pyObj)
        return Py::Object();
    return Py::Object(pyObj);
}

//
// StringExpression class
//

EXPR_TYPESYSTEM_SOURCE(App::StringExpression, App::Expression);

StringExpression::~StringExpression() {
    if(cache) {
        Base::PyGILStateLocker lock;
        Py::_XDECREF(cache);
    }
}

ExpressionPtr StringExpression::create(const DocumentObject *owner, ExpressionString &&str) {
    _EXPR_NEW(res,StringExpression,owner);
    res->str = std::move(str);
    return _res;
}

void StringExpression::_toString(std::ostream &ss, bool,int) const
{
    static const char *quoteLeft[] = {"<<","'","\"","'''","\"\"\""};
    static const char *quoteRight[] = {">>","'","\"","'''","\"\"\""};
    static const char quotes[] = {'>','\'','"','\'','"'};

    size_t idx = (size_t)str.quote;
    assert(idx<sizeof(quoteLeft)/sizeof(quoteLeft[0]));
    if(str.prefix0)
        ss << str.prefix0;
    if(str.prefix1)
        ss << str.prefix1;
    ss << quoteLeft[idx];
    bool isRaw = str.isRaw();
    bool isLong = str.isLong();
    char quote = quotes[idx];

    for(auto c : str.text) {
        if(c == quote)
            ss << '\\' << c;
        else if(isRaw) 
            ss << c;
        else {
            switch (c) {
            case '\t':
                ss << "\\t";
                break;
            case '\n':
                if(isLong)
                    ss << c;
                else
                    ss << "\\n";
                break;
            case '\r':
                ss << "\\r";
                break;
            case '\\':
                ss << "\\\\";
                break;
            default:
                ss << c;
            }
        }
    }
    ss << quoteRight[idx];
}

ExpressionString ExpressionString::unquote(const char *txt) {
    std::stringstream ss;
    ExpressionString res;

    if(std::isalpha((unsigned char)txt[0])) {
        char c = std::tolower((unsigned char)txt[0]);
        if(c != 'r' && c != 'u')
            PARSER_THROW("Unsupported string prefix '" << c << "'");
        else{
            res.prefix0 = c;
            ++txt;
            if(std::isalpha((unsigned char)txt[0])) {
                c = std::tolower((unsigned char)txt[0]);
                if(c != 'r' && c != 'u')
                    PARSER_THROW("Unsupported string prefix '" << c << "'");
                else{
                    res.prefix1 = c;
                    ++txt;
                }
            }
        }
    }

    if(txt[0] == '<') {
        assert(res.prefix0==0);
        assert(txt[1] == '<');
        txt += 2;
    } else if(txt[0] == '\'') {
        ++txt;
        if(txt[0] == '\'') {
            if(txt[1] == 0) {
                res.quote = QuoteSingle;
                return res;
            }
            if(txt[1]!='\'')
                PARSER_THROW("Invalid string");
            res.quote = QuoteSingleLong;
            txt += 2;
        }else
            res.quote = QuoteSingle;
    }else if(txt[0] == '"') {
        ++txt;
        if(txt[0] == '"') {
            if(txt[1] == 0) {
                res.quote = QuoteDouble;
                return res;
            }
            if(txt[1]!='"')
                PARSER_THROW("Invalid string");
            res.quote = QuoteDoubleLong;
            txt += 2;
        }else
            res.quote = QuoteDouble;
    }else
        PARSER_THROW("Invalid string");

    size_t len = strlen(txt);
    if(!res.quote) {
        if(len<2)
            PARSER_THROW("Invalid string");
        len-=2;
    }else if(res.quote == QuoteSingle || res.quote==QuoteDouble) {
        if(len<1)
            PARSER_THROW("Invalid string");
        --len;
    }else{
        if(len<3)
            PARSER_THROW("Invalid string");
        len -= 3;
    }

    bool isRaw = res.isRaw();
    bool escaped = false;
    const char *end = txt+len;
    for(const char *cur=txt;cur!=end;++cur) {
        if (escaped) {
            switch (*cur) {
            case '>':
                ss << '>';
                break;
            case 't':
                ss << '\t';
                break;
            case 'n':
                ss << '\n';
                break;
            case 'r':
                ss << '\r';
                break;
            case '\\':
                ss << '\\';
                break;
            case '\'':
                ss << '\'';
                break;
            case '"':
                ss << '"';
                break;
            default:
                ss << '\\';
                ss << *cur;
            }
            escaped = false;
        } else if(!isRaw && *cur == '\\')
            escaped = true;
        else
            ss << *cur;
    }
    res.text = ss.str();
    return res;
}

void StringExpression::append(const ExpressionString &s) {
    str.text += s.text;
    if(cache) {
        Base::PyGILStateLocker lock;
        Py::_XDECREF(cache);
        cache = 0;
    }
}

ExpressionPtr StringExpression::_copy() const
{
    return StringExpression::create(owner, ExpressionString(str));
}

Py::Object StringExpression::_getPyValue(int *) const {
    if(!cache) 
        cache = Py::new_reference_to(Py::String(str.text));
    return Py::Object(cache);
}

///////////////////////////////////////////////////////////////////

EXPR_TYPESYSTEM_SOURCE(App::ConditionalExpression, App::Expression);

ExpressionPtr ConditionalExpression::create(const DocumentObject *owner, 
        ExpressionPtr &&condition, ExpressionPtr &&trueExpr, ExpressionPtr &&falseExpr, bool pythonForm)
{
    _EXPR_NEW(res,ConditionalExpression,owner);
    res->condition = std::move(condition);
    res->trueExpr = std::move(trueExpr);
    res->falseExpr = std::move(falseExpr);
    res->pythonForm = pythonForm;
    return _res;
}

bool ConditionalExpression::isTouched() const
{
    return condition->isTouched() || trueExpr->isTouched() || falseExpr->isTouched();
}

Py::Object ConditionalExpression::_getPyValue(int *) const {
    if(condition->getPyValue().isTrue())
        return trueExpr->getPyValue();
    else
        return falseExpr->getPyValue();
}

ExpressionPtr ConditionalExpression::simplify() const
{
    ExpressionPtr e(condition->simplify());
    NumberExpression * v = freecad_dynamic_cast<NumberExpression>(e.get());

    if (v == 0) {
        return copy();
    } else {
        if (fabs(v->getValue()) > 0.5)
            return trueExpr->simplify();
        else
            return falseExpr->simplify();
    }
}

void ConditionalExpression::_toString(std::ostream &ss, bool persistent,int) const
{
    if(!pythonForm)
        ss << condition->toStr(persistent) << " ? ";

    if (trueExpr->priority() <= priority())
        ss << '(' << trueExpr->toStr(persistent) << ')';
    else
        ss << trueExpr->toStr(persistent);

    if(pythonForm)
        ss << " if " << condition->toStr(persistent) << " else ";
    else
        ss << " : ";

    if (falseExpr->priority() <= priority())
        ss << '(' << falseExpr->toStr(persistent) << ')';
    else
        ss << falseExpr->toStr(persistent);
}

ExpressionPtr ConditionalExpression::_copy() const
{
    return ConditionalExpression::create(
            owner,condition->copy(),trueExpr->copy(),falseExpr->copy());
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

/////////////////////////////////////////////////////////////////////////

EXPR_TYPESYSTEM_SOURCE(App::ConstantExpression, App::NumberExpression);

ExpressionPtr ConstantExpression::create(const DocumentObject *owner, 
        std::string &&name, const Quantity & quantity)
{
    _EXPR_NEW(res, ConstantExpression, owner, quantity);
    res->name = std::move(name);
    return _res;
}

ExpressionPtr ConstantExpression::create(const DocumentObject *owner, 
        const char *name, const Quantity & quantity)
{
    _EXPR_NEW(res,ConstantExpression,owner,quantity);
    res->name = name;
    return _res;
}

void ConstantExpression::_toString(std::ostream &ss, bool,int) const
{
    ss << name;
}

ExpressionPtr ConstantExpression::_copy() const
{
    return ConstantExpression::create(owner, std::string(name), getQuantity());
}

ExpressionPtr ConstantExpression::simplify() const {
    return copy();
}

Py::Object ConstantExpression::_getPyValue(int *) const {
    if(!cache) {
        if(name == "None") 
            cache = Py::new_reference_to(Py::None());
        if(name == "True")
            cache = Py::new_reference_to(Py::True());
        if(name == "False")
            cache = Py::new_reference_to(Py::False());
        else
            return NumberExpression::_getPyValue();
    }
    return Py::Object(cache);
}

/////////////////////////////////////////////////////////////////////////////////////

EXPR_TYPESYSTEM_SOURCE(App::BooleanExpression, App::NumberExpression);

ExpressionPtr BooleanExpression::create(const DocumentObject *owner, bool value) {
    return EXPR_NEW(BooleanExpression,owner,Quantity(value?1.0:0.0));
}

ExpressionPtr BooleanExpression::_copy() const {
    return EXPR_NEW(BooleanExpression,owner,getQuantity());
}

Py::Object BooleanExpression::_getPyValue(int *) const {
    return Py::Boolean(essentiallyZero(getValue()));
}

/////////////////////////////////////////////////////////////////////////////////////

EXPR_TYPESYSTEM_SOURCE(App::RangeExpression, App::Expression);

ExpressionPtr RangeExpression::create(const DocumentObject *owner, std::string &&begin, std::string &&end) 
{
    _EXPR_NEW(res,RangeExpression,owner);
    res->begin = std::move(begin);
    res->end = std::move(end);
    return _res;
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

void RangeExpression::_toString(std::ostream &ss, bool,int) const
{
    ss << begin << ':' << end;
}

ExpressionPtr RangeExpression::_copy() const
{
    return RangeExpression::create(owner,std::string(begin),std::string(end));
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

Py::Object RangeExpression::_getPyValue(int *) const {
    Range i(getRange());
    Py::Tuple tuple(i.size());
    int j=0;
    do {
        try {
            ObjectIdentifier var(owner,i.address());
            tuple.setItem(j++,var.getPyValue());
        }catch(Base::Exception &e) {
            _EXPR_RETHROW(e,"Failed to obtian cell '" << i.address() << "': ",this);
        }
    }while(i.next());
    return tuple;
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


/////////////////////////////////////////////////////////////////////////////////

EXPR_TYPESYSTEM_SOURCE(App::ComprehensionExpression, App::Expression);

ExpressionPtr ComprehensionExpression::create(const DocumentObject *owner,
        int catchAll, ExpressionList &&targets, ExpressionPtr &&expr)
{
    _EXPR_NEW(res,ComprehensionExpression,owner);
    res->add(catchAll, std::move(targets),std::move(expr));
    return _res;
}

ComprehensionExpression::CompFor::CompFor(const CompFor &other) {
    catchAll = other.catchAll;
    if(other.expr)
        expr = other.expr->copy();
    copy_vector(targets,other.targets);
    copy_vector(conds,other.conds);
}

void ComprehensionExpression::setExpr(ExpressionPtr &&_key, ExpressionPtr &&_value) {
    key = std::move(_key);
    value = std::move(_value);
    list = false;
}

void ComprehensionExpression::setExpr(ExpressionPtr &&_key, bool isList) {
    key = std::move(_key);
    value.reset();
    list = isList;
}

void ComprehensionExpression::_visit(ExpressionVisitor & v) {
    if(key) key->visit(v);
    if(value) value->visit(v);
    for(auto &comp : comps) {
        for(auto &e : comp.targets)
            e->visit(v);
        comp.expr->visit(v);
        for(auto &cond : comp.conds)
            cond->visit(v);
    }
}

bool ComprehensionExpression::isTouched() const {
    if(key && key->isTouched())
        return true;
    if(value && value->isTouched())
        return true;
    for(auto &comp : comps) {
        for(auto &e : comp.targets) {
            if(e->isTouched())
                return true;
        }
        if(comp.expr->isTouched())
            return true;
        for(auto &cond : comp.conds) {
            if(cond->isTouched())
                return true;
        }
    }
    return false;
}

void ComprehensionExpression::add(int catchAll, ExpressionList &&targets, ExpressionPtr &&expr) {
    comps.emplace_back();
    auto &comp = comps.back();
    comp.catchAll = catchAll;
    comp.targets = std::move(targets);
    comp.expr = std::move(expr);
}

void ComprehensionExpression::addCond(ExpressionPtr &&cond) {
    assert(comps.size());
    comps.back().conds.push_back(std::move(cond));
}

void ComprehensionExpression::_toString(std::ostream &ss, bool persistent,int) const {
    if(!key)
        return;
    if(list)
        ss << "[  " << key->toStr(persistent);
    else if(value)
        ss << "{ " << key->toStr(persistent,true) << " : " << value->toStr(persistent,true);
    else
        ss << "{ " << key->toStr(persistent);
    for(auto &comp : comps) {
        ss << " for ";
        bool first = true;
        int i = 0;
        for(auto &e : comp.targets) {
            if(first)
                first = false;
            else
                ss << ", ";
            if(i++ == comp.catchAll)
                ss << '*';
            ss << e->toStr(persistent);
        }
        ss << " in " << comp.expr->toStr(persistent);
        for(auto &cond : comp.conds)
            ss << " if " << cond->toStr(persistent);
    }
    ss << (list?" ]":" }");
}

ExpressionPtr ComprehensionExpression::_copy() const
{
    _EXPR_NEW(res,ComprehensionExpression,owner);
    res->list = list;
    if(key) res->key = key->copy();
    if(value) res->value = value->copy();
    for(auto &comp : comps) {
        res->comps.emplace_back();
        auto &other = res->comps.back();
        copy_vector(other.targets,comp.targets);
        other.expr = comp.expr->copy();
        copy_vector(other.conds,comp.conds);
    }
    return _res;
}

Py::Object ComprehensionExpression::_getPyValue(int *) const {
    if(!key)
        return Py::Object();
    EvalFrame frame;
    frame.push();
    Py::Object res(list?PyList_New(0):(value?PyDict_New():PySet_New(0)));
    try {
        _calc(res,comps.begin());
    }catch(Py::Exception &) {
        EXPR_PY_THROW(this);
    }
    return res;
}

void ComprehensionExpression::_calc(Py::Object &res, CompForList::const_iterator iter) const{
    if(iter == comps.end()) {
        if(value)
            PyDict_SetItem(res.ptr(),*key->getPyValue(),*value->getPyValue());
        else if(list)
            PyList_Append(res.ptr(),*key->getPyValue());
        else
            PySet_Add(res.ptr(),*key->getPyValue());
        return;
    }

    auto &comp = *iter++;

    ExpressionPtr tmp = PyObjectExpression::create(owner);
    auto pyexpr = static_cast<PyObjectExpression*>(tmp.get());

    PyIterable seq(comp.expr->getPyValue(),this,false);
    Py::Object item;
    while(seq.next(item,comp.expr.get())) {
        pyexpr->setPyObject(item);
        AssignmentExpression::apply(this,comp.catchAll,comp.targets,pyexpr);
        bool proceed = true;
        for(auto &cond : comp.conds) {
            if(!(proceed=cond->getPyValue().isTrue()))
                break;
        }
        if(proceed)
            _calc(res,iter);
    }
}

// ListExpression class
//

EXPR_TYPESYSTEM_SOURCE(App::ListExpression, App::Expression);

ExpressionPtr ListExpression::create(const DocumentObject *owner) {
    return EXPR_NEW(ListExpression,owner);
}

ExpressionPtr ListExpression::create(const DocumentObject *owner,
        ExpressionList &&items, FlagList &&flags)
{
    _EXPR_NEW(res,ListExpression,owner);
    res->flags = flags;
    res->items = std::move(items);
    if(flags.size() < items.size())
        flags.resize(items.size(),false);
    return _res;
}

void ListExpression::_visit(ExpressionVisitor & v) {
    for(auto &item : items)
        item->visit(v);
}

bool ListExpression::isTouched() const {
    for(auto &item : items) {
        if(item->isTouched())
            return true;
    }
    return false;
}

void ListExpression::printItems(std::ostream &ss, bool persistent) const {
    int i=0;
    bool first = true;
    for(auto &item : items) {
        bool flag = flags[i++];
        if(first)
            first = false;
        else
            ss << ", ";
        if(flag)
            ss << '*';
        ss << item->toStr(persistent,flag);
    }
}

void ListExpression::_toString(std::ostream &ss, bool persistent,int) const {
    ss << '[';
    printItems(ss,persistent);
    ss << ']';
}

ExpressionPtr ListExpression::_copy() const
{
    _EXPR_NEW(res,ListExpression,owner);
    res->flags = flags;
    copy_vector(res->items,items);
    return _res;
}

Py::Object ListExpression::_getPyValue(int *) const {
    Py::List list;
    int i = 0;
    for(auto &item : items) {
        bool flag = flags[i++];
        Py::Object pyvalue = item->getPyValue();
        if(!flag) {
            list.append(pyvalue);
            continue;
        }
        PyIterable seq(pyvalue,this,false);
        Py::Object pyitem;
        while(seq.next(pyitem,item.get()))
            list.append(pyitem);
    }
    return list;
}

void ListExpression::setItem(std::size_t index, ExpressionPtr &&expr) {
    if(index >= items.size())
        __EXPR_THROW(IndexError,"Index out of bound",this);
    items[index] = std::move(expr);
}

void ListExpression::append(ExpressionPtr &&expr) {
    items.push_back(std::move(expr));
}


// TupleExpression class
//

EXPR_TYPESYSTEM_SOURCE(App::TupleExpression, App::ListExpression);

ExpressionPtr TupleExpression::create(const DocumentObject *owner) {
    return EXPR_NEW(TupleExpression,owner);
}

ExpressionPtr TupleExpression::create(const DocumentObject *owner, ExpressionPtr &&item, bool flag)
{
    _EXPR_NEW(res,TupleExpression,owner);
    res->flags.push_back(flag);
    res->items.push_back(std::move(item));
    return _res;
}

ExpressionPtr TupleExpression::create(const DocumentObject *owner,
        ExpressionList &&items, FlagList &&flags)
{
    _EXPR_NEW(res,TupleExpression,owner);
    res->flags = flags;
    res->items = std::move(items);
    if(flags.size() < items.size())
        flags.resize(items.size(),false);
    return _res;
}

void TupleExpression::_toString(std::ostream &ss, bool persistent,int) const {
    ss << '(';
    printItems(ss,persistent);
    if(items.size() == 1)
        ss << ", ";
    ss << ')';
}

ExpressionPtr TupleExpression::_copy() const
{
    _EXPR_NEW(res,TupleExpression,owner);
    res->flags = flags;
    copy_vector(res->items,items);
    return _res;
}

Py::Object TupleExpression::_getPyValue(int *) const {
    Py::Sequence seq(ListExpression::_getPyValue());
    return Py::Tuple(seq);
}


// DictExpression class
//

EXPR_TYPESYSTEM_SOURCE(App::DictExpression, App::Expression);

ExpressionPtr DictExpression::create(const DocumentObject *owner) {
    return EXPR_NEW(DictExpression,owner);
}

ExpressionPtr DictExpression::create(const DocumentObject *owner, ExpressionPtr &&key, ExpressionPtr &&value)
{
    _EXPR_NEW(res,DictExpression,owner);
    res->addItem(std::move(key),std::move(value));
    return _res;
}

ExpressionPtr DictExpression::create(const DocumentObject *owner, ExpressionPtr &&value)
{
    _EXPR_NEW(res,DictExpression,owner);
    res->addItem(std::move(value));
    return _res;
}

void DictExpression::_visit(ExpressionVisitor & v) {
    int i=0;
    for(auto &key : keys) {
        auto &value = values[i++];
        if(key) 
            key->visit(v);
        value->visit(v);
    }
}

bool DictExpression::isTouched() const {
    int i=0;
    for(auto &key : keys) {
        auto &value = values[i++];
        if(key && key->isTouched()) 
            return true;
        if(value->isTouched())
            return true;
    }
    return false;
}

void DictExpression::addItem(ExpressionPtr &&key, ExpressionPtr &&value) {
    assert(key && value);
    keys.push_back(std::move(key));
    values.push_back(std::move(value));
}

void DictExpression::addItem(ExpressionPtr &&value) {
    assert(value);
    keys.push_back(0);
    values.push_back(std::move(value));
}

void DictExpression::_toString(std::ostream &ss, bool persistent,int) const {
    ss << '{';
    bool first = true;
    int i=0;
    for(auto &key : keys) {
        auto &value = values[i++];
        if(first)
            first = false;
        else
            ss << ", ";
        if(key)
            ss << key->toStr(persistent,true) << ':';
        else
            ss << "**";
        ss << value->toStr(persistent,true);
    }
    ss << '}';
}

ExpressionPtr DictExpression::_copy() const
{
    _EXPR_NEW(res,DictExpression,owner);
    copy_vector(res->keys,keys);
    copy_vector(res->values,values);
    return _res;
}

Py::Object DictExpression::_getPyValue(int *) const {
    Py::Dict dict;
    int i=0;
    for(auto &key : keys) {
        auto &value = values[i++];
        Py::Object pyvalue = value->getPyValue();
        if(key) {
            dict.setItem(key->getPyValue(),pyvalue);
            continue;
        }
        if(!pyvalue.isMapping()) 
            _EXPR_THROW("Cannot expand non mapping object.",value.get());
        Py::Mapping map(pyvalue);
        for(auto it=map.begin();it!=map.end();++it) {
            const auto &v = (*it);
            dict.setItem(v.first,v.second);
        }
    }
    return dict;
}


// IDictExpression class
//

EXPR_TYPESYSTEM_SOURCE(App::IDictExpression, App::Expression);

ExpressionPtr IDictExpression::create(const DocumentObject *owner) {
    return EXPR_NEW(IDictExpression,owner);
}

ExpressionPtr IDictExpression::create(const DocumentObject *owner, std::string &&key, ExpressionPtr &&value)
{
    _EXPR_NEW(res,IDictExpression,owner);
    res->addItem(std::move(key),std::move(value));
    return _res;
}

ExpressionPtr IDictExpression::create(const DocumentObject *owner, const char *key, ExpressionPtr &&value)
{
    _EXPR_NEW(res,IDictExpression,owner);
    res->addItem(std::move(key),std::move(value));
    return _res;
}

void IDictExpression::_visit(ExpressionVisitor & v) {
    for(auto &value : values)
        value->visit(v);
}

bool IDictExpression::isTouched() const {
    for(auto &v : values) {
        if(v->isTouched())
            return true;
    }
    return false;
}

void IDictExpression::addItem(std::string &&key, ExpressionPtr &&value) {
    assert(value);
    keys.push_back(std::move(key));
    values.push_back(std::move(value));
}

void IDictExpression::addItem(const char *key, ExpressionPtr &&value) {
    assert(key && value);
    keys.emplace_back(key);
    values.push_back(std::move(value));
}

void IDictExpression::_toString(std::ostream &ss, bool persistent,int) const {
    ss << '{';
    bool first = true;
    int i = 0;
    for(auto &value : values) {
        auto &key = keys[i++];
        if(first)
            first = false;
        else
            ss << ", ";
        ss << key << '=' << value->toStr(persistent,true);
    }
    ss << '}';
}

ExpressionPtr IDictExpression::_copy() const
{
    _EXPR_NEW(res,IDictExpression,owner);
    res->keys = keys;
    copy_vector(res->values,values);
    return _res;
}

Py::Object IDictExpression::_getPyValue(int *) const {
    Py::Dict dict;
    int i = 0;
    for(auto &value : values) {
        auto &key = keys[i++];
        Py::Object pyvalue = value->getPyValue();
        if(key != "**") {
            dict.setItem(key,pyvalue);
            continue;
        }
        if(!pyvalue.isMapping()) 
            _EXPR_THROW("Cannot expand non mapping object.",value.get());
        Py::Mapping map(pyvalue);
        for(auto it=map.begin();it!=map.end();++it) {
            const auto &value = (*it);
            dict.setItem(value.first,value.second);
        }
    }
    return dict;
}

////////////////////////////////////////////////////////////////////////////////////
EXPR_TYPESYSTEM_SOURCE(App::BaseStatement, App::Expression);

/////////////////////////////////////////////////////////////

EXPR_TYPESYSTEM_SOURCE(App::PseudoStatement, App::BaseStatement);

enum PseudoType {
    PY_NONE,
    PY_BEGIN,
    PY_END,
    PY_PASS,
};

ExpressionPtr  PseudoStatement::create(const App::DocumentObject *owner, int type) {
    return EXPR_NEW(PseudoStatement,owner,type);
}

ExpressionPtr PseudoStatement::_copy() const {
    return PseudoStatement::create(owner,type);
}

void PseudoStatement::_toString(std::ostream &ss, bool, int) const {
    switch(type) {
    case PY_BEGIN:
        ss << "#@pybegin";
        break;
    case PY_END:
        ss << "#@pyend";
        break;
    case PY_PASS:
        ss << "pass";
        break;
    default:
        assert(0);
    }
}

Py::Object PseudoStatement::_getPyValue(int *) const {
    switch(type) {
    case PY_BEGIN:
    case PY_END:
        if(_EvalStack.size())
            _EvalStack.front()->setPythonMode(type==PY_BEGIN);
        break;
    }
    return Py::Object();
}

////////////////////////////////////////////////////////////////////////////////////

EXPR_TYPESYSTEM_SOURCE(App::JumpStatement, App::BaseStatement);

ExpressionPtr JumpStatement::create(const App::DocumentObject *owner, int type, ExpressionPtr &&expr) {
    _EXPR_NEW(res,JumpStatement,owner);
    res->type = type;
    res->expr = std::move(expr);
    return _res;
}

ExpressionPtr JumpStatement::_copy() const {
    return JumpStatement::create(owner,type,expr?expr->copy():ExpressionPtr());
}

void JumpStatement::_visit(ExpressionVisitor &v) {
    if(expr)
        expr->visit(v);
}

bool JumpStatement::isTouched() const {
    if(expr)
        return expr->isTouched();
    return false;
}

static inline void printIndent(std::ostream &ss, int indent) {
    for(int i=0;i<indent;++i)
        ss << ' ';
}

#define INDENT_STEP 4

void JumpStatement::_toString(std::ostream &ss, bool persistent, int) const {
    switch(type) {
    case JUMP_BREAK:
        ss << "break";
        break;
    case JUMP_CONTINUE:
        ss << "continue";
        break;
    case JUMP_RETURN:
        ss << "return";
        if(expr)
            ss << ' ' << expr->toStr(persistent);
        break;
    case JUMP_RAISE:
        ss << "raise";
        if(expr)
            ss << ' ' << expr->toStr(persistent);
        break;
    default:
        assert(0);
    }
}

Py::Object JumpStatement::_getPyValue(int *jumpCode) const {
    if(type == JUMP_RAISE) {
        CHECK_STACK(RaiseStatement,this);
        Py::Object exception;
        if(!expr) {
            exception = _EvalStack.back()->lastException;
            if(exception.isNone())
                EXPR_THROW("no current exception");
        } else {
            exception = expr->getPyValue();
        }
        throw PyException(exception);
    }
    if(jumpCode) 
        *jumpCode = type;
    if(!expr)
        return Py::Object();
    return expr->getPyValue();
}

////////////////////////////////////////////////////////////////////////////////////

EXPR_TYPESYSTEM_SOURCE(App::IfStatement, App::BaseStatement);

ExpressionPtr IfStatement::create(const App::DocumentObject *owner, ExpressionPtr &&cond, ExpressionPtr &&stmt) {
    _EXPR_NEW(res,IfStatement,owner);
    res->add(std::move(cond),std::move(stmt));
    return _res;
}

void IfStatement::addElse(ExpressionPtr &&stmt) {
    conditions.push_back(0);
    statements.push_back(std::move(stmt));
}

void IfStatement::add(ExpressionPtr &&cond, ExpressionPtr &&stmt) {
    conditions.push_back(std::move(cond));
    statements.push_back(std::move(stmt));
}

ExpressionPtr IfStatement::_copy() const {
    _EXPR_NEW(res,IfStatement,owner);
    copy_vector(res->conditions,conditions);
    copy_vector(res->statements,statements);
    return _res;
}

void IfStatement::_visit(ExpressionVisitor &v) {
    for(auto &cond : conditions) {
        if(cond)
            cond->visit(v);
    }
    for(auto &stmt : statements)
        stmt->visit(v);
}

bool IfStatement::isTouched() const {
    for(auto &cond : conditions) {
        if(cond && cond->isTouched())
            return true;
    }
    for(auto &stmt : statements) {
        if(stmt->isTouched())
            return true;
    }
    return false;
}

void IfStatement::_toString(std::ostream &ss, bool persistent, int indent) const {
    bool first = true;
    int i=0;
    for(auto &cond : conditions) {
        auto &stmt = statements[i++];
        if(first) {
            first = false;
            ss << "if ";
            if(cond)
                ss << cond->toStr(persistent) << ":";
            else
                ss << " True:" ;
        }else if(cond) {
            ss << std::endl;
            printIndent(ss,indent);
            ss << "elif " << cond->toStr(persistent) << ":";
        }else {
            ss << std::endl;
            printIndent(ss,indent);
            ss << "else:";
        }
        if(stmt->isDerivedFrom(Statement::getClassTypeId()))
            ss << std::endl;
        else
            ss << ' ';
        ss << stmt->toStr(persistent,false,indent+INDENT_STEP);
    }
}

bool IfStatement::needLineEnd() const {
    return true;
}

Py::Object IfStatement::_getPyValue(int *jumpCode) const {
    int i=0;
    for(auto &cond : conditions) {
        auto &stmt = statements[i++];
        if(!cond || cond->getPyValue().isTrue())
            return stmt->getPyValue(0,jumpCode);
    }
    return Py::Object();
}

////////////////////////////////////////////////////////////////////////////////////

EXPR_TYPESYSTEM_SOURCE(App::WhileStatement, App::BaseStatement);

ExpressionPtr WhileStatement::create(
        const App::DocumentObject *owner, ExpressionPtr &&cond, ExpressionPtr &&stmt) 
{
    _EXPR_NEW(res,WhileStatement,owner);
    res->condition = std::move(cond);
    res->statement = std::move(stmt);
    return _res;
}

void WhileStatement::addElse(ExpressionPtr &&expr) {
    else_expr = std::move(expr);
}

ExpressionPtr WhileStatement::_copy() const {
    _EXPR_NEW(res,WhileStatement,owner);
    if(condition) res->condition = condition->copy();
    if(statement) res->statement = statement->copy();
    if(else_expr) res->else_expr = else_expr->copy();
    return _res;
}

void WhileStatement::_visit(ExpressionVisitor &v) {
    if(condition)
        condition->visit(v);
    if(statement)
        statement->visit(v);
    if(else_expr)
        else_expr->visit(v);
}

bool WhileStatement::isTouched() const {
    if(condition && condition->isTouched())
        return true;
    if(statement && statement->isTouched())
        return true;
    if(else_expr && else_expr->isTouched())
        return true;
    return false;
}

void WhileStatement::_toString(std::ostream &ss, bool persistent, int indent) const {
    if(!condition)
        return;
    ss << "while " << condition->toStr(persistent) << " :";
    if(statement->isDerivedFrom(Statement::getClassTypeId()))
        ss << std::endl;
    else
        ss << ' ';
    ss << statement->toStr(persistent,false,indent+INDENT_STEP);
    if(else_expr) {
        ss << std::endl;
        printIndent(ss,indent);
        ss << "else:";
        if(else_expr->isDerivedFrom(Statement::getClassTypeId()))
            ss << std::endl;
        else
            ss << ' ';
        ss << else_expr->toStr(persistent,false,indent+INDENT_STEP);
    }
}

bool WhileStatement::needLineEnd() const {
    return true;
}

Py::Object WhileStatement::_getPyValue(int *jumpCode) const {
#define INIT_JUMP_CODE \
    int _jumpCode;\
    if(!jumpCode)\
        jumpCode = &_jumpCode;\
    *jumpCode = 0

    INIT_JUMP_CODE;

    TimingInit();

    Py::Object res;

    if(condition) {
        int count = 0;
        int limit = EvalFrame::getLookCheck();
        for(;;) {
            {
                Timing(cond);
                if(!condition->getPyValue().isTrue())
                    break;
            }
            res = statement->getPyValue(0,jumpCode);
            switch(*jumpCode) {
            case JUMP_RETURN:
                return res;
            case JUMP_BREAK:
                *jumpCode = 0;
                return Py::Object();
            case JUMP_CONTINUE:
                *jumpCode = 0;
                // fall through
            case JUMP_NONE:
                res = Py::Object();
                if(limit>0 && (++count % limit)==0)
                    Base::Sequencer().checkAbort();
                continue;
            default:
                assert(0);
            }
            break;
        }
    }
    if(else_expr)
        res = else_expr->getPyValue(0,jumpCode);
    TimingPrint();
    return res;
}

////////////////////////////////////////////////////////////////////////////////////

EXPR_TYPESYSTEM_SOURCE(App::ForStatement, App::BaseStatement);

ExpressionPtr ForStatement::create(const App::DocumentObject *owner, 
        int catchAll, ExpressionList &&targets, ExpressionList &&exprs, ExpressionPtr &&stmt)
{
    _EXPR_NEW(res,ForStatement,owner);
    res->catchAll = catchAll;
    res->targets = std::move(targets);
    if(exprs.size()==1)
        res->value = std::move(exprs[0]);
    else {
        res->value = TupleExpression::create(owner,std::move(exprs));
        res->valueTuple = true;
    }
    res->statement = std::move(stmt);
    return _res;
}

void ForStatement::addElse(ExpressionPtr &&expr) {
    else_expr = std::move(expr);
}

ExpressionPtr ForStatement::_copy() const {
    _EXPR_NEW(res,ForStatement,owner);
    copy_vector(res->targets,targets);
    res->value = value->copy();
    if(statement) res->statement = statement->copy();
    if(else_expr) res->else_expr = else_expr->copy();
    return _res;
}

void ForStatement::_visit(ExpressionVisitor &v) {
    for(auto &e : targets)
        e->visit(v);
    value->visit(v);
    if(statement)
        statement->visit(v);
    if(else_expr)
        else_expr->visit(v);
}

bool ForStatement::isTouched() const {
    for(auto &e : targets) {
        if(e->isTouched())
            return true;
    }
    if(value->isTouched())
        return true;
    if(statement && statement->isTouched())
        return true;
    if(else_expr && else_expr->isTouched())
        return true;
    return false;
}

void ForStatement::_toString(std::ostream &ss, bool persistent, int indent) const {
    if(!value)
        return;
    ss << "for ";
    bool first = true;
    int i=0;
    for(auto &e : targets) {
        if(first)
            first = false;
        else
            ss << ", ";
        if(i++ == catchAll)
            ss << '*';
        ss << e->toStr(persistent);
    }
    ss << " in ";
    if(valueTuple) {
        assert(value->isDerivedFrom(TupleExpression::getClassTypeId()));
        static_cast<TupleExpression*>(value.get())->printItems(ss,persistent);
    }else
        ss << value->toStr(persistent);
    ss << " :";
    if(statement->isDerivedFrom(Statement::getClassTypeId()))
        ss << std::endl;
    else
        ss << ' ';
    ss << statement->toStr(persistent,false,indent+INDENT_STEP);
    if(else_expr) {
        ss << std::endl;
        printIndent(ss,indent);
        ss << "else:";
        if(else_expr->isDerivedFrom(Statement::getClassTypeId()))
            ss << std::endl;
        else
            ss << ' ';
        ss << else_expr->toStr(persistent,false,indent+INDENT_STEP);
    }
}

bool ForStatement::needLineEnd() const {
    return true;
}

Py::Object ForStatement::_getPyValue(int *jumpCode) const {
    INIT_JUMP_CODE;
    CHECK_STACK(ForStatement,this);

    Py::Object res;
    ExpressionPtr tmp = PyObjectExpression::create(owner);
    auto pyexpr = static_cast<PyObjectExpression*>(tmp.get());
    PyIterable seq(value->getPyValue(),this,false);
    Py::Object item;
    while(seq.next(item,value.get())) {
        pyexpr->setPyObject(item);
        AssignmentExpression::apply(this,catchAll,targets,pyexpr);
        res = statement->getPyValue(0,jumpCode);
        switch(*jumpCode) {
        case JUMP_RETURN:
            return res;
        case JUMP_BREAK:
            *jumpCode = 0;
            return Py::Object();
        case JUMP_CONTINUE:
            *jumpCode = 0;
            // fall through
        case JUMP_NONE:
            res = Py::Object();
            continue;
        default:
            assert(0);
        }
        break;
    }
    if(else_expr)
        res = else_expr->getPyValue(0,jumpCode);
    return res;
}

////////////////////////////////////////////////////////////////////////////////////

EXPR_TYPESYSTEM_SOURCE(App::SimpleStatement, App::BaseStatement);

ExpressionPtr SimpleStatement::create(const App::DocumentObject *owner, ExpressionPtr &&expr) {
    _EXPR_NEW(res,SimpleStatement,owner);
    if(expr)
        res->add(std::move(expr));
    return _res;
}

void SimpleStatement::add(ExpressionPtr &&expr) {
    exprs.push_back(std::move(expr));
}

ExpressionPtr SimpleStatement::_copy() const {
    _EXPR_NEW(res,SimpleStatement,owner);
    copy_vector(res->exprs,exprs);
    return _res;
}

void SimpleStatement::_visit(ExpressionVisitor &v) {
    for(auto &expr : exprs)
        expr->visit(v);
}

bool SimpleStatement::isTouched() const {
    for(auto &expr : exprs) {
        if(expr->isTouched())
            return true;
    }
    return false;
}

const Expression *SimpleStatement::getExpr(std::size_t i) const {
    if(i < exprs.size())
        return exprs[i].get();
    return 0;
}

void SimpleStatement::_toString(std::ostream &ss, bool persistent, int) const {
    bool first = true;
    for(auto &expr : exprs) {
        if(first)
            first = false;
        else
            ss << "; ";
        ss << expr->toStr(persistent,false);
    }
}

Py::Object SimpleStatement::_getPyValue(int *jumpCode) const {
    INIT_JUMP_CODE;

    Expression *lastString = 0;
    Py::Object res;
    for(auto &e : exprs) {
        // Optimization for using string as comment, i.e. do not evaluate
        // string if it is used as a statement.
        if(!e->hasComponent() 
                && e->getTypeId() == StringExpression::getClassTypeId())
        {
            res = Py::Object();
            lastString = e.get();
            continue;
        }
        lastString = 0;
        res = e->getPyValue(0,jumpCode);
        switch(*jumpCode) {
        case JUMP_RETURN:
        case JUMP_BREAK:
        case JUMP_CONTINUE:
            return res;
        case JUMP_NONE:
            continue;
        default:
            assert(0);
        }
        break;
    }
    if(lastString)
        return lastString->getPyValue();
    return res;
}

ExpressionPtr SimpleStatement::reduce() const {
    if(exprs.size() == 1) {
        auto e = freecad_dynamic_cast<const SimpleStatement>(exprs[0].get());
        if(e)
            return e->reduce();
        return exprs[0]->copy();
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////

EXPR_TYPESYSTEM_SOURCE(App::Statement, App::SimpleStatement);

ExpressionPtr Statement::create(const App::DocumentObject *owner, ExpressionPtr &&expr) {
    _EXPR_NEW(res,Statement,owner);
    if(expr)
        res->add(std::move(expr));
    return _res;
}

void Statement::_toString(std::ostream &ss, bool persistent, int indent) const {
    for(size_t i=0;i<exprs.size();++i) {
        if(i!=0) {
            ss << std::endl;
            if(exprs[i-1]->needLineEnd())
                ss << std::endl;
        }
        printIndent(ss,indent);
        ss << exprs[i]->toStr(persistent,false,indent);
    }
}

ExpressionPtr Statement::_copy() const {
    _EXPR_NEW(res,Statement,owner);
    copy_vector(res->exprs,exprs);
    return _res;
}

///////////////////////////////////////////////////////////////////////////////////

EXPR_TYPESYSTEM_SOURCE(App::LambdaExpression, App::Expression);

static void checkArgs(const Expression::StringList &names, 
                      const Expression::ExpressionList &args) {
    StringSet nameSet;
    bool hasTuple = false;
    bool hasDict = false;
    bool hasKeyword = false;
    int i=0;
    for(auto &name : names) {
        auto &arg = args[i++];
        const char *n = name.c_str();
        assert(name.size());
        if(name[0] == '*') {
            if(name[1] == '*') {
                if(hasDict)
                    PARSER_THROW("Multiple dict arg '" << name << "'");
                hasDict = true;
                n += 2;
            }else if(hasDict)
                PARSER_THROW("Unexpected tuple arg '" << name << "'");
            else if(hasTuple)
                PARSER_THROW("Multiple tuple arg '" << name << "'");
            else {
                hasTuple = true;
                n += 1;
            }
        }else {
            if(hasTuple || hasDict)
                PARSER_THROW("Invalid arg '" << name << "'");
            if(arg)
                hasKeyword = true;
            else if(hasKeyword)
                PARSER_THROW("Invalid positional arg '" << name << "'");
        }
        if(!nameSet.insert(n).second)
            PARSER_THROW("Duplicate arg '" << name << "'");
    }
}

ExpressionPtr LambdaExpression::create(const App::DocumentObject *owner, 
        ExpressionPtr &&body, StringList &&names, ExpressionList &&args)
{
    checkArgs(names,args);
    _EXPR_NEW(res,LambdaExpression,owner);
    assert(body);
    res->body = std::move(body);
    res->names = std::move(names);
    res->args = std::move(args);
    return _res;
}

void LambdaExpression::_toString(std::ostream &ss, bool persistent, int) const {
    ss << "lambda ";
    bool first = true;
    int i=0;
    for(auto &arg : args) {
        auto &name = names[i++];
        if(first)
            first = false;
        else
            ss << ", ";
        ss << name;
        if(arg) {
            ss << '=';
            ss << arg->toStr(persistent);
        }
    }
    ss << " : " << body->toStr(persistent);
}

bool LambdaExpression::isTouched() const
{
    if(body->isTouched())
        return true;
    for(auto &arg : args) {
        if(arg && arg->isTouched())
            return true;
    }
    return false;
}

void LambdaExpression::_visit(ExpressionVisitor &v) {
    FunctionDepth depth;
    for(auto &arg : args) {
        if(arg) arg->visit(v);
    }
    body->visit(v);
}

ExpressionPtr LambdaExpression::_copy() const {
    _EXPR_NEW(res,LambdaExpression,owner);
    res->names = names;
    copy_vector(res->args,args);
    res->body = body->copy();
    return _res;
}

static Py::Object makeFunc(const Expression *owner,
                           const Expression &body, 
                           const Expression::StringList &names, 
                           const Expression::ExpressionList &args, 
                           const char *name=0)
{
    Expression::ExpressionList eval_args;
    eval_args.reserve(args.size());
    for(auto &arg : args) {
        if(arg)
            eval_args.push_back(arg->eval());
        else
            eval_args.emplace_back();
    }
    auto res = CallableExpression::create(owner->getOwner(),
                                          body.copy(),
                                          Expression::StringList(names),
                                          std::move(eval_args),
                                          FUNC_PARSED,
                                          std::string(name?name:""),
                                          false);
    Py::Object pyobj(new ExpressionPy(res.release()),false);
    if(name && _EvalStack.size()) {
        auto var = _EvalStack.back()->getVar(owner,name,BindLocalOnly);
        *var = pyobj;
    }
    return pyobj;
}

Py::Object LambdaExpression::_getPyValue(int *) const {
    return makeFunc(this,*body,names,args,"<lambda>");
}

///////////////////////////////////////////////////////////////////////////////////

EXPR_TYPESYSTEM_SOURCE(App::FunctionStatement, App::LambdaExpression);

ExpressionPtr FunctionStatement::create(const App::DocumentObject *owner, 
        std::string &&name, ExpressionPtr &&body, StringList &&names, ExpressionList &&args)
{
    checkArgs(names,args);
    _EXPR_NEW(res,FunctionStatement,owner);
    assert(body);
    res->body = std::move(body);
    res->names = std::move(names);
    res->args = std::move(args);
    res->name = std::move(name);
    return _res;
}

void FunctionStatement::_toString(std::ostream &ss, bool persistent, int indent) const {
    ss << "def " << name << '(';
    bool first = true;
    int i=0;
    for(auto &arg : args) {
        auto &name = names[i++];
        if(first)
            first = false;
        else
            ss << ", ";
        ss << name;
        if(arg) {
            ss << '=';
            ss << arg->toStr(persistent);
        }
    }
    ss << "):";
    if(body->isDerivedFrom(Statement::getClassTypeId()))
        ss << std::endl;
    else
        ss << ' ';
    ss << body->toStr(persistent, false, indent+INDENT_STEP);
}

bool FunctionStatement::needLineEnd() const {
    return true;
}

ExpressionPtr FunctionStatement::_copy() const {
    _EXPR_NEW(res,FunctionStatement,owner);
    res->names = names;
    copy_vector(res->args,args);
    res->body = body->copy();
    res->name = name;
    return _res;
}

Py::Object FunctionStatement::_getPyValue(int *) const {
    return makeFunc(this,*body,names,args,name.c_str());
}

///////////////////////////////////////////////////////////////////////////////////

EXPR_TYPESYSTEM_SOURCE(App::DelStatement, App::BaseStatement);

ExpressionPtr DelStatement::create(const App::DocumentObject *owner, ExpressionList &&targets) {
    _EXPR_NEW(res,DelStatement,owner);
    res->targets = std::move(targets);
    return _res;
}

void DelStatement::_toString(std::ostream &ss, bool persistent, int) const {
    ss << "del ";
    bool first = true;
    for(auto &e : targets) {
        if(first)
            first = false;
        else
            ss << ", ";
        ss << e->toStr(persistent);
    }
}

bool DelStatement::isTouched() const
{
    for(auto &e : targets) {
        if(e->isTouched())
            return true;
    }
    return false;
}

void DelStatement::_visit(ExpressionVisitor &v) {
    for(auto &e : targets)
        e->visit(v);
}

ExpressionPtr DelStatement::_copy() const {
    _EXPR_NEW(res,DelStatement,owner);
    copy_vector(res->targets,targets);
    return _res;
}

Py::Object DelStatement::_getPyValue(int *) const {
    CHECK_STACK(DelStatement, this);
    auto &frame = *_EvalStack.back();
    for(auto &target : targets) {
        auto e = freecad_dynamic_cast<VariableExpression>(target.get());
        if(!e) EXPR_THROW("Invalid expression");
        std::string name;
        VarInfo info(e->push(this,true,&name));
        if(!info.component)
            frame.erase(this,name);
        else
            info.component->del(this,info.rhs);
    }
    return Py::Object();
}

///////////////////////////////////////////////////////////////////////////////////

EXPR_TYPESYSTEM_SOURCE(App::ScopeStatement, App::BaseStatement);

ExpressionPtr ScopeStatement::create(const App::DocumentObject *owner, StringList &&names, bool global) {
    _EXPR_NEW(res,ScopeStatement,owner);
    res->names = std::move(names);
    res->global = global;
    return _res;
}

void ScopeStatement::_toString(std::ostream &ss, bool, int) const {
    ss << (global?"global ":"nonlocal ");
    bool first = true;
    for(auto &name : names) {
        if(first)
            first = false;
        else
            ss << ", ";
        ss << name;
    }
}

ExpressionPtr ScopeStatement::_copy() const {
    _EXPR_NEW(res,ScopeStatement,owner);
    res->names = names;
    res->global = global;
    return _res;
}

Py::Object ScopeStatement::_getPyValue(int *) const {
    CHECK_STACK(ScopeStatement, this);
    auto &frame = *_EvalStack.back();
    for(auto &name : names) 
        frame.getVar(this,name,global?BindGlobal:BindNonLocal);
    return Py::Object();
}

////////////////////////////////////////////////////////////////////////////////////

EXPR_TYPESYSTEM_SOURCE(App::TryStatement, App::BaseStatement);

ExpressionPtr TryStatement::create(const App::DocumentObject *owner, ExpressionPtr &&body) {
    _EXPR_NEW(res,TryStatement,owner);
    assert(body);
    res->body = std::move(body);
    return _res;
}

void TryStatement::add(ExpressionPtr &&body, ExpressionPtr &&expr, std::string &&name) {
    assert(body);
    bodies.push_back(std::move(body));
    exprs.push_back(std::move(expr));
    names.push_back(std::move(name));
}

void TryStatement::addElse(ExpressionPtr &&body) {
    assert(body);
    if(else_body)
        PARSER_THROW("Multiple 'else' in try statement");
    if(final_body)
        PARSER_THROW("Invalid 'else' after 'finally'");
    else_body = std::move(body);
}

void TryStatement::addFinal(ExpressionPtr &&body) {
    assert(body);
    if(final_body)
        PARSER_THROW("Multiple 'finally' in try statement");
    final_body = std::move(body);
}

void TryStatement::_toString(std::ostream &ss, bool persistent, int indent) const {
    ss << "try:";
    if(body->isDerivedFrom(Statement::getClassTypeId()))
        ss << std::endl;
    else
        ss << ' ';
    ss << body->toStr(persistent,false,indent+INDENT_STEP);
    for(size_t i=0;i<bodies.size();++i) {
        ss << std::endl;
        printIndent(ss,indent);
        ss << "except";
        if(exprs[i]) {
            ss << ' ' << exprs[i]->toStr(persistent);
            if(names[i].size()) 
                ss << " as " << names[i];
        }
        ss << ':';
        if(bodies[i]->isDerivedFrom(Statement::getClassTypeId()))
            ss << std::endl;
        else
            ss << ' ';
        ss << bodies[i]->toStr(persistent,false,indent+INDENT_STEP);
    }
    if(else_body) {
        ss << std::endl;
        printIndent(ss,indent);
        ss << "else:";
        if(else_body->isDerivedFrom(Statement::getClassTypeId()))
            ss << std::endl;
        else
            ss << ' ';
        ss << else_body->toStr(persistent,false,indent+INDENT_STEP);
    }
    if(final_body) {
        ss << std::endl;
        printIndent(ss,indent);
        ss << "finally:";
        if(final_body->isDerivedFrom(Statement::getClassTypeId()))
            ss << std::endl;
        else
            ss << ' ';
        ss << final_body->toStr(persistent,false,indent+INDENT_STEP);
    }
}

void TryStatement::check() {
    if(bodies.size() || final_body)
        return;
    PARSER_THROW("Invalid try statement");
}

ExpressionPtr TryStatement::_copy() const {
    _EXPR_NEW(res,TryStatement,owner);
    res->body = body->copy();
    res->bodies.reserve(bodies.size());
    res->names.reserve(bodies.size());
    res->exprs.reserve(exprs.size());
    for(size_t i=0;i<bodies.size();++i) {
        res->bodies.push_back(bodies[i]->copy());
        res->exprs.push_back(exprs[i]?exprs[i]->copy():ExpressionPtr());
        res->names.push_back(names[i]);
    }
    if(else_body)
        res->else_body = else_body->copy();
    if(final_body)
        res->final_body = final_body->copy();
    return _res;
}

Py::Object TryStatement::_getPyValue(int *jumpCode) const {
    CHECK_STACK(TryStatement, this);
    Py::Object res;
    try {
        res = body->getPyValue(0,jumpCode);
    } catch (Base::AbortException &) {
        throw;
    } catch (Base::Exception &e) {
        PyObject *type = e.getPyExceptionType();
        if(!type)
            type = PyExc_BaseException;
        if(!findException(res,jumpCode,e,type))
            throw;
    }
    if(final_body)
        final_body->getPyValue();
    return res;
}

bool TryStatement::findException(Py::Object &res, 
        int *jumpCode, Base::Exception &e, PyObject *pyobj) const
{
    auto &frame = *_EvalStack.back();
    for(size_t i=0;i<bodies.size();++i) {
        if(exprs[i]) {
            Py::Object obj = exprs[i]->getPyValue();
            if(obj.isTuple()) {
                Py::Tuple tuple(obj);
                size_t j=0;
                for(;j<tuple.size();++j) {
                    if(pyobj!=tuple[j].ptr() && !PyObject_IsSubclass(pyobj,tuple[j].ptr()))
                        break;
                }
                if(j>=tuple.size())
                    continue;
            }
            if(pyobj!=obj.ptr() && !PyObject_IsSubclass(pyobj,obj.ptr()))
                continue;
        }
        EvalFrame::Exception exception(e,pyobj);
        if(names[i].size()) 
            *frame.getVar(this,names[i],BindLocal) = frame.lastException;
        res = bodies[i]->getPyValue(0,jumpCode);
        return true;
    }
    if(else_body) {
        EvalFrame::Exception exception(e,pyobj);
        res = else_body->getPyValue(0,jumpCode);
        return true;
    }
    return false;
}

bool TryStatement::needLineEnd() const {
    return true;
}

///////////////////////////////////////////////////////////////////////////////////

EXPR_TYPESYSTEM_SOURCE(App::ImportStatement, App::BaseStatement);

ExpressionPtr ImportStatement::create(const App::DocumentObject *owner, 
        std::string &&module, std::string &&name) 
{
    _EXPR_NEW(res,ImportStatement,owner);
    res->add(std::move(module),std::move(name));
    return _res;
}

void ImportStatement::add(std::string &&module, std::string &&name) {
    modules.push_back(std::move(module));
    names.push_back(std::move(name));
}

void ImportStatement::_toString(std::ostream &ss, bool, int) const {
    ss << "import ";
    bool first = true;
    int i=0;
    for(auto &name : names) {
        if(first)
            first = false;
        else
            ss << ", ";
        ss << modules[i++];
        if(name.size())
            ss << " as " << name;
    }
}

ExpressionPtr ImportStatement::_copy() const {
    _EXPR_NEW(res,ImportStatement,owner);
    res->names = names;
    res->modules = modules;
    return _res;
}

Py::Object ImportStatement::_getPyValue(int *) const {
    CHECK_STACK(ImportStatement, this);
    auto &frame = *_EvalStack.back();
    int i=0;
    for(auto &name : names) {
        auto &module = modules[i++];
        auto pymod = ImportModules::instance()->getModule(module,this);
        if(name.size())
            *frame.getVar(this,name,BindLocalOnly) = pymod;
        else {
            auto pos = module.find('.');
            if(pos == std::string::npos)
                *frame.getVar(this,module,BindLocalOnly) = pymod;
            else
                *frame.getVar(this,module.substr(0,pos),BindLocalOnly) = pymod;
        }
    }
    return Py::Object();
}

///////////////////////////////////////////////////////////////////////////////////

EXPR_TYPESYSTEM_SOURCE(App::FromStatement, App::BaseStatement);

ExpressionPtr FromStatement::create(const App::DocumentObject *owner, 
        std::string &&module, std::string &&tail, std::string &&name) 
{
    _EXPR_NEW(res,FromStatement,owner);
    res->module = std::move(module);
    res->add(std::move(tail), std::move(name));
    return _res;
}

void FromStatement::add(std::string &&tail, std::string &&name) {
    tails.push_back(std::move(tail));
    names.push_back(std::move(name));
}

void FromStatement::_toString(std::ostream &ss, bool, int) const {
    ss << "from " << module << " import ";
    bool first = true;
    int i=0;
    for(auto &name : names) {
        if(first)
            first = false;
        else
            ss << ", ";
        ss << tails[i++];
        if(name.size())
            ss << " as " << name;
    }
}

ExpressionPtr FromStatement::_copy() const {
    _EXPR_NEW(res,FromStatement,owner);
    res->names = names;
    res->tails = tails;
    res->module = module;
    return _res;
}

Py::Object FromStatement::_getPyValue(int *) const {
    CHECK_STACK(FromStatement, this);
    auto &frame = *_EvalStack.back();
    int i=0;
    auto pymod = ImportModules::instance()->getModule(module,this);
    for(auto &name : names) {
        auto &tail = tails[i++];
        if(tail == "*") {
            if(pymod.hasAttr("__all__")) {
                Py::Sequence seq(pymod.getAttr("__all__"));
                for(size_t j=0;j<seq.size();++j) {
                    std::string n(Py::Object(seq[i]).as_string());
                    *frame.getVar(this,n,BindLocalOnly) = pymod.getAttr(n);
                }
            }
        }else if(pymod.hasAttr(tail)) {
            *frame.getVar(this,name.size()?name:tail,BindLocalOnly) = pymod.getAttr(tail);
        }else {
            std::string submod(module);
            submod += ".";
            submod += tail;
            auto pysubmod = ImportModules::instance()->getModule(submod,this);
            *frame.getVar(this,name.size()?name:tail,BindLocalOnly) = pysubmod;
        }
    }
    return Py::Object();
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

void clearWarning() {
    _EvalWarned.clear();
}

bool isModuleImported(PyObject *module) {
    return ImportModules::instance()->isImported(module);
}

bool check_text(const char *txt, const char *prefix) {
    for(;*txt && std::isspace((unsigned char)txt[0]);++txt);
    return boost::starts_with(txt,prefix);
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

typedef struct yy_buffer_state *YY_BUFFER_STATE;

struct Context {

    ExpressionPtr ScanResult;                    /**< The resulting expression after a successful parsing */
    const DocumentObject *obj;                  /**< The DocumentObject that will own the expression */
    bool unitExpression = false;                    /**< True if the parsed string is a unit only */
    bool valueExpression = false;                   /**< True if the parsed string is a full expression */

    // lexer state variables
    int nl_count=0; // newline token count
    int last_column=0;
    int column=0;
    int last_token=0;
    int bracket_count=0;
    bool line_continue = false;

    // For the sake of performance, we make the followings static
    static std::vector<int> IndentStack;
    static std::vector<char> ScannerBuffer; 

    static bool busy;

    const char *input;
    YY_BUFFER_STATE lexer_buffer;

    Context(const char *buf, std::size_t len, const DocumentObject *obj=0, bool pythonMode=false);
    ~Context();

    std::string getErrorContext(const std::string &msg);
};

std::vector<int> Context::IndentStack;
std::vector<char> Context::ScannerBuffer;
bool Context::busy;

using ::App::ExpressionPtr;
typedef ::App::Expression::ComponentPtr             ComponentPtr;
typedef ::App::Expression::ExpressionList           ExpressionList;
typedef ::App::Expression::StringList               StringList;
typedef std::pair<ExpressionList, int>              VarList;
typedef std::pair<std::string, Base::Quantity>      UnitInfo;
typedef std::pair<std::string, ExpressionPtr>       NamedArgument;
typedef std::pair<StringList, ExpressionList>       NamedArgumentList;
typedef ::App::ListExpression::FlagExpression       FlagExpression;
typedef ::App::ListExpression::FlagExpressionList   FlagExpressionList;
typedef long long int                               Integer;

} // end of namespace ExpressionParser
} // end of namespace App

#define ExpressionParserStack std::vector<T,ExpressionAllocator(T) >

#include "ExpressionParser.tab.cc"

namespace App {
namespace ExpressionParser {

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

void parser::error(const std::string& m) {
    throw parser::syntax_error(m);
}

Context::Context(const char *buf, std::size_t len, 
        const DocumentObject *obj, bool pythonMode)
    :obj(obj),input(buf)
{
    if(busy) {
        // We can do recursive parsing and scanning by simply setup multiple
        // lexer_buffer, and call yy_switch_to_buffer() when resuming. But
        // there is no need for recursive parsing at the moment, as we never
        // evaluate during parsing. So we disable it, and use a static
        // ScannerBuffer to save memory allocation time.
        throw Base::RuntimeError("Recursive parsing is not supported");
    }
    IndentStack.resize(1,0);
    busy = true;
    if(!len)
        len = std::strlen(buf);
    ScannerBuffer.clear();
    ScannerBuffer.reserve(len);
    ScannerBuffer.insert(ScannerBuffer.end(),buf,buf+len);
    ScannerBuffer.push_back(0);
    ScannerBuffer.push_back(0);
    lexer_buffer = yy_scan_buffer(&ScannerBuffer[0],ScannerBuffer.size());

    // yy_init and yy_start is internal variables defined by flex generated
    // scanner. By right, we should have called yy_init_globals() before and
    // yylex_destroy() after using scanner. We didn't do that to save the need
    // of reallocating an internal buffer stack, which we don't really use.
    yy_init = 0;
    yy_start = 0;

    if(pythonMode)
        BEGIN py_mode;
}

Context::~Context() {
    busy = false;
    yy_delete_buffer(lexer_buffer);
}

std::string Context::getErrorContext(const std::string &msg) {
    std::ostringstream ss;
    ss << msg;
    const char *cur = lexer_buffer->yy_buf_pos + 
        strlen(lexer_buffer->yy_buf_pos) - yyget_leng();
    cur = input + (cur - lexer_buffer->yy_ch_buf);
    const char *start = cur;
    const char *buf = input;
    if(start!=buf && *start=='\n')
        --start;
    for(;start!=buf;--start) {
        if(*start == '\n') {
            ++start;
            break;
        }
    }
    const char *end = cur;
    for(;*end;++end) {
        if(*end == '\n')
            break;
    }
    if(start!=end) {
        ss << std::endl;
        size_t count = cur-start;
        for(size_t i=0;i<count;++i)
            ss << ' ';
        ss << 'v' << std::endl;
        ss.write(start,end-start);
    }
    return ss.str();
}
    
std::vector<boost::tuple<int, int, std::string> > tokenize(const std::string &str)
{
    Context ctx(str.c_str(),str.size());
    std::vector<boost::tuple<int, int, std::string> > result;
    int token;
    try {
        while ( (token  = yylex(0,ctx)) != 0)
            result.push_back(boost::make_tuple(token, ctx.last_column, yytext));
    }
    catch (...) {
        // Ignore all exceptions
    }
    return result;
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

ExpressionPtr parse(const App::DocumentObject *owner, 
        const char* buffer, std::size_t len, bool verbose, bool pythonMode)
{
    Context ctx(buffer,len, owner, pythonMode);

    ExpressionParser::parser parser(ctx);

    int result;
    try {
        result = parser.parse ();
    }catch (parser::syntax_error &e) {
        if(!verbose)
            throw ParserError(e.what());
        throw ParserError(ctx.getErrorContext(e.what()));
    }catch (ParserError &e) {
        if(!verbose)
            throw;
        throw ParserError(ctx.getErrorContext(e.what()));
    }

    if (result != 0)
        throw ParserError("Failed to parse expression.");

    if (!ctx.ScanResult)
        throw ParserError("Unknown error in expression");

    if (ctx.valueExpression)
        return std::move(ctx.ScanResult);
    else {
        throw Expression::Exception("Expression can not evaluate to a value.");
    }
}

ExpressionPtr parseUnit(const App::DocumentObject *owner, const char* buffer, std::size_t len)
{
    Context ctx(buffer, len, owner);

    ExpressionParser::parser parser(ctx);
    int result;

    try {
        result = parser.parse ();
    }catch(parser::syntax_error &e) {
        throw ParserError(e.what());
    }

    if (result != 0)
        throw ParserError("Failed to parse expression.");

    if (ctx.ScanResult == 0)
        throw ParserError("Unknown error in expression");

    // Simplify expression
    auto simplified = ctx.ScanResult->simplify();

    if (!ctx.unitExpression) {
        OperatorExpression * fraction = freecad_dynamic_cast<OperatorExpression>(ctx.ScanResult.get());

        if (fraction && fraction->getOperator() == OP_DIV) {
            auto nom = freecad_dynamic_cast<const NumberExpression>(fraction->getLeft());
            auto denom = freecad_dynamic_cast<const UnitExpression>(fraction->getRight());

            // If not initially a unit expression, but value is equal to 1, it means the expression is something like 1/unit
            if (denom && nom && essentiallyEqual(nom->getValue(), 1.0))
                ctx.unitExpression = true;
        }
    }
    if (ctx.unitExpression) {
        NumberExpression * num = freecad_dynamic_cast<NumberExpression>(simplified.get());

        if (num) {
            simplified = UnitExpression::create(num->getOwner(), num->getQuantity(), "");
        }
        return std::move(simplified);
    }
    else {
        throw Expression::Exception("Expression is not a unit.");
    }
}

bool isTokenAnIndentifier(const std::string & str)
{
    Context ctx(str.c_str(),str.size());
    bool res = false;
    try {
        int token = yylex(0,ctx);
        int status = yylex(0,ctx);
        if (status == 0 && (token == GET_TOKEN(TOK_IDENTIFIER) || token == GET_TOKEN(TOK_CELLADDRESS) ))
            res = true;
    }catch(...) {}
    return res;
}

bool isTokenAUnit(const std::string & str)
{
    Context ctx(str.c_str(),str.size());
    bool res = false;
    try {
        int token = yylex(0,ctx);
        int status = yylex(0,ctx);
        if (status == 0 && token == GET_TOKEN(TOK_UNIT))
            res = true;
    }catch(...) {}
    return res;
}

int translateToken(int token) {
    if(!token)
        return FC_TOK_END;
    static std::map<int,int> tokenMap;
    if(tokenMap.empty()) {
#define MAP_TOKEN(_t,_v) tokenMap[GET_TOKEN(_t)] = FC_TOK_##_v
        MAP_TOKEN(TOK_PSTRING,STRING);
        MAP_TOKEN(TOK_STRING,STRING);
        MAP_TOKEN(TOK_IF,KEYWORD);
        MAP_TOKEN(TOK_ELIF,KEYWORD);
        MAP_TOKEN(TOK_ELSE,KEYWORD);
        MAP_TOKEN(TOK_WHILE,KEYWORD);
        MAP_TOKEN(TOK_FOR,KEYWORD);
        MAP_TOKEN(TOK_BREAK,KEYWORD);
        MAP_TOKEN(TOK_CONTINUE,KEYWORD);
        MAP_TOKEN(TOK_RETURN,KEYWORD);
        MAP_TOKEN(TOK_IN,KEYWORD);
        MAP_TOKEN(TOK_PY_BEGIN,KEYWORD);
        MAP_TOKEN(TOK_PY_END,KEYWORD);
        MAP_TOKEN(TOK_PASS,KEYWORD);
        MAP_TOKEN(TOK_CONSTANT,KEYWORD);
        MAP_TOKEN(TOK_IS,KEYWORD);
        MAP_TOKEN(TOK_IS_NOT,KEYWORD);
        MAP_TOKEN(TOK_NOT_IN,KEYWORD);
        MAP_TOKEN(TOK_NOT,KEYWORD);
        MAP_TOKEN(TOK_AS,KEYWORD);
        MAP_TOKEN(TOK_RAISE,KEYWORD);
        MAP_TOKEN(TOK_TRY,KEYWORD);
        MAP_TOKEN(TOK_EXCEPT,KEYWORD);
        MAP_TOKEN(TOK_FINALLY,KEYWORD);
        MAP_TOKEN(TOK_LAMBDA,KEYWORD);
        MAP_TOKEN(TOK_FROM,KEYWORD);
        MAP_TOKEN(TOK_IMPORT,KEYWORD);
        MAP_TOKEN(TOK_CELLADDRESS,CELLADDRESS);
        MAP_TOKEN(TOK_IDENTIFIER,IDENTIFIER);
        MAP_TOKEN(TOK_UNIT,UNIT);
        MAP_TOKEN(TOK_ONE,NUMBER);
        MAP_TOKEN(TOK_INTEGER,NUMBER);
        MAP_TOKEN(TOK_NUM,NUMBER);
        MAP_TOKEN(TOK_EQ,OPERATOR);
        MAP_TOKEN(TOK_NEQ,OPERATOR);
        MAP_TOKEN(TOK_LT,OPERATOR);
        MAP_TOKEN(TOK_GT,OPERATOR);
        MAP_TOKEN(TOK_GTE,OPERATOR);
        MAP_TOKEN(TOK_LTE,OPERATOR);
        MAP_TOKEN(TOK_AND_OP,KEYWORD);
        MAP_TOKEN(TOK_OR_OP,KEYWORD);
        MAP_TOKEN(TOK_MUL_ASSIGN,OPERATOR);
        MAP_TOKEN(TOK_DIV_ASSIGN,OPERATOR);
        MAP_TOKEN(TOK_MOD_ASSIGN,OPERATOR);
        MAP_TOKEN(TOK_ADD_ASSIGN,OPERATOR);
        MAP_TOKEN(TOK_SUB_ASSIGN,OPERATOR);
        MAP_TOKEN(TOK_MINUSSIGN,OPERATOR);
        MAP_TOKEN(TOK_EXPAND,OPERATOR);
        tokenMap['+'] = FC_TOK_OPERATOR;
        tokenMap['-'] = FC_TOK_OPERATOR;
        tokenMap['/'] = FC_TOK_OPERATOR;
        tokenMap['*'] = FC_TOK_OPERATOR;
        tokenMap['%'] = FC_TOK_OPERATOR;
        tokenMap['^'] = FC_TOK_OPERATOR;
    }
    auto it = tokenMap.find(token);
    if(it!=tokenMap.end())
        return it->second;
    return FC_TOK_OTHER;
}

} // end of namespace ExpressionParser
} // end of namespace App


