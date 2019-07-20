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

#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdelete-non-virtual-dtor"
#endif

#include <Base/Console.h>
#include "Base/Exception.h"
#include <Base/Interpreter.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentPy.h>
#include <App/DocumentObject.h>
#include <App/PropertyUnits.h>
#include <Base/QuantityPy.h>
#include <QStringList>
#include <string>
#include <sstream>
#include <math.h>
#include <stdio.h>
#include <stack>
#include <deque>
#include <algorithm>
#include "Expression.h"
#include <Base/Unit.h>
#include <App/PropertyUnits.h>
#include <App/ObjectIdentifier.h>
#include <boost/math/special_functions/round.hpp>
#include <boost/math/special_functions/trunc.hpp>

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
    if(_expr) ss << std::endl << (_expr)->toString();\
    throw _e(ss.str().c_str());\
}while(0)

#define _EXPR_THROW(_msg,_expr) __EXPR_THROW(ExpressionError,_msg,_expr)

#define __EXPR_SET_MSG(_e,_msg,_expr) do {\
    std::ostringstream ss;\
    ss << _msg << _e.what();\
    if(_expr) ss << std::endl << (_expr)->toString();\
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

} // namespace App


//
// Expression base-class
//

TYPESYSTEM_SOURCE_ABSTRACT(App::Expression, Base::BaseClass);

Expression::Expression(const DocumentObject *_owner)
    : owner(const_cast<App::DocumentObject*>(_owner)) 
{

}

Expression::~Expression()
{
}

void Expression::visit(ExpressionVisitor &v) {
    _visit(v);
    v.visit(*this);
}

Expression * Expression::parse(const DocumentObject *owner, const std::string &buffer)
{
    return ExpressionParser::parse(owner, buffer.c_str());
}

