// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <set>
#include <string>

#include <App/Expression.h>
#include <App/Material.h>

#include <Mod/Spreadsheet/SpreadsheetGlobal.h>

#include "DisplayUnit.h"
#include "Utils.h"


namespace Base
{
class Unit;
class XMLReader;
class Writer;
}  // namespace Base

namespace Spreadsheet
{

class PropertySheet;
class DisplayUnit;

class SpreadsheetExport Cell
{
private:
    Cell(const Cell& other);

public:
    Cell(const App::CellAddress& _address, PropertySheet* _owner);

    Cell(PropertySheet* _owner, const Cell& other);

    Cell& operator=(const Cell& rhs);

    ~Cell();

    const App::Expression* getExpression(bool withFormat = false) const;

    bool getStringContent(std::string& s, bool persistent = false) const;

    void setContent(const char* value);

    void setAlignment(int _alignment);
    bool getAlignment(int& _alignment) const;

    void setStyle(const std::set<std::string>& _style);
    bool getStyle(std::set<std::string>& style) const;

    void setForeground(const Base::Color& color);
    bool getForeground(Base::Color& color) const;

    void setBackground(const Base::Color& color);
    bool getBackground(Base::Color& color) const;

    void setDisplayUnit(const std::string& unit);
    bool getDisplayUnit(DisplayUnit& unit) const;

    void setAlias(const std::string& n);
    bool getAlias(std::string& n) const;

    void setComputedUnit(const Base::Unit& unit);
    bool getComputedUnit(Base::Unit& unit) const;

    void setSpans(int rows, int columns);
    bool getSpans(int& rows, int& columns) const;

    void setException(const std::string& e, bool silent = false);

    void clearException();

    void clearDirty();

    void setDirty();

    void setResolveException(const std::string& e);

    void clearResolveException();

    const std::string& getException() const
    {
        return exceptionStr;
    }

    bool hasException() const
    {
        return isUsed(EXCEPTION_SET) || isUsed(PARSE_EXCEPTION_SET) || isUsed(RESOLVE_EXCEPTION_SET);
    }

    void moveAbsolute(App::CellAddress newAddress);

    void restore(Base::XMLReader& reader, bool checkAlias = false);

    void afterRestore();

    void save(Base::Writer& writer) const;
    void save(std::ostream& os, const char* indent, bool noContent) const;

    bool isUsed() const;

    void mark()
    {
        setUsed(MARK_SET);
    }

    bool isMarked() const
    {
        return isUsed(MARK_SET);
    }

    void visit(App::ExpressionVisitor& v);

    App::CellAddress getAddress() const
    {
        return address;
    }

    std::string getFormattedQuantity();

    /* Alignment */
    static const int ALIGNMENT_LEFT;
    static const int ALIGNMENT_HCENTER;
    static const int ALIGNMENT_RIGHT;
    static const int ALIGNMENT_HORIZONTAL;
    static const int ALIGNMENT_HIMPLIED;
    static const int ALIGNMENT_TOP;
    static const int ALIGNMENT_VCENTER;
    static const int ALIGNMENT_BOTTOM;
    static const int ALIGNMENT_VERTICAL;
    static const int ALIGNMENT_VIMPLIED;

    /* Static functions */
    static int decodeAlignment(const std::string& itemStr, int alignment);
    static std::string encodeAlignment(int alignment);

    static std::string encodeStyle(const std::set<std::string>& style);

    static std::string encodeColor(const Base::Color& color);
    static Base::Color decodeColor(const std::string& color, const Base::Color& defaultColor);

private:
    void setParseException(const std::string& e);

    void setExpression(App::ExpressionPtr&& expr);

    void setUsed(int mask, bool state = true);

    bool isUsed(int mask) const;

    void freeze();

    void unfreeze();

    /* Used */
    static const int EXPRESSION_SET;
    static const int ALIGNMENT_SET;
    static const int STYLE_SET;
    static const int BACKGROUND_COLOR_SET;
    static const int FOREGROUND_COLOR_SET;
    static const int DISPLAY_UNIT_SET;
    static const int COMPUTED_UNIT_SET;
    static const int ALIAS_SET;
    static const int SPANS_SET;
    static const int MARK_SET;
    static const int EXCEPTION_SET;
    static const int PARSE_EXCEPTION_SET;
    static const int RESOLVE_EXCEPTION_SET;

    App::CellAddress address;
    PropertySheet* owner;

    int used;
    mutable App::ExpressionPtr expression;
    int alignment;
    std::set<std::string> style;
    Base::Color foregroundColor;
    Base::Color backgroundColor;
    DisplayUnit displayUnit;
    std::string alias;
    Base::Unit computedUnit;
    int rowSpan;
    int colSpan;
    std::string exceptionStr;
    App::CellAddress anchor;
    friend class PropertySheet;
};

}  // namespace Spreadsheet
