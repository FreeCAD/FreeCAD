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
#include "Utils.h"
#include <boost/math/special_functions/round.hpp>

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
#endif

using namespace Base;
using namespace App;
using namespace Spreadsheet;

Path::Path(const App::DocumentObject * _owner, const std::string & property)
    : owner(_owner)
    , propertyIndex(-1)
    , documentNameSet(false)
    , documentObjectNameSet(false)
{
    if (property.size() > 0)
        addComponent(Component::SimpleComponent(property));
}

bool Path::operator ==(const Path &other) const
{
    if (owner != other.owner)
        return false;
    if (documentName != other.documentName)
        return false;
    if (documentObjectName != other.documentObjectName)
        return false;
    if (components != other.components)
        return false;
    return true;
}

bool Path::operator <(const Path &other) const
{
    if (documentName < other.documentName)
        return true;

    if (documentName > other.documentName)
        return false;

    if (documentObjectName < other.documentObjectName)
        return true;

    if (documentObjectName > other.documentObjectName)
        return false;

    if (components.size() < other.components.size())
        return true;

    if (components.size() > other.components.size())
        return false;

    for (int i = 0; i < components.size(); ++i) {
        if (components[i].component < other.components[i].component)
            return true;
        if (components[i].component > other.components[i].component)
            return false;
        if (components[i].type < other.components[i].type)
            return true;
        if (components[i].type > other.components[i].type)
            return false;
        if (components[i].type == Component::ARRAY) {
            if (components[i].index < other.components[i].index)
                return true;
            if (components[i].index > other.components[i].index)
                return false;
        }
        else if (components[i].type == Component::MAP) {
            if (components[i].key < other.components[i].key)
                return true;
            if (components[i].key > other.components[i].key)
                return false;
        }
    }
    return false;
}

int Path::numComponents() const
{
    return components.size();
}

Path Path::parse(const DocumentObject * docObj, const char *expr)
{
    return Path();
}

std::string Path::toString() const
{
    std::stringstream s;

    if (documentNameSet) {
        if (getDocumentName().isRealString())
            s << quote(getDocumentName().getString()) << "#";
        else
            s << getDocumentName().getString() << "#";
    }

    if (documentObjectNameSet) {
        if (getDocumentObjectName().isRealString())
            s << quote(getDocumentObjectName().getString()) << ".";
        else
            s << getDocumentObjectName().getString() << ".";
    }
    else if (propertyIndex > 0)
        s << components[0].component << ".";

    s << getPropertyName() << getSubPathStr();

    return s.str();
}

std::string Path::getPythonAccessor() const
{
    const Property * prop = getProperty();

    if (!prop)
        throw Exception("Property not found");

    const DocumentObject * docObj = freecad_dynamic_cast<DocumentObject>(prop->getContainer());

    if (!docObj)
        throw Exception("Document object not found");

    const Document * doc = docObj->getDocument();

    return "App.getDocument('" +
            std::string(doc->getName()) + "')." +
            docObj->getNameInDocument() + "." +
            getPropertyName() +
            getSubPathStr();
}

void Path::renameDocumentObject(const std::string &oldName, const std::string &newName)
{
    if (documentObjectNameSet && documentObjectName == oldName) {
        documentObjectName = newName;
        resolve();
    }
    else if (propertyIndex == 1 && documentObjectName == oldName) {
        components[0].component = newName;
        resolve();
    }
}

void Path::renameDocument(const std::string &oldName, const std::string &newName)
{
    if (documentNameSet && documentName == oldName) {
        documentName = newName;
        resolve();
    }
}

std::string Path::getSubPathStr() const
{
    std::stringstream s;

    std::vector<Component>::const_iterator i = components.begin() + propertyIndex + 1;
    while (i != components.end()) {
        s << "." << i->toString();
        ++i;
    }

    return s.str();
}

Path::Component::Component(const std::string &_component, Path::Component::typeEnum _type, int _index, String _key)
    : component(_component)
    , type(_type)
    , index(_index)
    , key(_key)
{
}

Path::Component Path::Component::SimpleComponent(const std::string &_component)
{
    return Component(_component);
}

Path::Component Path::Component::ArrayComponent(const std::string &_component, int _index)
{
    return Component(_component, ARRAY, _index);
}

