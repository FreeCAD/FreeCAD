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
#include <boost/algorithm/string/replace.hpp>
#include <Base/Console.h>
#include <Base/Exception.h>
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
#include <climits>
#include "Expression.h"
#include "ExpressionParser.h"
#include <Base/Unit.h>
#include <App/PropertyUnits.h>
#include <App/ObjectIdentifier.h>
#include <boost/math/special_functions/round.hpp>
#include <boost/math/special_functions/trunc.hpp>
#include "ExpressionPy.h"

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

#define __EXPR_THROW(_msg,_fmt,_e) do {\
    if(_e)\
        FC_LOG(_msg << _fmt << " -- " << _e->toString());\
    else\
        FC_LOG(_msg << _fmt);\
    throw ExpressionError(_msg);\
}while(0)

#define _EXPR_THROW(_msg,_e) __EXPR_THROW(_msg,"",_e)
#define EXPR_THROW(_msg) _EXPR_THROW(_msg,this)

using namespace Base;
using namespace App;

namespace App {
namespace ExpressionParser {

static std::string parser_msg;
void ExpressionParser_yyerror(const char *errorinfo) {
    if(errorinfo) {
        parser_msg = "Failed to parse expression: ";
        parser_msg += errorinfo;
    }
}

} // namespace ExpressionParser
} // namespace App

std::string unquote(const std::string & input, bool r_literal=false)
{
    assert(input.size() >= 4);

    if(r_literal)
        FC_LOG("found");
    std::string output;
    std::string::const_iterator cur = input.begin() + (r_literal?3:2);
    std::string::const_iterator end = input.end() - 2;

    output.reserve(input.size());

    bool escaped = false;
    while (cur != end) {
        if (escaped) {
            switch (*cur) {
            case '>':
                output += '>';
                break;
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
            default:
                output += '\\';
                output += *cur;
            }
            escaped = false;
        }
        else {
            if(!r_literal && *cur == '\\')
                escaped = true;
            else
                output += *cur;
        }
        ++cur;
    }

    return output;
}

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