bool Expression::isSame(const Expression &other) const {
    if(&other == this)
        return true;
    if(getTypeId()!=other.getTypeId())
        return false;
    return comment==other.comment && toString(true) == other.toString(true);
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

Expression *Expression::copy() const {
    auto expr = _copy();
    expr->comment = comment;
    return expr;
}

//
// UnitExpression class
//

TYPESYSTEM_SOURCE(App::UnitExpression, App::Expression);

UnitExpression::UnitExpression(const DocumentObject *_owner, const Base::Quantity & _quantity, const std::string &_unitStr)
    : Expression(_owner)
    , quantity(_quantity)
    , unitStr(_unitStr)
{
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
}

/**
  * Evaluate the expression
  *
  * @returns A NumberExpression set to 1.0.
  */

Expression *UnitExpression::eval() const
{
    return new NumberExpression(owner, quantity);
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

std::string UnitExpression::toString(bool) const
{
    return unitStr;
}

/**
  * Return a copy of the expression.
  */

Expression *UnitExpression::_copy() const
{
    return new UnitExpression(owner, quantity, unitStr);
}

int UnitExpression::priority() const
{
    return 20;
}

//
// NumberExpression class
//

TYPESYSTEM_SOURCE(App::NumberExpression, App::Expression);

NumberExpression::NumberExpression(const DocumentObject *_owner, const Quantity &_quantity)
    : UnitExpression(_owner, _quantity)
{
}

/**
  * Evaluate the expression. For NumberExpressions, it is a simply copy().
  */

Expression * NumberExpression::eval() const
{
    return copy();
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
    return new NumberExpression(owner, quantity);
}

int NumberExpression::priority() const
{
    return 20;
}

/**
  * Negate the stored value.
  */

void NumberExpression::negate()
{
    quantity.setValue(-quantity.getValue());
}

std::string NumberExpression::toString(bool) const
{
    std::stringstream s;
    s << std::setprecision(std::numeric_limits<double>::digits10 + 1) << quantity.getValue();

    /* Trim of any extra spaces */
    //while (s.size() > 0 && s[s.size() - 1] == ' ')
//        s.erase(s.size() - 1);

   return s.str();
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

TYPESYSTEM_SOURCE(App::OperatorExpression, App::Expression);

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


/**
  * Evaluate the expression. Returns a new Expression with the result, or throws
  * an exception if something is wrong, i.e the expression cannot be evaluated.
  */

Expression * OperatorExpression::eval() const
{
    std::unique_ptr<Expression> e1(left->eval());
    NumberExpression * v1;
    std::unique_ptr<Expression> e2(right->eval());
    NumberExpression * v2;
    Expression * output;

    v1 = freecad_dynamic_cast<NumberExpression>(e1.get());
    v2 = freecad_dynamic_cast<NumberExpression>(e2.get());

    if (v1 == 0 || v2 == 0)
        throw ExpressionError("Invalid expression");

    switch (op) {
    case ADD:
        if (v1->getUnit() != v2->getUnit())
            throw ExpressionError("Incompatible units for + operator");
        output = new NumberExpression(owner, v1->getQuantity() + v2->getQuantity());
        break;
    case SUB:
        if (v1->getUnit() != v2->getUnit())
            throw ExpressionError("Incompatible units for - operator");
        output = new NumberExpression(owner, v1->getQuantity()- v2->getQuantity());
        break;
    case MUL:
    case UNIT:
        output = new NumberExpression(owner, v1->getQuantity() * v2->getQuantity());
        break;
    case DIV:
        output = new NumberExpression(owner, v1->getQuantity() / v2->getQuantity());
        break;
    case POW:
        output = new NumberExpression(owner, v1->getQuantity().pow(v2->getQuantity()) );
        break;
    case EQ:
        if (v1->getUnit() != v2->getUnit())
            throw ExpressionError("Incompatible units for the = operator");
        output = new BooleanExpression(owner, essentiallyEqual(v1->getValue(), v2->getValue()) );
        break;
    case NEQ:
        if (v1->getUnit() != v2->getUnit())
            throw ExpressionError("Incompatible units for the != operator");
        output = new BooleanExpression(owner, !essentiallyEqual(v1->getValue(), v2->getValue()) );
        break;
    case LT:
        if (v1->getUnit() != v2->getUnit())
            throw ExpressionError("Incompatible units for the < operator");
        output = new BooleanExpression(owner, definitelyLessThan(v1->getValue(), v2->getValue()) );
        break;
    case GT:
        if (v1->getUnit() != v2->getUnit())
            throw ExpressionError("Incompatible units for the > operator");
        output = new BooleanExpression(owner, definitelyGreaterThan(v1->getValue(), v2->getValue()) );
        break;
    case LTE:
        if (v1->getUnit() != v2->getUnit())
            throw ExpressionError("Incompatible units for the <= operator");
        output = new BooleanExpression(owner, definitelyLessThan(v1->getValue(), v2->getValue()) ||
                                       essentiallyEqual(v1->getValue(), v2->getValue()));
        break;
    case GTE:
        if (v1->getUnit() != v2->getUnit())
            throw ExpressionError("Incompatible units for the >= operator");
        output = new BooleanExpression(owner, essentiallyEqual(v1->getValue(), v2->getValue()) ||
                                       definitelyGreaterThan(v1->getValue(), v2->getValue()));
        break;
    case NEG:
        output = new NumberExpression(owner, -v1->getQuantity() );
        break;
    case POS:
        output = new NumberExpression(owner, v1->getQuantity() );
        break;
    default:
        output = 0;
        assert(0);
    }

    return output;
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

std::string OperatorExpression::toString(bool persistent) const
{
    std::stringstream s;
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
        return s.str();
    case POS:
        s << "+" << (needsParens ? "(" : "") << left->toString(persistent) << (needsParens ? ")" : "");
        return s.str();
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

    if (needsParens)
        s << "(" << right->toString(persistent) << ")";
    else
        s << right->toString(persistent);

    return s.str();
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

TYPESYSTEM_SOURCE(App::FunctionExpression, App::UnitExpression);

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
        if (args.size() != 1)
            throw ExpressionError("Invalid number of arguments: exactly one required.");
        break;
    case MOD:
    case ATAN2:
    case POW:
        if (args.size() != 2)
            throw ExpressionError("Invalid number of arguments: exactly two required.");
        break;
    case HYPOT:
    case CATH:
        if (args.size() < 2 || args.size() > 3)
            throw ExpressionError("Invalid number of arguments: exactly two, or three required.");
        break;
    case STDDEV:
    case SUM:
    case AVERAGE:
    case COUNT:
    case MIN:
    case MAX:
        if (args.size() == 0)
            throw ExpressionError("Invalid number of arguments: at least one required.");
        break;
    case NONE:
    case AGGREGATES:
    case LAST:
    default:
        throw ExpressionError("Unknown function");
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
  * Determinte whether the expressions is considered touched, i.e one or both of its arguments
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

Expression * FunctionExpression::evalAggregate() const
{
    boost::shared_ptr<Collector> c;

    switch (f) {
    case SUM:
        c = boost::shared_ptr<Collector>(new SumCollector());
        break;
    case AVERAGE:
        c = boost::shared_ptr<Collector>(new AverageCollector());
        break;
    case STDDEV:
        c = boost::shared_ptr<Collector>(new StdDevCollector());
        break;
    case COUNT:
        c = boost::shared_ptr<Collector>(new CountCollector());
        break;
    case MIN:
        c = boost::shared_ptr<Collector>(new MinCollector());
        break;
    case MAX:
        c = boost::shared_ptr<Collector>(new MaxCollector());
        break;
    default:
        assert(false);
    }

    for (size_t i = 0; i< args.size(); ++i) {
        if (args[i]->isDerivedFrom(RangeExpression::getClassTypeId())) {
            RangeExpression * v = static_cast<RangeExpression*>(args[i]);
            Range range(v->getRange());

            do {
                Property * p = owner->getPropertyByName(range.address().c_str());
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
                    throw Exception("Invalid property type for aggregate");
            } while (range.next());
        }
        else {
            std::unique_ptr<Expression> e(args[i]->eval());
            NumberExpression * n(freecad_dynamic_cast<NumberExpression>(e.get()));

            if (n)
                c->collect(n->getQuantity());
        }
    }

    return new NumberExpression(owner, c->getQuantity());
}

/**
  * Evaluate function. Returns a NumberExpression if evaluation is successful.
  * Throws an ExpressionError exception if something fails.
  *
  * @returns A NumberExpression with the result.
  */

Expression * FunctionExpression::eval() const
{
    // Handle aggregate functions
    if (f > AGGREGATES)
        return evalAggregate();

    std::unique_ptr<Expression> e1(args[0]->eval());
    std::unique_ptr<Expression> e2(args.size() > 1 ? args[1]->eval() : 0);
    std::unique_ptr<Expression> e3(args.size() > 2 ? args[2]->eval() : 0);
    NumberExpression * v1 = freecad_dynamic_cast<NumberExpression>(e1.get());
    NumberExpression * v2 = freecad_dynamic_cast<NumberExpression>(e2.get());
    NumberExpression * v3 = freecad_dynamic_cast<NumberExpression>(e3.get());
    double output;
    Unit unit;
    double scaler = 1;

    if (v1 == 0)
        throw ExpressionError("Invalid argument.");

    double value = v1->getValue();

    /* Check units and arguments */
    switch (f) {
    case COS:
    case SIN:
    case TAN:
        if (!(v1->getUnit() == Unit::Angle || v1->getUnit().isEmpty()))
            throw ExpressionError("Unit must be either empty or an angle.");

        // Convert value to radians
        value *= M_PI / 180.0;
        unit = Unit();
        break;
    case ACOS:
    case ASIN:
    case ATAN:
        if (!v1->getUnit().isEmpty())
            throw ExpressionError("Unit must be empty.");
        unit = Unit::Angle;
        scaler = 180.0 / M_PI;
        break;
    case EXP:
    case LOG:
    case LOG10:
    case SINH:
    case TANH:
    case COSH:
        if (!v1->getUnit().isEmpty())
            throw ExpressionError("Unit must be empty.");
        unit = Unit();
        break;
    case ROUND:
    case TRUNC:
    case CEIL:
    case FLOOR:
    case ABS:
        unit = v1->getUnit();
        break;
    case SQRT: {
        unit = v1->getUnit();

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
            throw ExpressionError("All dimensions must be even to compute the square root.");

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
        if (v2 == 0)
            throw ExpressionError("Invalid second argument.");

        if (v1->getUnit() != v2->getUnit())
            throw ExpressionError("Units must be equal");
        unit = Unit::Angle;
        scaler = 180.0 / M_PI;
        break;
    case MOD:
        if (v2 == 0)
            throw ExpressionError("Invalid second argument.");
        unit = v1->getUnit() / v2->getUnit();
        break;
    case POW: {
        if (v2 == 0)
            throw ExpressionError("Invalid second argument.");

        if (!v2->getUnit().isEmpty())
            throw ExpressionError("Exponent is not allowed to have a unit.");

        // Compute new unit for exponentiation
        double exponent = v2->getValue();
        if (!v1->getUnit().isEmpty()) {
            if (exponent - boost::math::round(exponent) < 1e-9)
                unit = v1->getUnit().pow(exponent);
            else
                throw ExpressionError("Exponent must be an integer when used with a unit");
        }
        break;
    }
    case HYPOT:
    case CATH:
        if (v2 == 0)
            throw ExpressionError("Invalid second argument.");
        if (v1->getUnit() != v2->getUnit())
            throw ExpressionError("Units must be equal");

        if (args.size() > 2) {
            if (v3 == 0)
                throw ExpressionError("Invalid second argument.");
            if (v2->getUnit() != v3->getUnit())
                throw ExpressionError("Units must be equal");
        }
        unit = v1->getUnit();
        break;
    default:
        assert(0);
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
        output = fmod(value, v2->getValue());
        break;
    }
    case ATAN2: {
        output = atan2(value, v2->getValue());
        break;
    }
    case POW: {
        output = pow(value, v2->getValue());
        break;
    }
    case HYPOT: {
        output = sqrt(pow(v1->getValue(), 2) + pow(v2->getValue(), 2) + (v3 ? pow(v3->getValue(), 2) : 0));
        break;
    }
    case CATH: {
        output = sqrt(pow(v1->getValue(), 2) - pow(v2->getValue(), 2) - (v3 ? pow(v3->getValue(), 2) : 0));
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
        output = 0;
        assert(0);
    }

    return new NumberExpression(owner, Quantity(scaler * output, unit));
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

std::string FunctionExpression::toString(bool persistent) const
{
    std::stringstream ss;

    for (size_t i = 0; i < args.size(); ++i) {
        ss << args[i]->toString(persistent);
        if (i != args.size() - 1)
            ss << "; ";
    }

    switch (f) {
    case ACOS:
        return "acos(" + ss.str() + ")";
    case ASIN:
        return "asin(" + ss.str() + ")";
    case ATAN:
        return "atan(" + ss.str() + ")";
    case ABS:
        return "abs(" + ss.str() + ")";
    case EXP:
        return "exp(" + ss.str() + ")";
    case LOG:
        return "log(" + ss.str() + ")";
    case LOG10:
        return "log10(" + ss.str() + ")";
    case SIN:
        return "sin(" + ss.str() + ")";
    case SINH:
        return "sinh(" + ss.str() + ")";
    case TAN:
        return "tan(" + ss.str() + ")";
    case TANH:
        return "tanh(" + ss.str() + ")";
    case SQRT:
        return "sqrt(" + ss.str() + ")";
    case COS:
        return "cos(" + ss.str() + ")";
    case COSH:
        return "cosh(" + ss.str() + ")";
    case MOD:
        return "mod(" + ss.str() + ")";
    case ATAN2:
        return "atan2(" + ss.str() + ")";
    case POW:
        return "pow(" + ss.str() + ")";
    case HYPOT:
        return "hypot(" + ss.str() + ")";
    case CATH:
        return "cath(" + ss.str() + ")";
    case ROUND:
        return "round(" + ss.str() + ")";
    case TRUNC:
        return "trunc(" + ss.str() + ")";
    case CEIL:
        return "ceil(" + ss.str() + ")";
    case FLOOR:
        return "floor(" + ss.str() + ")";
    case SUM:
        return "sum(" + ss.str() + ")";
    case COUNT:
        return "count(" + ss.str() + ")";
    case AVERAGE:
        return "average(" + ss.str() + ")";
    case STDDEV:
        return "stddev(" + ss.str() + ")";
    case MIN:
        return "min(" + ss.str() + ")";
    case MAX:
        return "max(" + ss.str() + ")";
    default:
        assert(0);
        return std::string();
    }
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

int FunctionExpression::priority() const
{
    return 20;
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

TYPESYSTEM_SOURCE(App::VariableExpression, App::UnitExpression);

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

/**
  * Evaluate the expression. For a VariableExpression, this means to return the
  * value of the referenced Property. Quantities are converted to NumberExpression with unit,
  * int and floats are converted to a NumberExpression without unit. Strings properties
  * are converted to StringExpression objects.
  *
  * @returns The result of the evaluation, i.e a new (Number|String)Expression object.
  */

Expression * VariableExpression::eval() const
{
    const Property * prop = getProperty();
    PropertyContainer * parent = prop->getContainer();

    if (!parent->isDerivedFrom(App::DocumentObject::getClassTypeId()))
        throw ExpressionError("Property must belong to a document object.");

    boost::any value = prop->getPathValue(var);

    if (value.type() == typeid(Quantity)) {
        Quantity qvalue = boost::any_cast<Quantity>(value);

        return new NumberExpression(owner, qvalue);
    }
    else if (value.type() == typeid(double)) {
        double dvalue = boost::any_cast<double>(value);

        return new NumberExpression(owner, Quantity(dvalue));
    }
    else if (value.type() == typeid(float)) {
        double fvalue = boost::any_cast<float>(value);

        return new NumberExpression(owner, Quantity(fvalue));
    }
    else if (value.type() == typeid(int)) {
        int ivalue = boost::any_cast<int>(value);

        return new NumberExpression(owner, Quantity(ivalue));
    }
    else if (value.type() == typeid(long)) {
        long lvalue = boost::any_cast<long>(value);

        return new NumberExpression(owner, Quantity(lvalue));
    }
    else if (value.type() == typeid(bool)) {
        double bvalue = boost::any_cast<bool>(value) ? 1.0 : 0.0;

        return new NumberExpression(owner, Quantity(bvalue));
    }
    else if (value.type() == typeid(std::string)) {
        std::string svalue = boost::any_cast<std::string>(value);

        return new StringExpression(owner, svalue);
    }
    else if (value.type() == typeid(char*)) {
        char* svalue = boost::any_cast<char*>(value);

        return new StringExpression(owner, svalue);
    }
    else if (value.type() == typeid(const char*)) {
        const char* svalue = boost::any_cast<const char*>(value);

        return new StringExpression(owner, svalue);
    }
    else if (isAnyPyObject(value)) {
        Base::PyGILStateLocker lock;
        return new PyObjectExpression(owner,__pyObjectFromAny(value).ptr());
    }

    throw ExpressionError("Property is of invalid type.");
}

std::string VariableExpression::toString(bool persistent) const {
    if(persistent)
        return var.toPersistentString();
    else
        return var.toString();
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

int VariableExpression::priority() const
{
    return 20;
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

void VariableExpression::setPath(const ObjectIdentifier &path)
{
     var = path;
}

//
// PyObjectExpression class
//

TYPESYSTEM_SOURCE(App::PyObjectExpression, App::Expression);

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

std::string PyObjectExpression::toString(bool) const
{
    if(!pyObj)
        return "None";
    else {
        Base::PyGILStateLocker lock;
        return Py::Object(pyObj).as_string();
    }
}

Expression* PyObjectExpression::_copy() const
{
    return new PyObjectExpression(owner,pyObj,false);
}

boost::any PyObjectExpression::getValueAsAny() const {
    if(!pyObj || pyObj == Py_None)
        return boost::any();
    Base::PyGILStateLocker lock;
    return App::any(pyObjectWrap(pyObj));
}

//
// StringExpression class
//

TYPESYSTEM_SOURCE(App::StringExpression, App::Expression);

StringExpression::StringExpression(const DocumentObject *_owner, const std::string &_text)
    : Expression(_owner)
    , text(_text)
{
}

/**
  * Evaluate the string. For strings, this is a simple copy of the object.
  */

Expression * StringExpression::eval() const
{
    return copy();
}

/**
  * Simplify the expression. For strings, this is a simple copy of the object.
  */

Expression *StringExpression::simplify() const
{
    return copy();
}

std::string StringExpression::toString(bool) const
{
    return quote(text);
}

int StringExpression::priority() const
{
    return 20;
}

/**
  * Return a copy of the expression.
  */

Expression *StringExpression::_copy() const
{
    return new StringExpression(owner, text);
}

TYPESYSTEM_SOURCE(App::ConditionalExpression, App::Expression);

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

Expression *ConditionalExpression::eval() const
{
    std::unique_ptr<Expression> e(condition->eval());
    NumberExpression * v = freecad_dynamic_cast<NumberExpression>(e.get());

    if (v == 0)
        throw ExpressionError("Invalid expression");

    if (fabs(v->getValue()) > 0.5)
        return trueExpr->eval();
    else
        return falseExpr->eval();
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

std::string ConditionalExpression::toString(bool persistent) const
{
    std::string cstr = condition->toString(persistent);
    std::string tstr = trueExpr->toString(persistent);
    std::string fstr = falseExpr->toString(persistent);

    if (trueExpr->priority() <= priority())
        tstr = "(" + tstr + ")";

    if (falseExpr->priority() <= priority())
        fstr = "(" + fstr + ")";

    return cstr + " ? " + tstr + " : " + fstr;
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

TYPESYSTEM_SOURCE(App::ConstantExpression, App::NumberExpression);

ConstantExpression::ConstantExpression(const DocumentObject *_owner, std::string _name, const Quantity & _quantity)
    : NumberExpression(_owner, _quantity)
    , name(_name)
{
}

std::string ConstantExpression::toString(bool) const
{
    return name;
}

Expression *ConstantExpression::_copy() const
{
    return new ConstantExpression(owner, name.c_str(), quantity);
}

int ConstantExpression::priority() const
{
    return 20;
}

TYPESYSTEM_SOURCE_ABSTRACT(App::BooleanExpression, App::NumberExpression);

BooleanExpression::BooleanExpression(const DocumentObject *_owner, bool _value)
    : NumberExpression(_owner, Quantity(_value ? 1.0 : 0.0))
{
}

Expression *BooleanExpression::_copy() const
{
    return new BooleanExpression(owner, getValue() > 0.5 ? true : false);
}

TYPESYSTEM_SOURCE(App::RangeExpression, App::Expression);

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

Expression *RangeExpression::eval() const
{
    throw Exception("Range expression cannot be evaluated");
}

std::string RangeExpression::toString(bool) const
{
    return begin + ":" + end;
}

Expression *RangeExpression::_copy() const
{
    return new RangeExpression(owner, begin, end);
}

int RangeExpression::priority() const
{
    return 20;
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