Path::Component Path::Component::MapComponent(const std::string &_component, const String & _key)
{
    return Component(_component, MAP, -1, _key);
}

bool Path::Component::operator ==(const Path::Component &other) const
{
    if (type != other.type)
        return false;

    if (component != other.component)
        return false;

    switch (type) {
    case SIMPLE:
        return true;
    case ARRAY:
        return index == other.index;
    case MAP:
        return key == other.key;
    default:
        assert(0);
        return false;
    }
}

std::string Path::Component::toString() const
{
    std::stringstream s;

    s << component;
    switch (type) {
    case Component::SIMPLE:
        break;
    case Component::MAP:
        s << "[" << key.toString() << "]";
        break;
    case Component::ARRAY:
        s << "[" << index << "]";
        break;
    default:
        assert(0);
    }

    return s.str();
}

//
// Expression base-class
//

TYPESYSTEM_SOURCE_ABSTRACT(Spreadsheet::Expression, Base::BaseClass);

Expression::Expression(const DocumentObject *_owner)
    : owner(_owner)
{

}

Expression::~Expression()
{
}

Expression * Expression::parse(const DocumentObject *owner, const std::string &buffer)
{
    return ExpressionParser::parse(owner, buffer.c_str());
}

//
// UnitExpression class
//

TYPESYSTEM_SOURCE(Spreadsheet::UnitExpression, Spreadsheet::Expression);

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

std::string UnitExpression::toString() const
{
    return unitStr;
}

/**
  * Return a copy of the expression.
  */

Expression *UnitExpression::copy() const
{
    return new UnitExpression(owner, quantity);
}

//
// NumberExpression class
//

TYPESYSTEM_SOURCE(Spreadsheet::NumberExpression, Spreadsheet::Expression);

NumberExpression::NumberExpression(const DocumentObject *_owner, const Quantity &_quantity)
    : UnitExpression(_owner, _quantity)
{
}

/**
  * Evalute the expression. For NumberExpressions, it is a simply copy().
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

Expression *NumberExpression::copy() const
{
    return new NumberExpression(owner, quantity);
}

/**
  * Negate the stored value.
  */

void NumberExpression::negate()
{
    quantity.setValue(-quantity.getValue());
}

std::string NumberExpression::toString() const
{
    std::stringstream s;
    s << quantity.getValue();

    /* Trim of any extra spaces */
    //while (s.size() > 0 && s[s.size() - 1] == ' ')
//        s.erase(s.size() - 1);

   return s.str();
}

//
// OperatorExpression class
//

TYPESYSTEM_SOURCE(Spreadsheet::OperatorExpression, Spreadsheet::Expression);

OperatorExpression::OperatorExpression(const App::DocumentObject *_owner, Expression * _left, Operator _op, Expression * _right)
    : UnitExpression(_owner)
    , left(_left)
    , op(_op)
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
  * Evalutate the expression. Returns a new NumberExpression with the result, or throws
  * an exception if something is wrong, i.e the expression cannot be evaluated.
  */

