#include "PreCompiled.h"
#ifdef __GNUC__
# include <unistd.h>
#endif

#include "Base/Exception.h"
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentPy.h>
#include <App/DocumentObject.h>
#include <string>
#include <sstream>
#include <math.h>
#include <stdio.h>
#include "Expression.h"
#include <Base/Unit.h>
#include <App/PropertyUnits.h>

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

using namespace Base;
using namespace App;
using namespace Spreadsheet;

//
// Expression base-class
//

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

UnitExpression::UnitExpression(const DocumentObject *_owner, const Base::Unit & _unit, const char *_unitstr, double _scaler)
    : Expression(_owner)
    , unit(_unit)
    , unitstr(_unitstr ? _unitstr : "")
    , scaler(_scaler)
{
}

/**
  * Set unit information.
  *
  * @param _unit    A unit object
  * @param _unitstr The unit expressed as a string
  * @param _scaler  Scale factor to convert unit into internal unit.
  */

void UnitExpression::setUnit(const Unit &_unit, const char *_unitstr, double _scaler)
{
    unit = _unit;
    unitstr = _unitstr ? _unitstr : "";
    scaler = _scaler;
}

/**
  * Evaulate the expression
  *
  * @returns A NumberExpression set to 1.0.
  */

Expression *UnitExpression::eval() const
{
    return new NumberExpression(owner, 1.0, unit, unitstr.c_str(), scaler);
}

/**
  * Simplify the expression. In this case, a NumberExpression is returned,
  * as it cannot be simplified any more.
  */

Expression *UnitExpression::simplify() const
{
    return new NumberExpression(owner, 1.0, unit, unitstr.c_str(), scaler);
}

/**
  * Return a string representation, in this case the unit string.
  */

std::string UnitExpression::toString() const
{
    return unitstr;
}

/**
  * Return a copy of the expression.
  */

Expression *UnitExpression::copy() const
{
    return new UnitExpression(owner, unit, unitstr.c_str(), scaler);
}

//
// NumberExpression class
//

NumberExpression::NumberExpression(const DocumentObject *_owner, double _value, const Base::Unit & _unit, const char * _unitstr, double _scaler)
    : UnitExpression(_owner, _unit, _unitstr, _scaler)
    , value(_value)
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
  * Return a string representation of the expression.
  */

std::string NumberExpression::toString() const
{
    std::stringstream s;

    s << value;

    return s.str();
}

/**
  * Create and return a copy of the expression.
  */

Expression *NumberExpression::copy() const
{
    return new NumberExpression(owner, value, unit, unitstr.c_str(), scaler);
}

/**
  * Negate the stored value.
  */

void NumberExpression::negate()
{
    value = -value;
}

//
// OperatorExpression class
//

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

    v1 = dynamic_cast<NumberExpression*>(e1.get());
    v2 = dynamic_cast<NumberExpression*>(e2.get());

    if (v1 == 0 || v2 == 0)
        throw Exception("Invalid expression");

    switch (op) {
    case ADD:
        if (v1->getUnit() != v2->getUnit())
            throw Exception("Incompatible units for + operator");
        output = new NumberExpression(owner, v1->getValue() + v2->getValue(), v1->getUnit());
        break;
    case SUB:
        if (v1->getUnit() != v2->getUnit())
            throw Exception("Incompatible units for - operator");
        output = new NumberExpression(owner, v1->getValue()- v2->getValue(), v1->getUnit());
        break;
    case MUL:
        output = new NumberExpression(owner, v1->getValue() * v2->getValue(), v1->getUnit() * v2->getUnit());
        break;
    case DIV:
        output = new NumberExpression(owner, v1->getValue() / v2->getValue(), v1->getUnit() / v2->getUnit());
        break;
    case POW: {
        double exponent = v2->getValue();

        if (!v2->getUnit().isEmpty())
            throw Exception("Exponent is not allowed to have a unit.");

        if (v1->getUnit().isEmpty())
            output = new NumberExpression(owner, pow(v1->getValue(), exponent), v1->getUnit() );
        else {
            if (exponent - roundf(exponent) < 1e-9)
                output = new NumberExpression(owner, pow(v1->getValue(), exponent), v1->getUnit().pow(exponent) );
            else
                throw Exception("Exponent must be an integer when used with a unit");
        }
        break;
    }
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
    if (dynamic_cast<NumberExpression*>(v1) && dynamic_cast<NumberExpression*>(v2)) {
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
        return 10;
    case DIV:
        return 10;
    case POW:
        return 10;
    }
}

