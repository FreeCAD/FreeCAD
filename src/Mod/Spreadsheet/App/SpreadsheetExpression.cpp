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
#include "SpreadsheetExpression.h"
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

using namespace App;
using namespace Base;
using namespace Spreadsheet;

//
// FunctionExpression class. This class handles functions with one or two parameters.
//

TYPESYSTEM_SOURCE(Spreadsheet::AggregateFunctionExpression, App::FunctionExpression);

AggregateFunctionExpression::AggregateFunctionExpression(const DocumentObject *_owner,
                                                         App::FunctionExpression::Function _f,
                                                         std::vector<Expression *> _args)
    : FunctionExpression(_owner, static_cast<FunctionExpression::Function>(_f), _args)
{
}

AggregateFunctionExpression::~AggregateFunctionExpression()
{
}

/**
  * Evaluate function. Returns a NumberExpression if evaluation is successfuly.
  * Throws an exception if something fails.
  *
  * @returns A NumberExpression with the result.
  */

Expression * AggregateFunctionExpression::eval() const
{
    switch (static_cast<Function>(f)) {
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

            if ((qp = freecad_dynamic_cast<PropertyQuantity>(p)) != 0)
                value = qp->getQuantityValue();
            else if ((fp = freecad_dynamic_cast<PropertyFloat>(p)) != 0)
                value = fp->getValue();
            else
                throw Exception("Invalid property type for aggregate");

            if (first) {
                q.setUnit(value.getUnit());
                mean.setUnit(value.getUnit());
                M2.setUnit(value.getUnit());
            }

            switch (static_cast<Function>(f)) {
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
            default:
                break;
            }

            first = false;
        } while (range.next());

        switch (static_cast<Function>(f)) {
        case AVERAGE:
            q = q / (double)n;
            break;
        case STDDEV:
            if (n < 2)
                q = Quantity();
            else
                q = (M2 / (n - 1.0)).pow(Quantity(0.5));
            break;
        default:
            break;
        }

        return new NumberExpression(owner, q);
    }
    default:
        return App::FunctionExpression::eval();
    }
}

/**
  * Create a string representation of the expression.
  *
  * @returns A string representing the expression.
  */

std::string AggregateFunctionExpression::toString() const
{
    switch (static_cast<Function>(f)) {
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
        return App::FunctionExpression::toString();
    }
}

TYPESYSTEM_SOURCE(Spreadsheet::RangeExpression, App::Expression);

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
static const DocumentObject * DocumentObject = 0; /**< The DocumentObject that will own the expression */
static bool unitExpression = false;                    /**< True if the parsed string is a unit only */
static bool valueExpression = false;                   /**< True if the parsed string is a full expression */
static std::stack<std::string> labels;                /**< Label string primitive */
static std::map<std::string, FunctionExpression::Function> registered_functions;                /**< Registerd functions */

// show the parser the lexer method
#undef YYTOKENTYPE
#undef YYSTYPE
#undef YYSTYPE_ISDECLARED
#define yylex ExpressionParserlex
int ExpressionParserlex(void);

// Parser, defined in ExpressionParser.y
#include <Mod/Spreadsheet/App/ExpressionParser.tab.c>
#include <Mod/Spreadsheet/App/ExpressionParser.tab.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS
// Scanner, defined in ExpressionParser.l
#include <Mod/Spreadsheet/App/lex.ExpressionParser.c>
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
        registered_functions["sum"] = static_cast<FunctionExpression::Function>(AggregateFunctionExpression::SUM);
        registered_functions["count"] = static_cast<FunctionExpression::Function>(AggregateFunctionExpression::COUNT);
        registered_functions["average"] = static_cast<FunctionExpression::Function>(AggregateFunctionExpression::AVERAGE);
        registered_functions["stddev"] = static_cast<FunctionExpression::Function>(AggregateFunctionExpression::STDDEV);
        registered_functions["min"] = static_cast<FunctionExpression::Function>(AggregateFunctionExpression::MIN);
        registered_functions["max"] = static_cast<FunctionExpression::Function>(AggregateFunctionExpression::MAX);

        has_registered_functions = true;
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

Expression * parse(const App::DocumentObject *owner, const char* buffer)
{
    // parse from buffer
    ExpressionParser::YY_BUFFER_STATE my_string_buffer = ExpressionParser_scan_string (buffer);

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

UnitExpression * parseUnit(const App::DocumentObject *owner, const char* buffer)
{
    // parse from buffer
    ExpressionParser::YY_BUFFER_STATE my_string_buffer = ExpressionParser_scan_string (buffer);

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

}

}

