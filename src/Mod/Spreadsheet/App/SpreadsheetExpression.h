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

#ifndef SPREADSHEET_EXPRESSION_H
#define SPREADSHEET_EXPRESSION_H

#include <string>
#include <App/DocumentObject.h>
#include <Base/Exception.h>
#include <Base/Unit.h>
#include <App/Property.h>
#include <Base/BaseClass.h>
#include <Base/Quantity.h>
#include <Mod/Spreadsheet/App/Range.h>
#include <set>

namespace Spreadsheet  {

class SpreadsheetExport RangeExpression : public App::Expression {
    TYPESYSTEM_HEADER();
public:
    RangeExpression(const App::DocumentObject * _owner = 0, const std::string & begin = std::string(), const std::string & end = std::string());

    virtual ~RangeExpression() { }

    virtual bool isTouched() const;

    virtual Expression * eval() const;

    virtual std::string toString() const;

    virtual Expression * copy() const;

    virtual int priority() const { return 20; }

    virtual void getDeps(std::set<App::ObjectIdentifier> &props) const;

    virtual App::Expression * simplify() const;

    Range getRange() const { return range; }

    void setRange(const Range & r);

protected:
    Range range;
};

/**
  * Class implementing various functions, e.g sin, cos, etc.
  *
  */

class SpreadsheetExport AggregateFunctionExpression : public App::FunctionExpression {
    TYPESYSTEM_HEADER();
public:
    enum Function {
        NONE,

        // Aggregates
        SUM = App::FunctionExpression::LAST,
        AVERAGE,
        STDDEV,
        COUNT,
        MIN,
        MAX
    };

    AggregateFunctionExpression(const App::DocumentObject *_owner = 0,
                                App::FunctionExpression::Function _f = App::FunctionExpression::NONE,
                                std::vector<Expression *> _args = std::vector<Expression*>());

    virtual ~AggregateFunctionExpression();

    virtual Expression * copy() const;

    virtual App::Expression * eval() const;

    virtual std::string toString() const;
};

namespace ExpressionParser {
SpreadsheetExport App::Expression * parse(const App::DocumentObject *owner, const char *buffer);
SpreadsheetExport App::UnitExpression * parseUnit(const App::DocumentObject *owner, const char *buffer);
SpreadsheetExport App::ObjectIdentifier parsePath(const App::DocumentObject *owner, const char* buffer);
}

}
#endif // EXPRESSION_H