void ExpressionVisitor::importSubNames(Expression &e, const std::map<std::string,std::string> &subNameMap) {
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

bool ExpressionVisitor::renameDocument(
        Expression &e, const std::string &oldName, const std::string &newName) 
{
    return e._renameDocument(oldName,newName,*this);
}

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

Expression * Expression::parse(const DocumentObject *owner, const std::string &buffer)
{
    return ExpressionParser::parse(owner, buffer.c_str());
}

class GetDepsExpressionVisitor : public ExpressionVisitor {
public:
    GetDepsExpressionVisitor(ExpressionDeps &deps)
        :deps(deps)
    {}

    virtual void visit(Expression * e) {
        if(e) this->getDeps(*e,deps);
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

    virtual void visit(Expression * e) {
        if(e) this->getDepObjects(*e,deps,labels);
    }

    std::set<App::DocumentObject*> &deps;
    std::vector<std::string> *labels;
};

void Expression::getDepObjects(std::set<App::DocumentObject*> &deps, std::vector<std::string> *labels)  const {
    GetDepObjsExpressionVisitor v(deps,labels);
    const_cast<Expression*>(this)->visit(v);
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

    virtual void visit(Expression * e) {
        if(e) this->getIdentifiers(*e,deps);
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

    virtual void visit(Expression * e) {
        if(e && this->adjustLinks(*e,inList))
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
    ImportSubNamesExpressionVisitor(const std::map<std::string,std::string> &subNameMap)
        :subNameMap(subNameMap)
    {}

    virtual void visit(Expression * e) {
        if(e) 
            this->importSubNames(*e,subNameMap);
    }

    const std::map<std::string,std::string> &subNameMap;
};

Expression *Expression::importSubNames(const std::map<std::string,std::string> &nameMap) const {
    std::map<std::string,std::string> subNameMap;
    for(auto &dep : getDeps()) {
        for(auto &info : dep.second) {
            for(auto &path : info.second) {
                auto obj = path.getDocumentObject();
                if(!obj)
                    continue;
                std::string sub = path.getSubObjectName(true);
                if(sub.empty() || subNameMap.count(sub))
                    continue;
                std::string imported = PropertyLinkBase::tryImportSubName(nameMap,obj,sub.c_str());
                if(imported.size())
                    subNameMap.emplace(sub,imported);
            }
        }
    }
    if(subNameMap.empty())
        return 0;
    ImportSubNamesExpressionVisitor v(subNameMap);
    std::unique_ptr<Expression> expr(copy());
    expr->visit(v);
    return expr.release();
}

class UpdateLabelExpressionVisitor : public ExpressionVisitor {
public:
    UpdateLabelExpressionVisitor(App::DocumentObject *obj, const std::string &ref, const char *newLabel)
        :obj(obj),ref(ref),newLabel(newLabel)
    {}

    virtual void visit(Expression * e) {
        if(e) 
            this->updateLabelReference(*e,obj,ref,newLabel);
    }

    App::DocumentObject *obj;
    const std::string &ref;
    const char *newLabel;
};

Expression *Expression::updateLabelReference(
        App::DocumentObject *obj, const std::string &ref, const char *newLabel) const 
{
    std::vector<std::string> labels;
    getDepObjects(&labels);
    if(std::find(labels.begin(),labels.end(),ref) == labels.end())
        return 0;
    UpdateLabelExpressionVisitor v(obj,ref,newLabel);
    std::unique_ptr<Expression> expr(copy());
    expr->visit(v);
    return expr.release();
}

/* The following definitions are from The art of computer programming by Knuth
 * (copied from http://stackoverflow.com/questions/17333/most-effective-way-for-float-and-double-comparison)
 */

/*
static bool approximatelyEqual(double a, double b, double epsilon)
{
    return fabs(a - b) <= ( (fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}
*/

static bool essentiallyEqual(double a, double b, double epsilon)
{
    return fabs(a - b) <= ( (fabs(a) > fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

static bool definitelyGreaterThan(double a, double b, double epsilon)
{
    return (a - b) > ( (fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

static bool definitelyLessThan(double a, double b, double epsilon)
{
    return (b - a) > ( (fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

static Py::Object pyObjectFromAny(boost::any value, const Expression *expr=0) {
    Base::PyGILStateLocker lock;
    if(value.empty())
        return Py::Object();
    else if (value.type() == typeid(Py::Object))
        return boost::any_cast<Py::Object>(value);
    else if (value.type() == typeid(Quantity))
        return Py::Object(new QuantityPy(new Quantity(boost::any_cast<Quantity>(value))));
    else if (value.type() == typeid(double)) {
        double v = boost::any_cast<double>(value);
        double rv = std::round(v);
        const double epsilon = std::numeric_limits<double>::epsilon();
        if(essentiallyEqual(v,rv,epsilon)) {
            long l = (long)rv;
            if(std::abs(l)<INT_MAX)
                return Py::Int(l);
            return Py::Long((long)rv);
        }
        return Py::Float(v);
    } else if (value.type() == typeid(float)) {
        float v = boost::any_cast<float>(value);
        float rv = std::round(v);
        const float epsilon = std::numeric_limits<float>::epsilon();
        if(essentiallyEqual(v,rv,epsilon))
            return Py::Int((int)rv);
        return Py::Float(v);
    } else if (value.type() == typeid(int)) 
        return Py::Int(boost::any_cast<int>(value));
    else if (value.type() == typeid(long))
        return Py::Long(boost::any_cast<long>(value));
    else if (value.type() == typeid(bool))
        return Py::Boolean(boost::any_cast<bool>(value));
    else if (value.type() == typeid(std::string))
        return Py::String(boost::any_cast<string>(value));
    else if (value.type() == typeid(const char*))
        return Py::String(boost::any_cast<const char*>(value));

    try {
        return boost::any_cast<Py::Object>(value);
    }catch(...) {
        _EXPR_THROW("Unknown type",expr);
    }
}

static boost::any getValueByPython(const char *name, const char *cmd, const Expression *expr) {
    Base::PyGILStateLocker lock;
    PyObject * pyvalue = Base::Interpreter().getValue(cmd, name);
    if (!pyvalue)
        _EXPR_THROW("Failed to get value by python",expr);
    return ObjectIdentifier::pyObjectToAny(Py::Object(pyvalue,true));
}

boost::any Expression::getValueAsAny() const {
    if(components.empty())
        return _getValueAsAny();
    PythonVariables vars;
    std::string name = vars.add(pyObjectFromAny(_getValueAsAny(),this));
    std::ostringstream ss;
    ss << name << '=' << name;
    for(auto &c : components) {
        if(c.isSimple())
            ss << '.';
        c.toString(ss,true);
    }
    return getValueByPython(name.c_str(),ss.str().c_str(),this);
}

Expression *Expression::eval() const {
    if(components.empty()) {
        Expression *res = _eval();
        if(res) 
            return res;
    }
    return fromAny(getValueAsAny());
}

static Expression *checkRLiteral(App::DocumentObject *owner, const std::string &s) {
    bool r_literal = false;
    for(char c : s) {
        switch(c) {
        case '\\':
            r_literal = true;
            break;
        case '\n':
        case '\r':
        case '\t':
            return new StringExpression(owner,s);
        }
    }
    return new StringExpression(owner,s,r_literal);
}

Expression *Expression::fromAny(boost::any value) const {
    Base::PyGILStateLocker lock;
    if (value.empty())
        return new PyObjectExpression(owner);
    else if (value.type() == typeid(Py::Object))
        return new PyObjectExpression(owner,boost::any_cast<Py::Object>(value));
    else if (value.type() == typeid(std::string))
        return checkRLiteral(owner,boost::any_cast<std::string>(value));
    else if (value.type() == typeid(Quantity))
        return new NumberExpression(owner,boost::any_cast<Quantity>(value));
    else if (value.type() == typeid(bool))
        return new BooleanExpression(owner,boost::any_cast<bool>(value));
    else if (value.type() == typeid(int))
        return new NumberExpression(owner,Quantity(boost::any_cast<int>(value)));
    else if (value.type() == typeid(long))
        return new NumberExpression(owner,Quantity(boost::any_cast<long>(value)));
    else if (value.type() == typeid(float))
        return new NumberExpression(owner,Quantity(boost::any_cast<float>(value)));
    else if (value.type() == typeid(double))
        return new NumberExpression(owner,Quantity(boost::any_cast<double>(value)));
    else if (value.type() == typeid(const char*)) 
        return checkRLiteral(owner,boost::any_cast<const char *>(value));
    else
        EXPR_THROW("Failed to evaluate");
}

std::string Expression::toString(bool persistent) const {
    std::ostringstream ss;
    if(components.empty()) {
        _toString(ss,persistent);
        return ss.str();
    }
    if(!_isIndexable()) {
        ss << '(';
        _toString(ss,persistent);
        ss << ')';
    }else
        _toString(ss,persistent);
    for(auto &c : components) {
        if(c.isSimple())
            ss << '.';
        c.toString(ss,false);
    }
    return ss.str();
}

Expression *Expression::copy() const {
    auto expr = _copy();
    assert(expr);
    if(expr->components.empty() && components.size())
        expr->components = components;
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
  * Evaulate the expression
  *
  * @returns A NumberExpression set to 1.0.
  */

Expression *UnitExpression::_eval() const {
    assert(components.empty());
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

void UnitExpression::_toString(std::ostringstream &ss, bool) const
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

void NumberExpression::_toString(std::ostringstream &ss, bool) const
{
    ss << std::setprecision(std::numeric_limits<double>::digits10 + 2) << quantity.getValue();

    /* Trim of any extra spaces */
    //while (s.size() > 0 && s[s.size() - 1] == ' ')
//        s.erase(s.size() - 1);
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

Expression * OperatorExpression::_eval() const
{
    std::unique_ptr<Expression> e1(left->eval());
    auto output = _calc(e1.get());
    if(output)
        return output;
    std::unique_ptr<Expression> e2(right->eval());
    output = _calc(e1.get(),e2.get());
    if(output)
        return output;
    return fromAny(_calc(e1->getValueAsAny(),e2->getValueAsAny()));
}

Expression *OperatorExpression::_calc(Expression *e1) const {
    const double epsilon = std::numeric_limits<double>::epsilon();
    switch(op) {
    case AND_OP2:
    case AND_OP: {
        auto v1 = freecad_dynamic_cast<NumberExpression>(e1);
        if((v1 && essentiallyEqual(v1->getValue(), 0.0, epsilon)) ||
           (!v1 && !pyObjectFromAny(v1->getValueAsAny()).isTrue()))
            return new BooleanExpression(owner, false);
        break;
    }
    case OR_OP2:
    case OR_OP: {
        auto v1 = freecad_dynamic_cast<NumberExpression>(e1);
        if((v1 && !essentiallyEqual(v1->getValue(), 0.0, epsilon)) ||
           (!v1 && pyObjectFromAny(v1->getValueAsAny()).isTrue()))
            return new BooleanExpression(owner, true);
        break;
    }
    default:
        break;
    }
    return 0;
}
        
boost::any OperatorExpression::_getValueAsAny() const {
    std::unique_ptr<Expression> e1(left->eval());
    std::unique_ptr<Expression> expr(_calc(e1.get()));
    if(expr)
        return expr->getValueAsAny();
    std::unique_ptr<Expression> e2(right->eval());
    expr.reset(_calc(e1.get(),e2.get()));
    if(expr)
        return expr->getValueAsAny();
    return _calc(e1->getValueAsAny(),e2->getValueAsAny());
}

Expression *OperatorExpression::_calc(Expression *e1, Expression *e2) const {
    NumberExpression *v1, *v2;
    Expression * output;
    const double epsilon = std::numeric_limits<double>::epsilon();
    v1 = freecad_dynamic_cast<NumberExpression>(e1);
    v2 = freecad_dynamic_cast<NumberExpression>(e2);

    if (v1 == 0 || v2 == 0)
        return 0;

    switch (op) {
    case ADD:
        if (v1->getUnit() != v2->getUnit())
            EXPR_THROW("Incompatible units for + operator");
        output = new NumberExpression(owner, v1->getQuantity() + v2->getQuantity());
        break;
    case SUB:
        if (v1->getUnit() != v2->getUnit())
            EXPR_THROW("Incompatible units for - operator");
        output = new NumberExpression(owner, v1->getQuantity()- v2->getQuantity());
        break;
    case MOD:
        output = new NumberExpression(owner, 
                Quantity(fmod(v1->getValue(),v2->getValue()),v1->getUnit()/v2->getUnit()));
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
            EXPR_THROW("Incompatible units for the = operator");
        output = new BooleanExpression(owner, essentiallyEqual(v1->getValue(), v2->getValue(), epsilon) );
        break;
    case NEQ:
        if (v1->getUnit() != v2->getUnit())
            EXPR_THROW("Incompatible units for the != operator");
        output = new BooleanExpression(owner, !essentiallyEqual(v1->getValue(), v2->getValue(), epsilon) );
        break;
    case LT:
        if (v1->getUnit() != v2->getUnit())
            EXPR_THROW("Incompatible units for the < operator");
        output = new BooleanExpression(owner, definitelyLessThan(v1->getValue(), v2->getValue(), epsilon) );
        break;
    case GT:
        if (v1->getUnit() != v2->getUnit())
            EXPR_THROW("Incompatible units for the > operator");
        output = new BooleanExpression(owner, definitelyGreaterThan(v1->getValue(), v2->getValue(), epsilon) );
        break;
    case LTE:
        if (v1->getUnit() != v2->getUnit())
            EXPR_THROW("Incompatible units for the <= operator");
        output = new BooleanExpression(owner, definitelyLessThan(v1->getValue(), v2->getValue(), epsilon) ||
                                       essentiallyEqual(v1->getValue(), v2->getValue(), epsilon));
        break;
    case GTE:
        if (v1->getUnit() != v2->getUnit())
            EXPR_THROW("Incompatible units for the >= operator");
        output = new BooleanExpression(owner, essentiallyEqual(v1->getValue(), v2->getValue(), epsilon) ||
                                       definitelyGreaterThan(v1->getValue(), v2->getValue(), epsilon));
        break;
    case AND_OP:
    case AND_OP2:
        output = new BooleanExpression(owner, !essentiallyEqual(v2->getValue(), 0.0, epsilon));
        break;
    case OR_OP:
    case OR_OP2:
        output = new BooleanExpression(owner, !essentiallyEqual(v2->getValue(), 0.0, epsilon));
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

boost::any OperatorExpression::_calc(boost::any l, boost::any r) const {
    switch(op) {
    case AND_OP:
    case AND_OP2:
    case OR_OP:
    case OR_OP2:
        return boost::any(pyObjectFromAny(r).isTrue());
    default:
        break;
    }
    PythonVariables vars;
    std::string var1 = vars.add(pyObjectFromAny(l,left));
    std::string var2 = vars.add(pyObjectFromAny(r,right));
    std::ostringstream ss;
    ss << var1 << "=";
    switch (op) {
    case ADD:
        ss << var1 << '+' << var2;
        break;
    case SUB:
        ss << var1 << '-' << var2;
        break;
    case MUL:
    case UNIT:
        ss << var1 << '*' << var2;
        break;
    case MOD:
        ss << var1 << '%' << var2;
        break;
    case DIV:
        ss << var1 << '/' << var2;
        break;
    case POW:
        ss << var1 << '^' << var2;
        break;
    case EQ:
        ss << var1 << "==" << var2;
        break;
    case NEQ:
        ss << var1 << "!=" << var2;
        break;
    case LT:
        ss << var1 << "<" << var2;
        break;
    case GT:
        ss << var1 << ">" << var2;
        break;
    case LTE:
        ss << var1 << "<=" << var2;
        break;
    case GTE:
        ss << var1 << ">=" << var2;
        break;
    case NEG:
        ss << '-' << var1;
        break;
    case POS:
        ss << '+' << var1;
        break;
    default:
        assert(0);
    }
    return getValueByPython(var1.c_str(),ss.str().c_str(),this);
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

void OperatorExpression::_toString(std::ostringstream &s, bool persistent) const
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
    case AND_OP:
        s << " && ";
        break;
    case AND_OP2:
        s << " and ";
        break;
    case OR_OP:
        s << " || ";
        break;
    case OR_OP2:
        s << " or ";
        break;
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

    if (needsParens)
        s << "(" << right->toString(persistent) << ")";
    else
        s << right->toString(persistent);
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
    case OR_OP:
    case OR_OP2:
        return 1;
        break;
    case AND_OP:
    case AND_OP2:
        return 2;
        break;
    case EQ:
    case NEQ:
        return 3;
        break;
    case LT:
    case GT:
    case LTE:
    case GTE:
        return 4;
    case ADD:
    case SUB:
        return 5;
    case MOD:
    case MUL:
    case DIV:
        return 6;
    case POW:
        return 7;
    case UNIT:
    case NEG:
    case POS:
        return 8;
    default:
        assert(false);
        return 0;
    }
}

void OperatorExpression::visit(ExpressionVisitor &v)
{
    if (left)
        left->visit(v);
    if (right)
        right->visit(v);
    v.visit(this);
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

enum ExpressionFunctionType {
    NONE,

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
    POW,
    ROUND,
    TRUNC,
    CEIL,
    FLOOR,
    HYPOT,
    CATH,

    GET_VAR,
    HAS_VAR,
    EVAL,
    FUNC,
    FOR_EACH,

    PY_START,
    PY_ALL,
    PY_EACH,
    PY_BIN,
    PY_DICT,
    PY_ENUMERATE,
    PY_FILTER,
    PY_FLOAT,
    PY_FORMAT,
    PY_FROZENSET,
    PY_GLOBALS,
    PY_GETATTR,
    PY_HASATTR,
    PY_HASH,
    PY_HEX,
    PY_ID,
    PY_INT,
    PY_LEN,
    PY_LIST,
    PY_MAP,
    PY_OCT,
    PY_STR,
    PY_RANGE,
    PY_REPR,
    PY_REVERSED,
    PY_SET,
    PY_SLICE,
    PY_SORTED,
    PY_TUPLE,
    PY_ZIP,
    PY_END,

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

static std::map<std::string, int> registered_functions; 
static std::map<int, std::string> registered_function_names; 

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
    registered_functions["pow"] = POW;
    registered_functions["round"] = ROUND;
    registered_functions["trunc"] = TRUNC;
    registered_functions["ceil"] = CEIL;
    registered_functions["floor"] = FLOOR;
    registered_functions["hypot"] = HYPOT;
    registered_functions["cath"] = CATH;

    registered_functions["eval"] = EVAL;
    registered_functions["func"] = FUNC;
    registered_functions["getvar"] = GET_VAR;
    registered_functions["hasvar"] = HAS_VAR;
    registered_functions["foreach"] = FOR_EACH;

    // Python functions
    registered_functions["all"] = PY_ALL;
    registered_functions["any"] = PY_EACH;
    registered_functions["bin"] = PY_BIN;
    registered_functions["dict"] = PY_DICT;
    registered_functions["enumerate"] = PY_ENUMERATE;
    registered_functions["filter"] = PY_FILTER;
    registered_functions["float"] = PY_FLOAT;
    registered_functions["format"] = PY_FORMAT;
    registered_functions["frozenset"] = PY_FROZENSET;
    registered_functions["globals"] = PY_GLOBALS;
    registered_functions["getattr"] = PY_GETATTR;
    registered_functions["hasattr"] = PY_HASATTR;
    registered_functions["hash"] = PY_HASH;
    registered_functions["hex"] = PY_HEX;
    registered_functions["id"] = PY_ID;
    registered_functions["int"] = PY_INT;
    registered_functions["len"] = PY_LEN;
    registered_functions["list"] = PY_LIST;
    registered_functions["map"] = PY_MAP;
    registered_functions["oct"] = PY_OCT;
    registered_functions["str"] = PY_STR;
    registered_functions["range"] = PY_RANGE;
    registered_functions["repr"] = PY_REPR;
    registered_functions["reversed"] = PY_REVERSED;
    registered_functions["set"] = PY_SET;
    registered_functions["slice"] = PY_SLICE;
    registered_functions["sorted"] = PY_SORTED;
    registered_functions["tuple"] = PY_TUPLE;
    registered_functions["zip"] = PY_ZIP;

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

TYPESYSTEM_SOURCE(App::FunctionExpression, App::UnitExpression);

FunctionExpression::FunctionExpression(const DocumentObject *_owner, 
        int _f, const std::vector<Expression *> &_args)
    : UnitExpression(_owner)
    , f(_f)
    , args(_args)
{
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

                if (!p)
                    continue;

                if ((qp = freecad_dynamic_cast<PropertyQuantity>(p)) != 0)
                    c->collect(qp->getQuantityValue());
                else if ((fp = freecad_dynamic_cast<PropertyFloat>(p)) != 0)
                    c->collect(Quantity(fp->getValue()));
                else
                    EXPR_THROW("Invalid property type for aggregate");
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

Expression * FunctionExpression::_eval() const
{
    if(args.empty())
        EXPR_THROW("Function requires at least one argument");

    // Handle aggregate functions
    if (f > AGGREGATES)
        return evalAggregate();

    switch(f) {
    case GET_VAR:
    case HAS_VAR:
    case FUNC:
        return 0;
    default:
        break;
    }

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
        EXPR_THROW("Invalid argument.");

    double value = v1->getValue();

    /* Check units and arguments */
    switch (f) {
    case COS:
    case SIN:
    case TAN:
        if (!(v1->getUnit() == Unit::Angle || v1->getUnit().isEmpty()))
            EXPR_THROW("Unit must be either empty or an angle.");

        // Convert value to radians
        value *= M_PI / 180.0;
        unit = Unit();
        break;
    case ACOS:
    case ASIN:
    case ATAN:
        if (!v1->getUnit().isEmpty())
            EXPR_THROW("Unit must be empty.");
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
            EXPR_THROW("Unit must be empty.");
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
            EXPR_THROW("All dimensions must be even to compute the square root.");

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
            EXPR_THROW("Invalid second argument.");

        if (v1->getUnit() != v2->getUnit())
            EXPR_THROW("Units must be equal");
        unit = Unit::Angle;
        scaler = 180.0 / M_PI;
        break;
    case FMOD:
        if (v2 == 0)
            EXPR_THROW("Invalid second argument.");
        unit = v1->getUnit() / v2->getUnit();
        break;
    case POW: {
        if (v2 == 0)
            EXPR_THROW("Invalid second argument.");

        if (!v2->getUnit().isEmpty())
            EXPR_THROW("Exponent is not allowed to have a unit.");

        // Compute new unit for exponentiation
        double exponent = v2->getValue();
        if (!v1->getUnit().isEmpty()) {
            if (exponent - boost::math::round(exponent) < 1e-9)
                unit = v1->getUnit().pow(exponent);
            else
                EXPR_THROW("Exponent must be an integer when used with a unit");
        }
        break;
    }
    case HYPOT:
    case CATH:
        if (v2 == 0)
            EXPR_THROW("Invalid second argument.");
        if (v1->getUnit() != v2->getUnit())
            EXPR_THROW("Units must be equal");

        if (args.size() > 2) {
            if (v3 == 0)
                EXPR_THROW("Invalid second argument.");
            if (v2->getUnit() != v3->getUnit())
                EXPR_THROW("Units must be equal");
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
    case FMOD: {
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

boost::any FunctionExpression::_getValueAsAny() const {
    if(args.empty())
        EXPR_THROW("Function requires at least one argument");

    switch(f) {
    case GET_VAR: 
        if(args.size()>2)
            EXPR_THROW("Function expects 2 or 3 arguments");
        // fall through
    case HAS_VAR: {
        Base::PyGILStateLocker lock;
        boost::any value = args[0]->getValueAsAny();
        if(value.type()!=typeid(std::string))
            EXPR_THROW("Expects the first argument evaluating to a string");
        Py::Object pyobj;
        bool found = Base::Interpreter().getVariable(
                boost::any_cast<std::string>(value).c_str(),pyobj);
        if(f == HAS_VAR)
            return boost::any(found);
        if(!found) {
            if(args.size()==2)
                return args[1]->getValueAsAny();
            EXPR_THROW("Variable not found");
        }
        return ObjectIdentifier::pyObjectToAny(pyobj);
    } case FUNC: {
        std::unique_ptr<Expression> e(args[0]->eval());
        if(!e->isDerivedFrom(StringExpression::getClassTypeId()))
            EXPR_THROW("Expects the first argument evaluating to a string");
        return boost::any(Py::Object(new ExpressionPy(e.release())));
    } default:
        break;
    } 

    std::unique_ptr<Expression> expr(_eval());
    assert(expr);
    return expr->getValueAsAny();
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
        for (auto it = a.begin(); it != a.end(); ++it)
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

void FunctionExpression::_toString(std::ostringstream &ss, bool persistent) const
{
    init_functions();
    auto it = registered_function_names.find(f);
    assert(it!=registered_function_names.end());
    ss << it->second << '(';
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

int FunctionExpression::priority() const
{
    return 20;
}

void FunctionExpression::visit(ExpressionVisitor &v)
{
    std::vector<Expression*>::const_iterator i = args.begin();

    while (i != args.end()) {
        (*i)->visit(v);
        ++i;
    }
    v.visit(this);
}

//
// VariableExpression class
//

TYPESYSTEM_SOURCE(App::VariableExpression, App::Expression);

VariableExpression::VariableExpression(const DocumentObject *_owner, ObjectIdentifier _var)
    : Expression(_owner)
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

boost::any VariableExpression::_getValueAsAny() const {
    return var.getValue(true);
}

bool VariableExpression::_isIndexable() const {
    return true;
}

void VariableExpression::_toString(std::ostringstream &ss, bool persistent) const {
    if(persistent)
        ss << var.toPersistentString();
    else
        ss << var.toString();
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

void VariableExpression::setPath(const ObjectIdentifier &path)
{
     var = path;
}

bool VariableExpression::_renameDocument(const std::string &oldName,
        const std::string &newName, ExpressionVisitor &v)
{
    return var.renameDocument(oldName, newName,&v);
}

bool VariableExpression::_adjustLinks(const std::set<App::DocumentObject *> &inList, ExpressionVisitor &v) 
{
    return var.adjustLinks(inList,&v);
}

void VariableExpression::_importSubNames(const std::map<std::string,std::string> &subNameMap) 
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
    return var.updateElementReference(feature,reverse,&v);
}

//
// CallableExpression class
//

TYPESYSTEM_SOURCE(App::CallableExpression, App::Expression);

CallableExpression::CallableExpression(const DocumentObject *_owner, Expression *expr, 
        const std::vector<std::pair<std::string,Expression*> > &_named)
    : Expression(_owner), expr(expr),args(_named)
{
}

CallableExpression::~CallableExpression()
{
    for(auto &v : args)
        delete v.second;
    if(expr)
        delete expr;
}

Expression *CallableExpression::create(const DocumentObject *owner, 
        const ObjectIdentifier &path, 
        const std::vector<std::pair<std::string,Expression*> > &args)
{
    if(path.numComponents()!=1 || path.hasDocumentObjectName()) 
        return new CallableExpression(owner, new VariableExpression(owner,path),args);

    const std::string &name = path.getPropertyComponent(0).getName();
    auto it = registered_functions.find(name);
    if(it == registered_functions.end()) {
        std::string msg("Unknown function ");
        msg += name;
        ExpressionParser::ExpressionParser_yyerror(msg.c_str());
        return 0;
    }
    if(it->second == FOR_EACH || it->second == EVAL || 
       (it->second > PY_START && it->second < PY_END))
    {
        auto expr = new CallableExpression(owner,0,args);
        expr->name = it->first;
        expr->ftype = it->second;
        return expr;
    }

    std::vector<Expression*> a;
    for(auto &v : args) {
        if(v.first.size()) {
            std::ostringstream ss;
            ss << "Function '" << name << "' does not support named argument";
            ExpressionParser::ExpressionParser_yyerror(ss.str().c_str());
            return 0;
        }
        a.push_back(v.second);
    }
    return new FunctionExpression(owner,it->second,a);
}

void CallableExpression::_toString(std::ostringstream &ss, bool persistent) const {
    ss << (expr?expr->toString(persistent):name) << '(';

    bool first = true;
    for(auto &v : args) {
        if(first)
            first = false;
        else
            ss << ", ";
        if(v.first.size())
            ss << v.first << '=';
        ss << v.second->toString(persistent);
    }
    ss << ')';
}

bool CallableExpression::isTouched() const
{
    if(expr && expr->isTouched())
        return true;
    for(auto &v : args) {
        if(v.second->isTouched())
            return true;
    }
    return false;
}

void CallableExpression::visit(ExpressionVisitor &v) {
    for(auto &p : args)
        p.second->visit(v);
    if(expr)
        expr->visit(v);
    v.visit(this);
}

Py::Object CallableExpression::evaluate(const DocumentObject *owner, 
        const std::string &expr, PyObject *args, PyObject *kwds) {
    Base::PyGILStateLocker lock;
    std::unique_ptr<CallableExpression> callable(new CallableExpression(owner));
    callable->ftype = EVAL;
    callable->args.emplace_back("",new StringExpression(owner,expr));
    if(args) {
        Py::Sequence seq(args);
        for(size_t i=0;i<seq.size();++i)
            callable->args.emplace_back("",new PyObjectExpression(owner,Py::Object(seq[i])));
    }
    if(kwds) {
        Py::Dict dict(kwds);
        for(auto it=dict.begin();it!=dict.end();++it){
            const auto &value = (*it);
            callable->args.emplace_back(value.first.as_string(),
                    new PyObjectExpression(owner,Py::Object(value.second))); 
        }
    }
    return pyObjectFromAny(callable->getValueAsAny(),callable.get());
}

static void argumentSetup(PythonVariables &vars, std::string &cmd, 
        int &idx, const std::string &name, Expression *arg) 
{
    if(name == "*") {
        Py::Object pyobj(pyObjectFromAny(arg->getValueAsAny(),arg));
        if(!pyobj.isSequence())
            _EXPR_THROW("Expects a Python sequence from", arg);
        Py::Sequence seq(pyobj);
        for(size_t i=0;i<seq.size();++i) {
            std::unique_ptr<Expression> expr(new PyObjectExpression(
                        arg->getOwner(), Py::Object(seq[i])));
            argumentSetup(vars,cmd,idx,"",expr.get());
        }
        return;
    }
    if(name == "**") {
        Py::Object pyobj(pyObjectFromAny(arg->getValueAsAny(),arg));
        if(!pyobj.isMapping())
            _EXPR_THROW("Expects a Python mapping from", arg);
        Py::Mapping mapping(pyobj);
        for(auto it=mapping.begin();it!=mapping.end();++it) {
            const auto &value = *it;
            if(!value.first.isString())
                _EXPR_THROW("Only accepts string as key", arg);
            std::unique_ptr<Expression> expr(new PyObjectExpression(
                        arg->getOwner(), Py::Object(value.second)));
            argumentSetup(vars,cmd,idx,value.first.as_string(),expr.get());
        }
    }

    std::ostringstream ss;
    ss << '$';
    if(name.empty())
        ss << idx++;
    else
        ss << name;
    ss << '$';
    std::string key = ss.str();

    if(cmd.find(key) == std::string::npos)
        return;
    std::unique_ptr<Expression> expr(arg->eval());
    auto pyexpr = dynamic_cast<PyObjectExpression*>(expr.get());
    ss.str("");
    if(!pyexpr) 
        ss << '(' << expr->toString() << ')';
    else 
        ss << "getvar(<<" << vars.add(pyexpr->getPyObject()) << ">>)";
    boost::replace_all(cmd,key,ss.str());
}

Expression *CallableExpression::_eval() const {
    if(expr || ftype != EVAL)
        return 0;
    PythonVariables vars;
    std::vector<boost::any> cmds;
    boost::any value(args[0].second->getValueAsAny());
    if(value.type() == typeid(std::string))
        cmds.push_back(value);
    else if(value.type()!=typeid(Py::Object) || !boost::any_cast<Py::Object>(value).isSequence())
        EXPR_THROW("Expects the first argument to be a string or sequance of strings");
    else {
        Py::Sequence seq(boost::any_cast<Py::Object>(value));
        if(!seq.size())
            return new PyObjectExpression(owner);
        cmds.reserve(seq.size());
        for(size_t i=0;i<seq.size();++i)
            cmds.push_back(ObjectIdentifier::pyObjectToAny(Py::Object(seq[i])));
    }
    std::vector<std::pair<std::string,Py::Object> > results;
    results.reserve(cmds.size()-1);
    for(size_t i=0;i<cmds.size();++i) {
        if(cmds[i].type()!=typeid(std::string)) 
            __EXPR_THROW("None string expression[", i << ']', this);
        std::string cmd(boost::any_cast<std::string>(cmds[i]));
        int idx=1;
        for(size_t j=1;j<args.size();++j)
            argumentSetup(vars,cmd,idx,args[j].first,args[j].second);
        idx = -(int)results.size();
        for(auto &v : results) {
            std::unique_ptr<PyObjectExpression> expr(
                    new PyObjectExpression(owner,v.second));
            argumentSetup(vars,cmd,idx,std::to_string(idx++),expr.get());
        }
        std::unique_ptr<Expression> result(parse(owner,cmd.c_str()));
        if(i == cmds.size()-1) 
            return result->eval();
        Py::Object res(pyObjectFromAny(result->getValueAsAny()));
        results.emplace_back(vars.add(res),res);
    }
    return new PyObjectExpression(owner);
}

boost::any CallableExpression::_getValueAsAny() const {
    Base::PyGILStateLocker lock;
    if(!expr) {
        switch(ftype) {
        case EVAL: {
            std::unique_ptr<Expression> e(_eval());
            return e->getValueAsAny();
        } case FOR_EACH: {
            if(args.size()<2)
                EXPR_THROW("Function expects at least two arguments");
            Py::Object pyobj(pyObjectFromAny(args[0].second->getValueAsAny(),args[0].second));
            if(!pyobj.isSequence())
                EXPR_THROW("Function expecpts the first argument to be a Python sequence");
            boost::any value(args[1].second->getValueAsAny());
            if(value.type()!=typeid(std::string))
                EXPR_THROW("Function expecpts the second argument to be a string");
            Py::List list;
            Py::Sequence seq(pyobj);
            if(!seq.size())
                return boost::any(Py::Object(list));

            std::unique_ptr<CallableExpression> callable(new CallableExpression(owner));
            callable->ftype = EVAL;
            callable->args.reserve(args.size()+1);
            callable->args.emplace_back("",new StringExpression(owner,boost::any_cast<std::string>(value)));
            auto arg1 = new NumberExpression(owner);
            callable->args.emplace_back("",arg1);
            auto arg2 = new PyObjectExpression(owner);
            callable->args.emplace_back("",arg2);
            for(size_t i=2;i<args.size();++i)
                callable->args.emplace_back(args[i].first, args[i].second->eval());
            for(size_t i=0;i<seq.size();++i) {
                arg1->setUnit(Quantity((double)i));
                arg2->setPyObject(Py::Object(seq[i].ptr()));
                list.append(pyObjectFromAny(callable->getValueAsAny(),callable.get()));
            }
            return boost::any(Py::Object(list));
        } default: {
            assert(name.size());
            PythonVariables vars;
            std::string res = vars.add(Py::Object());
            std::ostringstream ss;
            ss << res << '=' << name << '(';
            bool first = true;
            for(auto &v : args) {
                if(first)
                    first = false;
                else
                    ss << ',';
                if(v.first.size()) {
                    ss << v.first;
                    if(v.first[0] != '*')
                        ss << '=';
                }
                ss << vars.add(pyObjectFromAny(v.second->getValueAsAny(),v.second));
            }
            ss << ')';
            return getValueByPython(res.c_str(), ss.str().c_str(), this);
        }}
    }
    boost::any value = expr->getValueAsAny();
    Py::Object pyobj;
    if(value.type()==typeid(Py::Object))
        pyobj = boost::any_cast<Py::Object>(value);
    if(!pyobj.isCallable())
        EXPR_THROW("Expects Python callable");

    int count=0;
    std::vector<Py::Sequence> seqs;
    for(auto &v : args) {
        if(v.first.empty())
            ++count;
        else if(v.first == "*") {
            Py::Object pyobj = pyObjectFromAny(v.second->getValueAsAny(),v.second);
            if(!pyobj.isSequence()) 
                EXPR_THROW("Expects Python sequence");
            seqs.push_back(pyobj);
            count += (int)seqs.back().size();
        }
    }
    Py::Tuple tuple(count);
    Py::Dict dict;
    int i=0;
    auto it = seqs.begin();
    for(auto &v : args) {
        if(v.first.empty())
            tuple.setItem(i++,pyObjectFromAny(v.second->getValueAsAny(),v.second));
        else if(v.first == "*") {
            Py::Sequence seq = *it++;
            for(size_t j=0;j<seq.size();++j)
                tuple.setItem(i++,Py::Object(seq[j].ptr()));
        }else if(v.first == "**") {
            Py::Object pyobj(pyObjectFromAny(v.second->getValueAsAny(),v.second));
            if(!pyobj.isMapping()) 
                EXPR_THROW("Expects Python mapping");
            Py::Mapping map(pyobj);
            for(auto it=map.begin();it!=map.end();++it) {
                const auto &value = *it;
                if(!value.first.isString())
                    EXPR_THROW("Expects only string key");
                dict.setItem(value.first, value.second);
            }
        }else
            dict.setItem(v.first, pyObjectFromAny(v.second->getValueAsAny(),v.second));
    }
    try {
        return ObjectIdentifier::pyObjectToAny(Py::Callable(pyobj).apply(tuple,dict));
    }catch (Py::Exception&) {
        Base::PyException::ThrowException();
        return boost::any();
    }
}

Expression *CallableExpression::_copy() const {
    auto ret = new CallableExpression(owner, expr?expr->copy():nullptr,args);
    ret->name = name;
    ret->ftype = ftype;
    for(auto &v : ret->args)
        v.second = v.second->copy();
    return ret;
}

//
// PyObjectExpression class
//

TYPESYSTEM_SOURCE(App::PyObjectExpression, App::Expression);

PyObjectExpression::PyObjectExpression(const DocumentObject *_owner, Py::Object obj)
    : Expression(_owner)
    , pyObj(obj)
{
}

void PyObjectExpression::_toString(std::ostringstream &ss, bool) const
{
    Base::PyGILStateLocker lock;
    ss << pyObj.as_string();
}

int PyObjectExpression::priority() const
{
    return 20;
}

Expression *PyObjectExpression::_copy() const
{
    return new PyObjectExpression(owner, pyObj);
}

boost::any PyObjectExpression::_getValueAsAny() const {
    return ObjectIdentifier::pyObjectToAny(pyObj);
}

//
// StringExpression class
//

TYPESYSTEM_SOURCE(App::StringExpression, App::Expression);

StringExpression::StringExpression(const DocumentObject *_owner, const std::string &_text, bool _r_literal)
    : Expression(_owner) , text(_text), r_literal(_r_literal)
{
}

void StringExpression::_toString(std::ostringstream &ss, bool) const
{
    if(r_literal)
        ss << "r<<" << text << ">>";
    else
        ss << quote(text);
}

int StringExpression::priority() const
{
    return 20;
}

Expression *StringExpression::_copy() const
{
    return new StringExpression(owner, text, r_literal);
}

boost::any StringExpression::_getValueAsAny() const {
    return boost::any(text);
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

static bool evalCondition(Expression *condition) {
    std::unique_ptr<Expression> e(condition->eval());
    NumberExpression * v = freecad_dynamic_cast<NumberExpression>(e.get());
    if (v == 0) 
        return pyObjectFromAny(e->getValueAsAny()).isTrue();
    else
        return fabs(v->getValue()) > 0.5;
}

Expression *ConditionalExpression::_eval() const
{
    if (evalCondition(condition))
        return trueExpr->eval();
    else
        return falseExpr->eval();
}

boost::any ConditionalExpression::_getValueAsAny() const {
    if(evalCondition(condition))
        return trueExpr->getValueAsAny();
    else
        return falseExpr->getValueAsAny();
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

void ConditionalExpression::_toString(std::ostringstream &ss, bool persistent) const
{
    ss << condition->toString(persistent) << " ? ";

    if (trueExpr->priority() <= priority())
        ss << '(' << trueExpr->toString(persistent) << ')';
    else
        ss << trueExpr->toString(persistent);

    if (falseExpr->priority() <= priority())
        ss << " : (" << falseExpr->toString(persistent) << ')';
    else
        ss << " : " << falseExpr->toString(persistent);
}

Expression *ConditionalExpression::_copy() const
{
    return new ConditionalExpression(owner, condition->copy(), trueExpr->copy(), falseExpr->copy());
}

int ConditionalExpression::priority() const
{
    return 2;
}

void ConditionalExpression::visit(ExpressionVisitor &v)
{
    condition->visit(v);
    trueExpr->visit(v);
    falseExpr->visit(v);
    v.visit(this);
}

TYPESYSTEM_SOURCE(App::ConstantExpression, App::NumberExpression);

ConstantExpression::ConstantExpression(const DocumentObject *_owner, std::string _name, const Quantity & _quantity)
    : NumberExpression(_owner, _quantity)
    , name(_name)
{
}

void ConstantExpression::_toString(std::ostringstream &ss, bool) const
{
    ss << name;
}

Expression *ConstantExpression::_copy() const
{
    return new ConstantExpression(owner, name.c_str(), quantity);
}

int ConstantExpression::priority() const
{
    return 20;
}

Expression *ConstantExpression::simplify() const {
    if(name == "None")
        return eval();
    return copy();
}

Expression *ConstantExpression::_eval() const {
    if(name == "None")
        return new PyObjectExpression(owner);
    return 0;
}

boost::any ConstantExpression::_getValueAsAny() const {
    if(name == "None")
        return boost::any();
    return NumberExpression::_getValueAsAny();
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
    : Expression(_owner)
    , range((begin + ":" + end).c_str())
{
}

bool RangeExpression::isTouched() const
{
    Range i(range);

    do {
        Property * prop = owner->getPropertyByName(i.address().c_str());

        if (prop && prop->isTouched())
            return true;
    } while (i.next());

    return false;
}

void RangeExpression::_toString(std::ostringstream &ss, bool) const
{
    ss << range.rangeString();
}

Expression *RangeExpression::_copy() const
{
    return new RangeExpression(owner, range.fromCellString(), range.toCellString());
}

int RangeExpression::priority() const
{
    return 20;
}

void RangeExpression::_getDeps(ExpressionDeps &deps) const
{
    assert(owner);

    Range i(range);

    auto &dep = deps[owner];
    do {
        std::string address = i.address();
        dep[address].push_back(ObjectIdentifier(owner,address));
    } while (i.next());
}

boost::any RangeExpression::_getValueAsAny() const {
    Py::Tuple value(range.size());
    Range i(range);
    int j=0;
    do {
        try {
            ObjectIdentifier var(owner,i.address());
            value.setItem(j++,pyObjectFromAny(var.getValue()));
        }catch(Base::Exception &e) {
            __EXPR_THROW("Failed to obtian cell ", i.address() << ": " << e.what(), this);
        }
    }while(i.next());
    return boost::any(Py::Object(value));
}

void RangeExpression::setRange(const Range &r)
{
    range = r;
}

// ListExpression class
//

TYPESYSTEM_SOURCE(App::ListExpression, App::Expression);

ListExpression::ListExpression(const DocumentObject *_owner, const std::vector<Expression *> &_items)
    : Expression(_owner), items(_items)
{
}

ListExpression::~ListExpression() {
    for(auto item : items)
        delete item;
}

void ListExpression::addItem(Expression *item) {
    assert(item);
    items.push_back(item);
}

void ListExpression::_toString(std::ostringstream &ss, bool persistent) const {
    ss << '[';
    for(auto item : items) 
        ss << item->toString(persistent) << ", ";
    ss << ']';
}

int ListExpression::priority() const
{
    return 20;
}

Expression *ListExpression::_copy() const
{
    auto ret = new ListExpression(owner,items);
    for(auto &item : ret->items)
        item = item->copy();
    return ret;
}

boost::any ListExpression::_getValueAsAny() const {
    Py::List list;
    for(auto item : items)
        list.append(pyObjectFromAny(item->getValueAsAny(),item));
    return boost::any(Py::Object(list));
}

// TupleExpression class
//

TYPESYSTEM_SOURCE(App::TupleExpression, App::ListExpression);

TupleExpression::TupleExpression(const DocumentObject *_owner, Expression *item)
    : ListExpression(_owner)
{
    if(item)
        addItem(item);
}

TupleExpression::TupleExpression(const DocumentObject *_owner, const std::vector<Expression *> &items)
    : ListExpression(_owner,items)
{
}


void TupleExpression::_toString(std::ostringstream &ss, bool persistent) const {
    ss << '(';
    if(items.empty())
        ss << ", ";
    else {
        for(auto item : items) 
            ss << item->toString(persistent) << ", ";
    }
    ss << ')';
}

Expression *TupleExpression::_copy() const
{
    auto ret = new TupleExpression(owner,items);
    for(auto &item : ret->items)
        item = item->copy();
    return ret;
}

boost::any TupleExpression::_getValueAsAny() const {
    Py::Tuple tuple(items.size());
    int i=0;
    for(auto item : items)
        tuple.setItem(i++,pyObjectFromAny(item->getValueAsAny(),item));
    return boost::any(Py::Object(tuple));
}


// DictExpression class
//

TYPESYSTEM_SOURCE(App::DictExpression, App::Expression);

DictExpression::DictExpression(const DocumentObject *_owner, Expression *key, Expression *value)
    : Expression(_owner)
{
    if(key)
        addItem(key,value);
}

DictExpression::~DictExpression() {
    for(auto &v : items) {
        delete v.first;
        delete v.second;
    }
}

void DictExpression::addItem(Expression *key, Expression *value) {
    assert(key && value);
    items.emplace_back(key,value);
}

void DictExpression::_toString(std::ostringstream &ss, bool persistent) const {
    ss << '{';
    for(auto &v : items) 
        ss << v.first->toString(persistent) << ':' << v.second->toString(persistent) << ", ";
    ss << '}';
}

int DictExpression::priority() const
{
    return 20;
}

Expression *DictExpression::_copy() const
{
    auto ret = new DictExpression(owner);
    ret->items.reserve(items.size());
    for(auto &v : items)
        ret->items.emplace_back(v.first->copy(),v.second->copy());
    return ret;
}

boost::any DictExpression::_getValueAsAny() const {
    Py::Dict dict;
    for(auto &v : items)
        dict.setItem(pyObjectFromAny(v.first->getValueAsAny(),v.first),
                pyObjectFromAny(v.second->getValueAsAny(),v.second));
    return boost::any(Py::Object(dict));
}


namespace App {

namespace ExpressionParser {

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

static Base::XMLReader *_Reader = 0;
static Expression * ScanResult = 0;                    /**< The resulting expression after a successful parsing */
static const App::DocumentObject * DocumentObject = 0; /**< The DocumentObject that will own the expression */
static bool unitExpression = false;                    /**< True if the parsed string is a unit only */
static bool valueExpression = false;                   /**< True if the parsed string is a full expression */
static std::stack<std::string> labels;                /**< Label string primitive */
static int last_column;
static int column;

} //namespace ExpressionParser

ExpressionImporter::ExpressionImporter(Base::XMLReader &reader) {
    assert(!ExpressionParser::_Reader);
    ExpressionParser::_Reader = &reader;
}

ExpressionImporter::~ExpressionImporter() {
    assert(ExpressionParser::_Reader);
    ExpressionParser::_Reader = 0;
}

namespace ExpressionParser {

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
    using namespace App::ExpressionParser;

    ScanResult = 0;
    App::ExpressionParser::DocumentObject = owner;
    labels = std::stack<std::string>();
    column = 0;
    unitExpression = valueExpression = false;
    parser_msg.clear();

    init_functions();
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

    if (result != 0) {
        if(parser_msg.size())
            throw ParserError(parser_msg.c_str());
        throw ParserError("Failed to parse expression.");
    }

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
            const double epsilon = std::numeric_limits<double>::epsilon();

            // If not initially a unit expression, but value is equal to 1, it means the expression is something like 1/unit
            if (denom && nom && essentiallyEqual(nom->getValue(), 1.0, epsilon))
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