Expression * OperatorExpression::eval() const
{
    std::auto_ptr<Expression> e1(left->eval());
    NumberExpression * v1;
    std::auto_ptr<Expression> e2(right->eval());
    NumberExpression * v2;
    NumberExpression * output;

    v1 = freecad_dynamic_cast<NumberExpression>(e1.get());
    v2 = freecad_dynamic_cast<NumberExpression>(e2.get());

    if (v1 == 0 || v2 == 0)
        throw Exception("Invalid expression");

    switch (op) {
    case ADD:
        if (v1->getUnit() != v2->getUnit())
            throw Exception("Incompatible units for + operator");
        output = new NumberExpression(owner, v1->getQuantity() + v2->getQuantity());
        break;
    case SUB:
        if (v1->getUnit() != v2->getUnit())
            throw Exception("Incompatible units for - operator");
        output = new NumberExpression(owner, v1->getQuantity()- v2->getQuantity());
        break;
    case MUL:
    case UNIT:
        output = new NumberExpression(owner, v1->getQuantity() * v2->getQuantity());
        break;
    case DIV:
        output = new NumberExpression(owner, v1->getQuantity() / v2->getQuantity());
        break;
    case POW: {
        output = new NumberExpression(owner, v1->getQuantity().pow(v2->getQuantity()) );
        break;
    }
    case EQ:
        if (v1->getUnit() != v2->getUnit())
            throw Exception("Incompatible units for + operator");
        output = new NumberExpression(owner, Quantity(fabs(v1->getValue() - v2->getValue()) < 1e-7));
        break;
    case NEQ:
        if (v1->getUnit() != v2->getUnit())
            throw Exception("Incompatible units for + operator");
        output = new NumberExpression(owner, Quantity(fabs(v1->getValue() - v2->getValue()) > 1e-7));
        break;
    case LT:
        if (v1->getUnit() != v2->getUnit())
            throw Exception("Incompatible units for + operator");
        output = new NumberExpression(owner, Quantity(v1->getValue() < v2->getValue()));
        break;
    case GT:
        if (v1->getUnit() != v2->getUnit())
            throw Exception("Incompatible units for + operator");
        output = new NumberExpression(owner, Quantity(v1->getValue() > v2->getValue()));
        break;
    case LTE:
        if (v1->getUnit() != v2->getUnit())
            throw Exception("Incompatible units for + operator");
        output = new NumberExpression(owner, Quantity(v1->getValue() - v2->getValue() < 1e-7));
        break;
    case GTE:
        if (v1->getUnit() != v2->getUnit())
            throw Exception("Incompatible units for + operator");
        output = new NumberExpression(owner, Quantity(v2->getValue() - v1->getValue()) < 1e-7);
        break;
    default:
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

std::string OperatorExpression::toString() const
{
    std::stringstream s;

    if (left->priority() < priority())
        s << "(" << left->toString() << ")";
    else
        s << left->toString();

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

    if (right->priority() < priority())
        s << "(" << right->toString() << ")";
    else
        s << right->toString();

    return s.str();
}

/**
  * A deep copy of the expression.
  */

Expression *OperatorExpression::copy() const
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
    case ADD:
        return 5;
    case SUB:
        return 5;
    case MUL:
    case UNIT:
        return 10;
    case DIV:
        return 10;
    case POW:
        return 10;
    default:
        return 0;
    }
}

/**
  * Compute the expressions dependencies, i.e the properties it relies on.
  *
  * @param props A set of strings. Each string contains the name of a property that this expression depends on.
  */

void OperatorExpression::getDeps(std::set<Path> &props) const
{
    left->getDeps(props);
    right->getDeps(props);
}

void OperatorExpression::visit(ExpressionVisitor &v)
{
    if (left)
        left->visit(v);
    if (right)
        right->visit(v);
    v.visit(this);
}

//
// FunctionExpression class. This class handles functions with one or two parameters.
//

TYPESYSTEM_SOURCE(Spreadsheet::FunctionExpression, Spreadsheet::UnitExpression);

