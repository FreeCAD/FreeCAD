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
#include <App/ObjectIdentifier.h>
#include <boost/math/special_functions/round.hpp>
#include <boost/math/special_functions/trunc.hpp>

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

using namespace Base;
using namespace App;

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

//
// Expression base-class
//

TYPESYSTEM_SOURCE_ABSTRACT(App::Expression, Base::BaseClass);

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

std::string UnitExpression::toString() const
{
    return unitStr;
}

/**
  * Return a copy of the expression.
  */

Expression *UnitExpression::copy() const
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

Expression *NumberExpression::copy() const
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

std::string NumberExpression::toString() const
{
    std::stringstream s;
    s << std::setprecision(std::numeric_limits<double>::digits10 + 1) << quantity.getValue();

    /* Trim of any extra spaces */
    //while (s.size() > 0 && s[s.size() - 1] == ' ')
//        s.erase(s.size() - 1);

   return s.str();
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
    const double epsilon = std::numeric_limits<double>::epsilon();

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
        output = new BooleanExpression(owner, essentiallyEqual(v1->getValue(), v2->getValue(), epsilon) );
        break;
    case NEQ:
        if (v1->getUnit() != v2->getUnit())
            throw ExpressionError("Incompatible units for the != operator");
        output = new BooleanExpression(owner, !essentiallyEqual(v1->getValue(), v2->getValue(), epsilon) );
        break;
    case LT:
        if (v1->getUnit() != v2->getUnit())
            throw ExpressionError("Incompatible units for the < operator");
        output = new BooleanExpression(owner, definitelyLessThan(v1->getValue(), v2->getValue(), epsilon) );
        break;
    case GT:
        if (v1->getUnit() != v2->getUnit())
            throw ExpressionError("Incompatible units for the > operator");
        output = new BooleanExpression(owner, definitelyGreaterThan(v1->getValue(), v2->getValue(), epsilon) );
        break;
    case LTE:
        if (v1->getUnit() != v2->getUnit())
            throw ExpressionError("Incompatible units for the <= operator");
        output = new BooleanExpression(owner, definitelyLessThan(v1->getValue(), v2->getValue(), epsilon) ||
                                       essentiallyEqual(v1->getValue(), v2->getValue(), epsilon));
        break;
    case GTE:
        if (v1->getUnit() != v2->getUnit())
            throw ExpressionError("Incompatible units for the >= operator");
        output = new BooleanExpression(owner, essentiallyEqual(v1->getValue(), v2->getValue(), epsilon) ||
                                       definitelyGreaterThan(v1->getValue(), v2->getValue(), epsilon));
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

std::string OperatorExpression::toString() const
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
        s << "-" << (needsParens ? "(" : "") << left->toString() << (needsParens ? ")" : "");
        return s.str();
    case POS:
        s << "+" << (needsParens ? "(" : "") << left->toString() << (needsParens ? ")" : "");
        return s.str();
    default:
        break;
    }

    if (needsParens)
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

/**
  * Compute the expressions dependencies, i.e the properties it relies on.
  *
  * @param props A set of strings. Each string contains the name of a property that this expression depends on.
  */

void OperatorExpression::getDeps(std::set<ObjectIdentifier> &props) const
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

                if (!p)
                    continue;

                if ((qp = freecad_dynamic_cast<PropertyQuantity>(p)) != 0)
                    c->collect(qp->getQuantityValue());
                else if ((fp = freecad_dynamic_cast<PropertyFloat>(p)) != 0)
                    c->collect(Quantity(fp->getValue()));
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

std::string FunctionExpression::toString() const
{
    std::stringstream ss;

    for (size_t i = 0; i < args.size(); ++i) {
        ss << args[i]->toString();
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

int FunctionExpression::priority() const
{
    return 20;
}

/**
  * Compute the dependency set of the expression. The set contains the names
  * of all Property objects this expression relies on.
  */

void FunctionExpression::getDeps(std::set<ObjectIdentifier> &props) const
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
    try {
        return getProperty()->isTouched();
    }
    catch (...) {
        return false;
    }
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

    throw ExpressionError("Property is of invalid type.");
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

int VariableExpression::priority() const
{
    return 20;
}

/**
  * Compute the dependency of the expression. In this case \a props
  * is a set of strings, i.e the names of the Property objects, and
  * the variable name this expression relies on is inserted into the set.
  * Notice that the variable may be unqualified, i.e without any reference
  * to the owning object. This must be taken into consideration when using
  * the set.
  */

void VariableExpression::getDeps(std::set<ObjectIdentifier> &props) const
{
    props.insert(var);
}

void VariableExpression::setPath(const ObjectIdentifier &path)
{
     var = path;
}

bool VariableExpression::validDocumentObjectRename(const std::string &oldName, const std::string &newName)
{
    return var.validDocumentObjectRename(oldName, newName);
}

bool VariableExpression::renameDocumentObject(const std::string &oldName, const std::string &newName)
{
    return var.renameDocumentObject(oldName, newName);
}

bool VariableExpression::validDocumentRename(const std::string &oldName, const std::string &newName)
{
    return var.validDocumentRename(oldName, newName);
}

bool VariableExpression::renameDocument(const std::string &oldName, const std::string &newName)
{
    return var.renameDocument(oldName, newName);
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

std::string StringExpression::toString() const
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

Expression *StringExpression::copy() const
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

std::string ConditionalExpression::toString() const
{
    std::string cstr = condition->toString();
    std::string tstr = trueExpr->toString();
    std::string fstr = falseExpr->toString();

    if (trueExpr->priority() <= priority())
        tstr = "(" + tstr + ")";

    if (falseExpr->priority() <= priority())
        fstr = "(" + fstr + ")";

    return cstr + " ? " + tstr + " : " + fstr;
}

Expression *ConditionalExpression::copy() const
{
    return new ConditionalExpression(owner, condition->copy(), trueExpr->copy(), falseExpr->copy());
}

int ConditionalExpression::priority() const
{
    return 2;
}

void ConditionalExpression::getDeps(std::set<ObjectIdentifier> &props) const
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

TYPESYSTEM_SOURCE(App::ConstantExpression, App::NumberExpression);

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

int ConstantExpression::priority() const
{
    return 20;
}

TYPESYSTEM_SOURCE_ABSTRACT(App::BooleanExpression, App::NumberExpression);

BooleanExpression::BooleanExpression(const DocumentObject *_owner, bool _value)
    : NumberExpression(_owner, Quantity(_value ? 1.0 : 0.0))
{
}

Expression *BooleanExpression::copy() const
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

int RangeExpression::priority() const
{
    return 20;
}

void RangeExpression::getDeps(std::set<ObjectIdentifier> &props) const
{
    Range i(range);

    do {
        props.insert(ObjectIdentifier(owner, i.address()));
    } while (i.next());
}

Expression *RangeExpression::simplify() const
{
    return copy();
}

void RangeExpression::setRange(const Range &r)
{
    range = r;
}

namespace App {

namespace ExpressionParser {

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
