#include "PreCompiled.h"
#ifdef __GNUC__
# include <unistd.h>
#endif

#include <QString>
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

NumberExpression::NumberExpression(const DocumentObject *_owner, double _value, const Base::Unit & _unit, const char * _unitstr, double _scaler)
    : UnitExpression(_owner, _unit, _unitstr, _scaler)
    , value(_value)
{
}

Expression * NumberExpression::eval() const
{
    return copy();
}

Expression *NumberExpression::simplify() const
{
    return copy();
}

std::string NumberExpression::toString() const
{
    std::stringstream s;

    s << value;

    return s.str();
}

Expression *NumberExpression::copy() const
{
    return new NumberExpression(owner, value, unit, unitstr.c_str(), scaler);
}

void NumberExpression::negate()
{
    value = -value;
}

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

bool OperatorExpression::isTouched() const
{
    return left->isTouched() || right->isTouched();
}

Expression * OperatorExpression::eval() const
{
    NumberExpression * v1;
    NumberExpression * v2;
    NumberExpression * output;

    v1 = dynamic_cast<NumberExpression*>(left->eval());
    v2 = dynamic_cast<NumberExpression*>(right->eval());

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
    delete v1;
    delete v2;

    return output;
}

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

Expression *OperatorExpression::copy() const
{
    return new OperatorExpression(owner, left->copy(), op, right->copy());
}

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

void OperatorExpression::getDeps(std::set<const App::Property*> &props) const
{
    left->getDeps(props);
    right->getDeps(props);
}

void OperatorExpression::getDeps(std::set<std::string> &props) const
{
    left->getDeps(props);
    right->getDeps(props);
}

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

bool FunctionExpression::isTouched() const
{
    return arg1->isTouched() || ( arg2 != 0  && arg2->isTouched() );
}

Expression * FunctionExpression::eval() const
{
    NumberExpression * v1 = dynamic_cast<NumberExpression*>(arg1->eval());
    double output;

    if (v1 == 0)
        throw Exception("Invalid argument.");

    switch (f) {
    case ACOS:
        output = acos(v1->getValue());
        break;
    case ASIN:
        output = asin(v1->getValue());
        break;
    case ATAN:
        output = atan(v1->getValue());
        break;
    case ABS:
        output = fabs(v1->getValue());
        break;
    case EXP:
        output = exp(v1->getValue());
        break;
    case LOG:
        output = log(v1->getValue());
        break;
    case LOG10:
        output = log(v1->getValue()) / log(10);
        break;
    case SIN:
        output = sin(v1->getValue());
        break;
    case SINH:
        output = sinh(v1->getValue());
        break;
    case TAN:
        output = tan(v1->getValue());
        break;
    case TANH:
        output = tanh(v1->getValue());
        break;
    case SQRT:
        output = sqrt(v1->getValue());
        break;
    case COS:
        output = cos(v1->getValue());
        break;
    case MOD: {
        NumberExpression * v2 = dynamic_cast<NumberExpression*>(arg2->eval());

        if (v2 == 0)
            throw Exception("Invalid argument.");

        output = fmod(v1->getValue(), v2->getValue());
        delete v2;
        break;
    }
    case ATAN2: {
        NumberExpression * v2 = dynamic_cast<NumberExpression*>(arg2->eval());

        if (v2 == 0)
            throw Exception("Invalid argument.");

        output = atan2(v1->getValue(), v2->getValue());
        delete v2;
        break;
    }
    case POW: {
        NumberExpression * v2 = dynamic_cast<NumberExpression*>(arg2->eval());

        if (v2 == 0)
            throw Exception("Invalid argument.");

        output = pow(v1->getValue(), v2->getValue());
        delete v2;
        break;
    }
    default:
        assert(0);
    }

    delete v1;

    return new NumberExpression(owner, output);
}