FunctionExpression::FunctionExpression(const DocumentObject *_owner, Function _f, std::vector<Expression *> _args)
    : UnitExpression(_owner)
    , f(_f)
    , args(_args)
{
    switch (f) {
    case NONE:
        throw Exception("Unknown function");
    case MOD:
    case ATAN2:
    case POW:
        if (args.size() != 2)
            throw Exception("Invalid number of arguments.");
        break;
    default:
        if (args.size() != 1)
            throw Exception("Invalid number of arguments.");
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

/**
  * Evaluate function. Returns a NumberExpression if evaluation is successfuly.
  * Throws an exception if something fails.
  *
  * @returns A NumberExpression with the result.
  */

Expression * FunctionExpression::eval() const
{
    switch (f) {
    case SUM:
    case AVERAGE:
    case STDDEV:
    case COUNT:
    case MIN:
    case MAX:
    {
        RangeExpression * v = freecad_dynamic_cast<RangeExpression>(args[0]);
        Quantity q;
        Quantity mean;
        Quantity M2;

        int n = 0;
        bool first = true;

        if (!v)
            throw Exception("Expected range as argument");

        Range range(v->getRange());

        do {
            Property * p = owner->getPropertyByName(range.address().c_str());
            PropertyQuantity * qp;
            PropertyFloat * fp;
            Quantity value;

            if (!p)
                continue;

            if (qp = freecad_dynamic_cast<PropertyQuantity>(p))
                value = qp->getQuantityValue();
            else if (fp = freecad_dynamic_cast<PropertyFloat>(p))
                value = fp->getValue();
            else
                throw Exception("Invalid property type for aggregate");

            if (first) {
                q.setUnit(value.getUnit());
                mean.setUnit(value.getUnit());
                M2.setUnit(value.getUnit());
            }

            switch (f) {
            case AVERAGE:
                n++;
            case SUM:
                q = q + value;
                break;
            case STDDEV: {
                n++;

                const Quantity delta = value - mean;
                mean = mean + delta / n;
                M2 = M2 + delta * (value - mean);
                break;
            }
            case COUNT:
                q = q + 1;
                break;
            case MIN:
                if (first || value < q)
                    q = value;
                break;
            case MAX:
                if (first || value > q)
                    q = value;
                break;
            }

            first = false;
        } while (range.next());

        switch (f) {
        case AVERAGE:
            q = q / (double)n;
            break;
        case STDDEV:
            if (n < 2)
                q = Quantity();
            else
                q = (M2 / (n - 1.0)).pow(Quantity(0.5));
            break;
        }

        return new NumberExpression(owner, q);
    }
    default:
        break;
    }


    std::auto_ptr<Expression> e1(args[0]->eval());
    std::auto_ptr<Expression> e2(args.size() > 1 ? args[1]->eval() : 0);
    NumberExpression * v1 = freecad_dynamic_cast<NumberExpression>(e1.get());
    NumberExpression * v2 = freecad_dynamic_cast<NumberExpression>(e2.get());
    double output;
    Unit unit;
    double scaler = 1;

    if (v1 == 0)
        throw Exception("Invalid argument.");

    double value = v1->getValue();

    /* Check units and arguments */
    switch (f) {
    case COS:
    case SIN:
    case TAN:
        if (!(v1->getUnit() == Unit::Angle || v1->getUnit().isEmpty()))
            throw Exception("Unit must be either empty or an angle.");

        // Convert value to radians
        value *= M_PI / 180.0;
        unit = Unit();
        break;
    case ACOS:
    case ASIN:
    case ATAN:
        if (!v1->getUnit().isEmpty())
            throw Exception("Unit must be empty.");
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
            throw Exception("Unit must be empty.");
        unit = Unit();
        break;
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
              ((s.LuminoseIntensity % 2) == 0) &&
              ((s.Angle % 2) == 0))
            throw Exception("All dimensions must be even to compute the square root.");

        unit = Unit(s.Length /2,
                    s.Mass / 2,
                    s.Time / 2,
                    s.ElectricCurrent / 2,
                    s.ThermodynamicTemperature / 2,
                    s.AmountOfSubstance / 2,
                    s.LuminoseIntensity / 2,
                    s.Angle);
        break;
    }
    case ATAN2:
        if (v2 == 0)
            throw Exception("Invalid second argument.");

        if (v1->getUnit() != v2->getUnit())
            throw Exception("Units must be equal");
        unit = Unit::Angle;
        scaler = 180.0 / M_PI;
        break;
    case MOD:
        if (v2 == 0)
            throw Exception("Invalid second argument.");
        if (!v2->getUnit().isEmpty())
            throw Exception("Second argument must have empty unit.");
        unit = v1->getUnit();
        break;
    case POW: {
        if (v2 == 0)
            throw Exception("Invalid second argument.");

        if (!v2->getUnit().isEmpty())
            throw Exception("Exponent is not allowed to have a unit.");

        // Compute new unit for exponentation
        double exponent = v2->getValue();
        if (!v1->getUnit().isEmpty()) {
            if (exponent - boost::math::round(exponent) < 1e-9)
                unit = v1->getUnit().pow(exponent);
            else
                throw Exception("Exponent must be an integer when used with a unit");
        }
        break;
    }
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
    default:
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
    Expression * v1 = args[0]->simplify();

    // Argument simplified to numeric expression? Then return evaluate and return
    if (freecad_dynamic_cast<NumberExpression>(v1)) {
        switch (f) {
        case ATAN2:
        case MOD:
        case POW:
            Expression * v2 = args[1]->simplify();

            if (freecad_dynamic_cast<NumberExpression>(v2)) {
                delete v1;
                delete v2;
                return eval();
            }
            else {
                std::vector<Expression*> a;
                a.push_back(v1);
                a.push_back(v2);
                return new FunctionExpression(owner, f, a);
            }
        }
        delete v1;
        return eval();
    }
    else {
        std::vector<Expression*> a;
        a.push_back(v1);
        return new FunctionExpression(owner, f, a);
    }
}

/**
  * Create a string representation of the expression.
  *
  * @returns A string representing the expression.
  */

std::string FunctionExpression::toString() const
{
    switch (f) {
    case ACOS:
        return "acos(" + args[0]->toString() + ")";
    case ASIN:
        return "asin(" + args[0]->toString() + ")";
    case ATAN:
        return "atan(" + args[0]->toString() + ")";
    case ABS:
        return "abs(" + args[0]->toString() + ")";
    case EXP:
        return "exp(" + args[0]->toString() + ")";
    case LOG:
        return "log(" + args[0]->toString() + ")";
    case LOG10:
        return "log10(" + args[0]->toString() + ")";
    case SIN:
        return "sin(" + args[0]->toString() + ")";
    case SINH:
        return "sinh(" + args[0]->toString() + ")";
    case TAN:
        return "tan(" + args[0]->toString() + ")";
    case TANH:
        return "tanh(" + args[0]->toString() + ")";
    case SQRT:
        return "sqrt(" + args[0]->toString() + ")";
    case COS:
        return "cos(" + args[0]->toString() + ")";
    case COSH:
        return "cosh(" + args[0]->toString() + ")";
    case MOD:
        return "mod(" + args[0]->toString() + ", " + args[1]->toString() + ")";
    case ATAN2:
        return "atan2(" + args[0]->toString() + ", " + args[1]->toString() +  ")";
    case POW:
        return "pow(" + args[0]->toString() + ", " + args[1]->toString() +  ")";
    case SUM:
        return "sum(" + args[0]->toString() + ")";
    case COUNT:
        return "count(" + args[0]->toString() + ")";
    case AVERAGE:
        return "average(" + args[0]->toString() + ")";
    case STDDEV:
        return "stddev(" + args[0]->toString() + ")";
    case MIN:
        return "min(" + args[0]->toString() + ")";
    case MAX:
        return "max(" + args[0]->toString() + ")";
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

Expression *FunctionExpression::copy() const
{
    std::vector<Expression*>::const_iterator i = args.begin();
    std::vector<Expression*> a;

    while (i != args.end()) {
        a.push_back((*i)->copy());
        ++i;
    }
    return new FunctionExpression(owner, f, a);
}

/**
  * Compute the dependecy set of the expression. The set contains the names
  * of all Property objects this expression relies on.
  */

void FunctionExpression::getDeps(std::set<Path> &props) const
{
    std::vector<Expression*>::const_iterator i = args.begin();

    while (i != args.end()) {
        (*i)->getDeps(props);
        ++i;
    }
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

TYPESYSTEM_SOURCE(Spreadsheet::VariableExpression, Spreadsheet::UnitExpression);

VariableExpression::VariableExpression(const DocumentObject *_owner, Path _var)
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
    try {
        return getProperty()->isTouched();
    }
    catch (...) {
        return false;
    }
}

const App::DocumentObject * Path::getDocumentObject(const App::Document * doc, const std::string & name) const
{
    DocumentObject * o1 = 0;
    DocumentObject * o2 = 0;
    std::vector<DocumentObject*> docObjects = doc->getObjects();

    for (std::vector<DocumentObject*>::iterator j = docObjects.begin(); j != docObjects.end(); ++j) {
        if (strcmp((*j)->Label.getValue(), name.c_str()) == 0) {
            // Found object with matching label
            if (o1 != 0)
                return 0;
            o1 = *j;
        }
    }

    // No object found with matching label, try using name directly
    o2 = doc->getObject(name.c_str());

    if (o1 == 0 && o2 == 0) // Not found at all
        return 0;
    else if (o1 == 0) // Found by name
        return o2;
    else if (o2 == 0) // Found by label
        return o1;
    else if (o1 == o2) // Found by both name and label, same object
        return o1;
    else
        return 0; // Found by both name and label, two different objects
}

void Path::resetResolve()
{
    if (!documentNameSet)
        documentName = String();
    if (!documentObjectNameSet)
        documentObjectName = String();
}

void Path::resolve()
{
    const App::Document * doc;
    const App::DocumentObject * docObject;

    /* Document name specified? */
    if (getDocumentName().getString().size() > 0)
        doc = getDocument();
    else {
        setDocumentName(String(owner->getDocument()->Label.getValue()));
        doc = owner->getDocument();
    }

    propertyName = "";
    propertyIndex = 0;
    if (doc == 0)
        return;

    /* Document object name specified? */
    if (getDocumentObjectName().getString().size() > 0) {
        docObject = getDocumentObject(doc, getDocumentObjectName().getString());
        if (!docObject)
            return;
        if (components.size() > 0) {
            propertyName = components[0].component;
            propertyIndex = 0;
        }
        else
            return;
    }
    else {
        /* Document object name not specified, resolve from path */
        if (components.size() == 1) {
            setDocumentObjectName(String(owner->getNameInDocument()));
            propertyName = components[0].component;
            propertyIndex = 0;
        }
        else if (components.size() >= 2) {
            if (!components[0].isSimple())
                return;

            docObject = getDocumentObject(doc, components[0].component);

            if (docObject) {
                setDocumentObjectName(components[0].component);
                propertyName = components[1].component;
                propertyIndex = 1;
            }
            else {
                setDocumentObjectName(String(owner->getNameInDocument()));
                propertyName = components[0].component;
                propertyIndex = 0;
            }
        }
        else
            return;
    }
}

Document * Path::getDocument() const
{
    App::Document * doc = 0;
    const std::vector<App::Document*> docs = App::GetApplication().getDocuments();

    for (std::vector<App::Document*>::const_iterator i = docs.begin(); i != docs.end(); ++i) {
        if ((*i)->Label.getValue() == getDocumentName().getString()) {
            if (doc != 0)
                return 0;
            doc = *i;
        }
    }

    return doc;
}

const Property *Path::getProperty() const
{
    const App::Document * doc = getDocument();

    if (!doc)
        return 0;

    const App::DocumentObject * docObj = getDocumentObject(doc, documentObjectName);

    if (!docObj)
        return 0;

    return docObj->getPropertyByName(propertyName.c_str());
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
#ifdef FULL_EXPRESSION_SUPPORT
    const Property * prop = docObject->getPropertyByPath(var);
#else
    const Property * prop = var.getProperty();
#endif
    if (prop)
        return prop;
    else
        throw Base::Exception("Property not found.");
}

/**
  * Evalute the expression. For a VariableExpression, this means to return the
  * value of the referenced Property. Quantities are converted to NumberExpression with unit,
  * int and floats are converted to a NumberExpression without unit. Strings properties
  * are converted to StringExpression objects.
  *
  * @returns The result of the evaluation, i.e a new (Number|String)Expression object.
  */

Expression * VariableExpression::eval() const
{
#ifdef FULL_EXPRESSION_SUPPORT
    const Property * prop = getProperty();
    PropertyContainer * parent = prop->getContainer();

    if (!parent->isDerivedFrom(App::DocumentObject::getClassTypeId()))
        throw Base::Exception("Property must belong to a document object.");

    return static_cast<App::DocumentObject*>(parent)->getValue(var);
#else
    std::string s = "_spreadsheet_temp_ = " + var.getPythonAccessor();
    PyObject * pyvalue = Base::Interpreter().getValue(s.c_str(), "_spreadsheet_temp_");
    Expression * output;

    if (!pyvalue)
        throw Base::Exception("Failed to get property value.");

    if (PyInt_Check(pyvalue))
        output = new NumberExpression(owner, PyInt_AsLong(pyvalue));
    else if (PyFloat_Check(pyvalue))
        output = new NumberExpression(owner, PyFloat_AsDouble(pyvalue));
    else if (PyString_Check(pyvalue))
        output = new StringExpression(owner, PyString_AsString(pyvalue));
    else if (PyUnicode_Check(pyvalue)) {
        PyObject * s = PyUnicode_AsUTF8String(pyvalue);

        output = new StringExpression(owner, PyString_AsString(s));
        Py_DECREF(s);
    }
    else if (PyObject_TypeCheck(pyvalue, &QuantityPy::Type)) {
        Base::QuantityPy * qp = static_cast<QuantityPy*>(pyvalue);
        Base::Quantity * q = qp->getQuantityPtr();

        output = new NumberExpression(owner, *q);
    }
    else {
        Py_DECREF(pyvalue);
        throw Base::Exception("Invalid property type.");
    }

    Py_DECREF(pyvalue);

    return output;
#if 0

    if (prop->isDerivedFrom(PropertyQuantity::getClassTypeId())) {
        const PropertyQuantity * value = static_cast<const PropertyQuantity*>(prop);
        return new NumberExpression(owner, value->getValue(), value->getUnit());
    }
    else if (prop->isDerivedFrom(PropertyFloat::getClassTypeId())) {
        const PropertyFloat * value = static_cast<const PropertyFloat*>(prop);
        return new NumberExpression(owner, value->getValue());
    }
    else if (prop->isDerivedFrom(PropertyInteger::getClassTypeId())) {
        const PropertyInteger * value = static_cast<const PropertyInteger*>(prop);
        return new NumberExpression(owner, value->getValue());
    }
    else if (prop->isDerivedFrom(PropertyString::getClassTypeId())) {
        const PropertyString * value = static_cast<const PropertyString*>(prop);
        return new StringExpression(owner, value->getValue());
    }

    throw Base::Exception("Property is of invalid type (not float).");
#endif
#endif
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

Expression *VariableExpression::copy() const
{
    return new VariableExpression(owner, var);
}

/**
  * Compute the dependecy of the expression. In this case \a props
  * is a set of strings, i.e the names of the Property objects, and
  * the variable name this expression relies on is inserted into the set.
  * Notice that the variable may be unqualified, i.e without any reference
  * to the owning object. This must be taken into consideration when using
  * the set.
  */

void VariableExpression::getDeps(std::set<Path> &props) const
{
    props.insert(var);
}

void VariableExpression::resolve()
{
    var.resetResolve();
    var.resolve();
}

void VariableExpression::renameDocumentObject(const std::string &oldName, const std::string &newName)
{
    var.renameDocumentObject(oldName, newName);
}

void VariableExpression::renameDocument(const std::string &oldName, const std::string &newName)
{
    var.renameDocument(oldName, newName);
}

//
// StringExpression class
//

TYPESYSTEM_SOURCE(Spreadsheet::StringExpression, Spreadsheet::Expression);

StringExpression::StringExpression(const DocumentObject *_owner, const std::string &_text)
    : Expression(_owner)
    , text(_text)
{
}

/**
  * Evalute the string. For strings, this is a simple copy of the object.
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

std::string StringExpression::toString() const
{
    return quote(text);
}

/**
  * Return a copy of the expression.
  */

Expression *StringExpression::copy() const
{
    return new StringExpression(owner, text);
}

TYPESYSTEM_SOURCE(Spreadsheet::ConditionalExpression, Spreadsheet::Expression);

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
    std::auto_ptr<Expression> e(condition->eval());
    NumberExpression * v = freecad_dynamic_cast<NumberExpression>(e.get());

    if (v == 0)
        throw Exception("Invalid expression");

    if (fabs(v->getValue()) > 0.5)
        return trueExpr->eval();
    else
        return falseExpr->eval();
}

Expression *ConditionalExpression::simplify() const
{
    std::auto_ptr<Expression> e(condition->simplify());
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

std::string ConditionalExpression::toString() const
{
    return condition->toString() + " ? " + trueExpr->toString() + " : " + falseExpr->toString();
}

Expression *ConditionalExpression::copy() const
{
    return new ConditionalExpression(owner, condition->copy(), trueExpr->copy(), falseExpr->copy());
}

int ConditionalExpression::priority() const
{
    return 10;
}

void ConditionalExpression::getDeps(std::set<Path> &props) const
{
    condition->getDeps(props);
    trueExpr->getDeps(props);
    falseExpr->getDeps(props);
}

void ConditionalExpression::visit(ExpressionVisitor &v)
{
    condition->visit(v);
    trueExpr->visit(v);
    falseExpr->visit(v);
}

TYPESYSTEM_SOURCE(Spreadsheet::ConstantExpression, Spreadsheet::NumberExpression);

ConstantExpression::ConstantExpression(const DocumentObject *_owner, std::string _name, const Quantity & _quantity)
    : NumberExpression(_owner, _quantity)
    , name(_name)
{
}

std::string ConstantExpression::toString() const
{
    return name;
}

Expression *ConstantExpression::copy() const
{
    return new ConstantExpression(owner, name.c_str(), quantity);
}

namespace Spreadsheet {

namespace ExpressionParser {

/**
 * Error function for parser. Throws a generic Base::Exception with the parser error.
 */

void ExpressionParser_yyerror(char *errorinfo)
{
}

/* helper function for tuning number strings with groups in a locale agnostic way... */
double num_change(char* yytext,char dez_delim,char grp_delim)
{
    double ret_val;
    char temp[40];
    int i = 0;
    for(char* c=yytext;*c!='\0';c++){
        // skipp group delimiter
        if(*c==grp_delim) continue;
        // check for a dez delimiter othere then dot
        if(*c==dez_delim && dez_delim !='.')
             temp[i++] = '.';
        else
            temp[i++] = *c;
        // check buffor overflow
        if (i>39) return 0.0;
    }
    temp[i] = '\0';

    ret_val = atof( temp );
    return ret_val;
}

static Expression * ScanResult = 0;                    /**< The resulting expression after a successful parsing */
static const App::DocumentObject * DocumentObject = 0; /**< The DocumentObject that will own the expression */
static bool unitExpression = false;                    /**< True if the parsed string is a unit only */
static bool valueExpression = false;                   /**< True if the parsed string is a full expression */
static std::stack<std::string> labels;                /**< Label string primitive */
static std::map<std::string, FunctionExpression::Function> registered_functions;                /**< Registerd functions */

// show the parser the lexer method
#define yylex ExpressionParserlex
int ExpressionParserlex(void);

// Parser, defined in ExpressionParser.y
#include "ExpressionParser.tab.c"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
// Scanner, defined in ExpressionParser.l
#include "lex.ExpressionParser.c"
#endif // DOXYGEN_SHOULD_SKIP_THIS
#ifdef _MSC_VER
# define strdup _strdup
#endif

static void initParser(const App::DocumentObject *owner)
{
    static bool has_registered_functions = false;

    using namespace Spreadsheet::ExpressionParser;

    ScanResult = 0;
    Spreadsheet::ExpressionParser::DocumentObject = owner;
    labels = std::stack<std::string>();
    unitExpression = valueExpression = false;

#ifdef FC_DEBUG
    yydebug = 1;
#else
    yydebug = 0;
#endif

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

}

std::string Path::String::toString() const
{
    if (isRealString())
        return quote(str);
    else
        return str;
}

TYPESYSTEM_SOURCE(Spreadsheet::RangeExpression, Spreadsheet::Expression);

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

Expression *RangeExpression::eval() const
{
    throw Exception("Range expression cannot be evaluated");
}

std::string RangeExpression::toString() const
{
    return range.rangeString();
}

Expression *RangeExpression::copy() const
{
    return new RangeExpression(owner, range.fromCellString(), range.toCellString());
}

void RangeExpression::getDeps(std::set<Path> &props) const
{
    Range i(range);

    do {
        props.insert(Path(owner, i.address()));
    } while (i.next());
}

Expression *RangeExpression::simplify() const
{
    return copy();
}

}

/**
  * Parse the expression given by \a buffer, and use \a owner as the owner of the
  * returned expression. If the parser fails for some reason, and exception is thrown.
  *
  * @param owner  The DocumentObject that will own the expression.
  * @param buffer The sting buffer to parse.
  *
  * @returns A pointer to an expression.
  *
  */

Expression * Spreadsheet::ExpressionParser::parse(const App::DocumentObject *owner, const char* buffer)
{
    // parse from buffer
    ExpressionParser::YY_BUFFER_STATE my_string_buffer = ExpressionParser::ExpressionParser_scan_string (buffer);

    initParser(owner);

    // run the parser
    int result = ExpressionParser::ExpressionParser_yyparse ();

    // free the scan buffer
    ExpressionParser::ExpressionParser_delete_buffer (my_string_buffer);

    if (result != 0)
        throw Base::Exception("Failed to parse expression.");

    if (ScanResult == 0)
        throw Base::Exception("Unknown error in expression");

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
        throw Base::Exception("Failed to parse expression.");

    if (ScanResult == 0)
        throw Base::Exception("Unknown error in expression");

    // Simplify expression
    Expression * simplified = ScanResult->simplify();
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