/**
  * Compute the expressions dependencies, i.e the properties it relies on.
  *
  * @param props A set of Property objects where the dependencies are stored.
  */

void OperatorExpression::getDeps(std::set<const App::Property*> &props) const
{
    left->getDeps(props);
    right->getDeps(props);
}

/**
  * Compute the expressions dependencies, i.e the properties it relies on.
  *
  * @param props A set of strings. Each string contains the name of a property that this expression depends on.
  */

void OperatorExpression::getDeps(std::set<std::string> &props) const
{
    left->getDeps(props);
    right->getDeps(props);
}

//
// FunctionExpression class. This class handles functions with one or two parameters.
//

FunctionExpression::FunctionExpression(const DocumentObject *_owner, Function _f, Expression *_arg1, Expression *_arg2)
    : UnitExpression(_owner)
    , f(_f)
    , arg1(_arg1)
    , arg2(_arg2)
{
}

FunctionExpression::~FunctionExpression()
{
    delete arg1;
    if (arg2)
        delete arg2;
}

/**
  * Determinte whether the expressions is considered touched, i.e one or both of its arguments
  * are touched.
  *
  * @return True if touched, false if not.
  */

bool FunctionExpression::isTouched() const
{
    return arg1->isTouched() || ( arg2 != 0  && arg2->isTouched() );
}

/**
  * Evaluate function. Returns a NumberExpression if evaluation is successfuly.
  * Throws an exception if something fails.
  *
  * @returns A NumberExpression with the result.
  */

Expression * FunctionExpression::eval() const
{
    std::auto_ptr<Expression> e1(arg1->eval());
    std::auto_ptr<Expression> e2(arg2 ? arg2->eval() : 0);
    NumberExpression * v1 = dynamic_cast<NumberExpression*>(e1.get());
    NumberExpression * v2 = dynamic_cast<NumberExpression*>(e2.get());
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
            throw Exception("Unit must empty.");
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
            throw Exception("Unit must empty.");
        unit = Unit();
        break;
    case ABS:
        unit = v1->getUnit();
        break;
    case SQRT: {
        unit = v1->getUnit();

        // All components of unit must be either zero or dividable by 2
        if ( !((unit.getLengthDimension() % 2) == 0) &&
              ((unit.getMassDimension() % 2) == 0) &&
              ((unit.getTimeDimension() % 2) == 0) &&
              ((unit.getElectricCurrentDimension() % 2) == 0) &&
              ((unit.getThermodynamicTemperatureDimension() % 2) == 0) &&
              ((unit.getAmountOfSubstanceDimension() % 2) == 0) &&
              ((unit.getLuminoseIntensityDimension() % 2) == 0) &&
              ((unit.getAngleDimension() % 2) == 0))
            throw Exception("All dimensions must be even to compute the square root.");

        unit = Unit(unit.getLengthDimension() /2,
                    unit.getMassDimension() / 2,
                    unit.getTimeDimension() / 2,
                    unit.getElectricCurrentDimension() / 2,
                    unit.getThermodynamicTemperatureDimension() / 2,
                    unit.getAmountOfSubstanceDimension() / 2,
                    unit.getLuminoseIntensityDimension() / 2,
                    unit.getAngleDimension());
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
            if (exponent - roundf(exponent) < 1e-9)
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
        output = log(value) / log(10);
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

    return new NumberExpression(owner, output, unit, "", scaler);
}

/**
  * Try to simplify the expression, i.e calculate all constant expressions.
  *
  * @returns A simplified expression.
  */

Expression *FunctionExpression::simplify() const
{
    Expression * v1 = arg1->simplify();

    // Argument simplified to numeric expression? Then return evaluate and return
    if (dynamic_cast<NumberExpression*>(v1)) {
        switch (f) {
        case ATAN2:
        case MOD:
        case POW:
            Expression * v2 = arg1->simplify();

            if (dynamic_cast<NumberExpression*>(v2)) {
                delete v1;
                delete v2;
                return eval();
            }
            else
                return new FunctionExpression(owner, f, v1, v2);
        }
        delete v1;
        return eval();
    }
    else
        return new FunctionExpression(owner, f, v1);
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
        return "acos(" + arg1->toString() + ")";
    case ASIN:
        return "asin(" + arg1->toString() + ")";
    case ATAN:
        return "atan(" + arg1->toString() + ")";
    case ABS:
        return "abs(" + arg1->toString() + ")";
    case EXP:
        return "exp(" + arg1->toString() + ")";
    case LOG:
        return "log(" + arg1->toString() + ")";
    case LOG10:
        return "log10(" + arg1->toString() + ")";
    case SIN:
        return "sin(" + arg1->toString() + ")";
    case SINH:
        return "sinh(" + arg1->toString() + ")";
    case TAN:
        return "tan(" + arg1->toString() + ")";
    case TANH:
        return "tanh(" + arg1->toString() + ")";
    case SQRT:
        return "sqrt(" + arg1->toString() + ")";
    case COS:
        return "cos(" + arg1->toString() + ")";
    case COSH:
        return "cosh(" + arg1->toString() + ")";
    case MOD:
        return "mod(" + arg1->toString() + ", " + arg2->toString() + ")";
    case ATAN2:
        return "atan2(" + arg1->toString() + ", " + arg2->toString() +  ")";
    case POW:
        return "pow(" + arg1->toString() + ", " + arg2->toString() +  ")";
    default:
        assert(0);
    }
}