Expression *FunctionExpression::simplify() const
{
    Expression * v1 = arg1->simplify();

    // Argument simplified to numeric expression? Then return evaluate and return
    if (dynamic_cast<NumberExpression*>(v1)) {
        delete v1;
        switch (f) {
        case ATAN2:
        case MOD:
        case POW:
            Expression * v2 = arg1->simplify();

            if (dynamic_cast<NumberExpression*>(v2))
                return eval();
            else
                return new FunctionExpression(owner, f, v1, v2);
        }
        delete v1;
        return eval();
    }
    else
        return new FunctionExpression(owner, f, v1);
}

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

Expression *FunctionExpression::copy() const
{
    return new FunctionExpression(owner, f, arg1->copy(), arg2 ? arg2->copy() : 0);
}

void FunctionExpression::getDeps(std::set<const App::Property*> &props) const
{
    arg1->getDeps(props);
    if (arg2)
        arg2->getDeps(props);
}

void FunctionExpression::getDeps(std::set<std::string> &props) const
{
    arg1->getDeps(props);
    if (arg2)
        arg2->getDeps(props);
}

VariableExpression::VariableExpression(const DocumentObject *_owner, const std::string &_var)
    : UnitExpression(_owner)
    , var(_var)
{
}

bool VariableExpression::isTouched() const
{
    try {
        return getProperty()->isTouched();
    }
    catch (...) {
        return false;
    }
}

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
        App::Document * doc = App::GetApplication().getActiveDocument();

        if (!doc)
            throw Base::Exception("No active document.");

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

Expression *VariableExpression::simplify() const
{
    return copy();
}

Expression *VariableExpression::copy() const
{
    return new VariableExpression(owner, var);
}

void VariableExpression::getDeps(std::set<const Property*> &props) const
{
    try {
        Property * prop = getProperty();

        props.insert(prop);
    }
    catch (...) {

    }
}

void VariableExpression::getDeps(std::set<std::string> &props) const
{
    props.insert(var);
}

StringExpression::StringExpression(const DocumentObject *_owner, const std::string &_text)
    : Expression(_owner)
    , text(_text)
{
}

Expression * StringExpression::eval() const
{
    return copy();
}

Expression *StringExpression::simplify() const
{
    return copy();
}

Expression *StringExpression::copy() const
{
    return new StringExpression(owner, text);
}



UnitExpression::UnitExpression(const DocumentObject *_owner, const Base::Unit & _unit, const char *_unitstr, double _scaler)
    : Expression(_owner)
    , unit(_unit)
    , unitstr(_unitstr ? _unitstr : "")
    , scaler(_scaler)
{
}

void UnitExpression::setUnit(const Unit &_unit, const char *_unitstr, double _scaler)
{
    unit = _unit;
    unitstr = _unitstr ? _unitstr : "";
    scaler = _scaler;
}

Expression *UnitExpression::eval() const
{
    return new NumberExpression(owner, 1.0, unit, unitstr.c_str(), scaler);
}

Expression *UnitExpression::simplify() const
{
    return new NumberExpression(owner, 1.0, unit, unitstr.c_str(), scaler);
}

std::string UnitExpression::toString() const
{
    return unitstr;
}

Expression *UnitExpression::copy() const
{
    return new UnitExpression(owner, unit, unitstr.c_str(), scaler);
}

namespace Spreadsheet {

namespace ExpressionParser {

// error func
void ExpressionParser_yyerror(char *errorinfo)
{
    throw Base::Exception(errorinfo);
}

// for VC9 (isatty and fileno not supported anymore)
#ifdef _MSC_VER
int isatty (int i) {return _isatty(i);}
int fileno(FILE *stream) {return _fileno(stream);}
#endif
Expression * ScanResult = 0;
Base::Unit unit;
double scaler = 0;
const char * unitstr;
const App::DocumentObject * DocumentObject = 0;
bool unitExpression = false;
bool valueExpression = false;

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

    std::string s = ScanResult->toString();

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