/**
  * Create a copy of the expression.
  *
  * @returns A deep copy of the expression.
  */

Expression *FunctionExpression::copy() const
{
    return new FunctionExpression(owner, f, arg1->copy(), arg2 ? arg2->copy() : 0);
}

/**
  * Compute the dependency set of the expression, i.e a set of all Property objects
  * this expression relies on.
  *
  */

void FunctionExpression::getDeps(std::set<const App::Property*> &props) const
{
    arg1->getDeps(props);
    if (arg2)
        arg2->getDeps(props);
}

/**
  * Compute the dependecy set of the expression. The set contains the names
  * of all Property objects this expression relies on.
  */

void FunctionExpression::getDeps(std::set<std::string> &props) const
{
    arg1->getDeps(props);
    if (arg2)
        arg2->getDeps(props);
}

//
// VariableExpression class
//

VariableExpression::VariableExpression(const DocumentObject *_owner, const std::string &_var)
    : UnitExpression(_owner)
    , var(_var)
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

Property * VariableExpression::getProperty() const
{
    const App::DocumentObject * docObject = 0;
    std::string propName;

    int i = var.find('.');
    std::string object = var.substr(0, i);
    if (i == std::string::npos) {
        /* No dot found -- local property */
        docObject = owner;
        propName = var;
    }
    else {
        App::Document * doc = owner->getDocument();

        docObject = doc->getObject(object.c_str());
        if (!docObject) {
            // Not found; try to find by label instead
            std::vector<DocumentObject*> docObjects;

            for (std::vector<DocumentObject*>::iterator j = docObjects.begin(); j != docObjects.end(); ++j) {
                PropertyString * label = dynamic_cast<PropertyString*>((*j)->getPropertyByName("Label"));

                if (label && strcmp(label->getValue(), object.c_str()) == 0) {
                    // Found object with matching label
                    docObject = *j;
                    break;
                }
            }

            if (!docObject)
                throw Base::Exception("Document object not found.");
        }

        propName = var.substr(i + 1);
        if (propName.size() == 0 )
            throw Base::Exception("Invalid property name");
    }

    Property * prop = docObject->getPropertyByName(propName.c_str());
    if (prop) {
        if (prop->isDerivedFrom(PropertyFloat::getClassTypeId()))
            return prop;
        else if (prop->isDerivedFrom(PropertyInteger::getClassTypeId()))
            return prop;
        else if (prop->isDerivedFrom(PropertyString::getClassTypeId()))
            return prop;
        else
            throw Base::Exception("Property is of invalid type (not float).");
    }
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
    Property * prop = getProperty();

    if (prop->isDerivedFrom(PropertyQuantity::getClassTypeId())) {
        PropertyQuantity * value = static_cast<PropertyQuantity*>(prop);
        return new NumberExpression(owner, value->getValue(), value->getUnit());
    }
    else if (prop->isDerivedFrom(PropertyFloat::getClassTypeId())) {
        PropertyFloat * value = static_cast<PropertyFloat*>(prop);
        return new NumberExpression(owner, value->getValue());
    }
    else if (prop->isDerivedFrom(PropertyInteger::getClassTypeId())) {
        PropertyInteger * value = static_cast<PropertyInteger*>(prop);
        return new NumberExpression(owner, value->getValue());
    }
    else if (prop->isDerivedFrom(PropertyString::getClassTypeId())) {
        PropertyString * value = static_cast<PropertyString*>(prop);
        return new StringExpression(owner, value->getValue());
    }
    else
        throw Base::Exception("Property is of invalid type (not float).");
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
  * Compute the depdency of the expression. For a VariableExpression, this
  * is simply the single Property, and this is inserted into the \a props parameter.
  */

void VariableExpression::getDeps(std::set<const Property*> &props) const
{
    try {
        Property * prop = getProperty();

        props.insert(prop);
    }
    catch (...) {

    }
}

/**
  * Compute the dependecy of the expression. In this case \a props
  * is a set of strings, i.e the names of the Property objects, and
  * the variable name this expression relies on is inserted into the set.
  * Notice that the variable may be unqualified, i.e without any reference
  * to the owning object. This must be taken into consideration when using
  * the set.
  */

void VariableExpression::getDeps(std::set<std::string> &props) const
{
    props.insert(var);
}

//
// StringExpression class
//

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

/**
  * Return a copy of the expression.
  */

Expression *StringExpression::copy() const
{
    return new StringExpression(owner, text);
}

namespace Spreadsheet {

namespace ExpressionParser {

/**
 * Error function for parser. Throws a generic Base::Exception with the parser error.
 */

void ExpressionParser_yyerror(char *errorinfo)
{
    throw Base::Exception(errorinfo);
}

// for VC9 (isatty and fileno not supported anymore)
#ifdef _MSC_VER
int isatty (int i) {return _isatty(i);}
int fileno(FILE *stream) {return _fileno(stream);}
#endif
Expression * ScanResult = 0;                    /**< The resulting expression after a successful parsing */
Base::Unit unit;                                /**< Variable used to hold current unit during parsing */
double scaler = 0;                              /**< Variable used to hold current scaler during parsing */
const char * unitstr;                           /**< Variable used to hold current unit string during parsing */
const App::DocumentObject * DocumentObject = 0; /**< The DocumentObject that will own the expression */
bool unitExpression = false;                    /**< True if the parsed string is a unit only */
bool valueExpression = false;                   /**< True if the parsed string is a full expression */

// show the parser the lexer method
#define yylex ExpressionParserlex
int ExpressionParserlex(void);

// Parser, defined in UnitsApi.y
#include "ExpressionParser.tab.c"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
// Scanner, defined in UnitsApi.l
#include "lex.ExpressionParser.c"
#endif // DOXYGEN_SHOULD_SKIP_THIS

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

    // set the global variables
    ScanResult = 0;
    DocumentObject = owner;
    unit = Unit();
    scaler = 1.0;
    unitstr = 0;
    unitExpression = valueExpression = false;

    // run the parser
    ExpressionParser::ExpressionParser_yyparse ();

    // free the scan buffer
    ExpressionParser::ExpressionParser_delete_buffer (my_string_buffer);

    if (ScanResult == 0)
        throw Base::Exception("Unknown error in expression");

    // Simplify expression
    Expression * simplified = ScanResult->simplify();
    delete ScanResult;

    if (valueExpression)
        return simplified;
    else {
        delete simplified;
        throw Expression::Exception("Expression can not evaluate to a value.");
        return 0;
    }
}

UnitExpression * ExpressionParser::parseUnit(const App::DocumentObject *owner, const char* buffer)
{
    // parse from buffer
    ExpressionParser::YY_BUFFER_STATE my_string_buffer = ExpressionParser::ExpressionParser_scan_string (buffer);

    // set the global variables
    ScanResult = 0;
    DocumentObject = owner;
    unit = Unit();
    scaler = 1.0;
    unitstr = 0;
    unitExpression = valueExpression = false;

    // run the parser
    ExpressionParser::ExpressionParser_yyparse ();

    // free the scan buffer
    ExpressionParser::ExpressionParser_delete_buffer (my_string_buffer);

    if (ScanResult == 0)
        throw Base::Exception("Unknown error in expression");

    std::string s = ScanResult->toString();

    // Simplify expression
    Expression * simplified = ScanResult->simplify();
    delete ScanResult;

    if (unitExpression)
        return dynamic_cast<UnitExpression*>(simplified);
    else {
        delete simplified;
        throw Expression::Exception("Expression is not a unit.");
        return 0;
    }
}
