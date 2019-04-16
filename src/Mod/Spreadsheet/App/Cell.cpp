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

#ifndef _PreComp_
#endif

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "Cell.h"
#include "Utils.h"
#include <boost/tokenizer.hpp>
#include <Base/Reader.h>
#include <Base/Quantity.h>
#include <Base/Writer.h>
#include <Base/Console.h>
#include <App/ExpressionParser.h>
#include "Sheet.h"
#include <iomanip>

FC_LOG_LEVEL_INIT("Spreadsheet",true,true);

#ifdef _MSC_VER
#define __func__ __FUNCTION__
#endif

using namespace App;
using namespace Base;
using namespace Spreadsheet;

/////////////////////////////////////////////////////////

// expose the read() function for simpler partial xml reading in setExpression()
class ReaderPrivate: public Base::XMLReader {
public:
    ReaderPrivate(const char* FileName, std::istream &is)
        :XMLReader(FileName,is)
    {}

    bool read() {
        return XMLReader::read();
    }
};

///////////////////////////////////////////////////////////

const int Cell::EXPRESSION_SET       = 1;
const int Cell::ALIGNMENT_SET        = 4;
const int Cell::STYLE_SET            = 8;
const int Cell::BACKGROUND_COLOR_SET = 0x10;
const int Cell::FOREGROUND_COLOR_SET = 0x20;
const int Cell::DISPLAY_UNIT_SET     = 0x40;
const int Cell::COMPUTED_UNIT_SET    = 0x80;
const int Cell::ALIAS_SET            = 0x100;
const int Cell::SPANS_SET            = 0x200;
const int Cell::MARK_SET             = 0x40000000;
const int Cell::EXCEPTION_SET        = 0x20000000;
const int Cell::PARSE_EXCEPTION_SET  = 0x80000000;
const int Cell::RESOLVE_EXCEPTION_SET= 0x01000000;
const int Cell::SPANS_UPDATED        = 0x10000000;

/* Alignment */
const int Cell::ALIGNMENT_LEFT       = 0x01;
const int Cell::ALIGNMENT_HCENTER    = 0x02;
const int Cell::ALIGNMENT_RIGHT      = 0x04;
const int Cell::ALIGNMENT_HIMPLIED   = 0x08;
const int Cell::ALIGNMENT_HORIZONTAL = 0x0f;
const int Cell::ALIGNMENT_TOP        = 0x10;
const int Cell::ALIGNMENT_VCENTER    = 0x20;
const int Cell::ALIGNMENT_BOTTOM     = 0x40;
const int Cell::ALIGNMENT_VIMPLIED   = 0x80;
const int Cell::ALIGNMENT_VERTICAL   = 0xf0;

/**
  * Construct a CellContent object.
  * @param _address  The address of the cell (i.e. row and column)
  * @param _owner    The spreadsheet that owns this cell.
  *
  */

Cell::Cell(const CellAddress &_address, PropertySheet *_owner)
    : address(_address)
    , owner(_owner)
    , used(0)
    , alignment(ALIGNMENT_HIMPLIED | ALIGNMENT_LEFT | ALIGNMENT_VIMPLIED | ALIGNMENT_VCENTER)
    , style()
    , foregroundColor(0, 0, 0, 1)
    , backgroundColor(1, 1, 1, 1)
    , displayUnit()
    , alias()
    , computedUnit()
    , rowSpan(1)
    , colSpan(1)
    , anchor()
    , editMode(EditNormal)
{
    assert(address.isValid());
}

Cell::Cell(PropertySheet *_owner, const Cell &other)
    : address(other.address)
    , owner(_owner)
    , used(other.used)
    , expression(other.expression ? other.expression->copy() : nullptr)
    , alignment(other.alignment)
    , style(other.style)
    , foregroundColor(other.foregroundColor)
    , backgroundColor(other.backgroundColor)
    , displayUnit(other.displayUnit)
    , alias(other.alias)
    , computedUnit(other.computedUnit)
    , rowSpan(other.rowSpan)
    , colSpan(other.colSpan)
    , editMode(other.editMode)
{
    setUsed(MARK_SET, false);
}

Cell &Cell::operator =(const Cell &rhs)
{
    PropertySheet::AtomicPropertyChange signaller(*owner);

    address = rhs.address;

    setExpression(rhs.expression ? rhs.expression->copy() : ExpressionPtr());
    setAlignment(rhs.alignment);
    setStyle(rhs.style);
    setBackground(rhs.backgroundColor);
    setForeground(rhs.foregroundColor);
    setDisplayUnit(rhs.displayUnit.stringRep);
    setComputedUnit(rhs.computedUnit);
    setAlias(rhs.alias);
    setSpans(rhs.rowSpan, rhs.colSpan);
    editMode = rhs.editMode;

    setUsed(MARK_SET, false);

    signaller.tryInvoke();
    return *this;
}

/**
  * Destroy a CellContent object.
  *
  */

Cell::~Cell()
{
}

/**
  * Set the expression tree to \a expr.
  *
  */

void Cell::setExpression(App::ExpressionPtr &&expr)
{
    PropertySheet::AtomicPropertyChange signaller(*owner);

    /* Remove dependencies */
    owner->removeDependencies(address);

    auto func = SimpleStatement::cast<FunctionStatement>(expr.get());
    if(func)
        setAlias(func->getName());

    if(expr && expr->comment.size()) {
        if(!boost::starts_with(expr->comment,"<Cell "))
            FC_WARN("Unknown style of cell "
                << owner->sheet()->getFullName() << '.' << address.toString());
        else {
            try {
                std::istringstream in(expr->comment);
                ReaderPrivate reader("<memory>", in);
                reader.read();
                restore(reader,true);
            }catch(Base::Exception &e) {
                e.ReportException();
                FC_ERR("Failed to restore style of cell "
                    << owner->sheet()->getFullName() << '.' 
                    << address.toString() << ": " << e.what());
            }
        }
        expr->comment.clear();
    }

    auto simple = Base::freecad_dynamic_cast<SimpleStatement>(expr.get());
    if(simple)
        expression = simple->reduce();
    else
        expression.reset();
    if(!expression)
        expression = std::move(expr);

    setUsed(EXPRESSION_SET, !!expression);

    /* Update dependencies */
    owner->addDependencies(address);
    owner->setDirty(address);

    signaller.tryInvoke();
}

/**
  * Get the expression tree.
  *
  */

const App::Expression *Cell::getExpression(bool withFormat) const
{
    if(withFormat && expression) {
        if(editMode || (used & (ALIGNMENT_SET
                                | STYLE_SET
                                | FOREGROUND_COLOR_SET
                                | BACKGROUND_COLOR_SET
                                | DISPLAY_UNIT_SET 
                                | ALIAS_SET
                                | SPANS_SET)))
        {
            std::ostringstream ss;
            save(ss,"",true);
            expression->comment = ss.str();
        }
    }
    return expression.get();
}

/**
  * Get string content.
  *
  */

bool Cell::getStringContent(std::string & s, bool persistent) const
{
    if (expression) {
        auto sexpr = SimpleStatement::cast<App::StringExpression>(expression.get());
        if(sexpr) {
            const auto &txt = sexpr->getText();
            if(txt.size() && txt[0]!='=') {
                s = "'";
                s += txt;
            }else
                s = txt;
        }
        else if (SimpleStatement::cast<App::ConstantExpression>(expression.get()))
            s = "=" + expression->toString();
        else if (SimpleStatement::cast<App::NumberExpression>(expression.get()))
            s = expression->toString();
        else
            s = "=" + expression->toString(persistent);

        return true;
    }
    else {
        s = "";
        return false;
    }
}

void Cell::afterRestore() {
    auto expr = SimpleStatement::cast<StringExpression>(expression.get());
    if(expr) 
        setContent(expr->getText().c_str());
}

void Cell::setContent(const char * value)
{
    PropertySheet::AtomicPropertyChange signaller(*owner);
    App::ExpressionPtr expr;

    setUsed(PARSE_EXCEPTION_SET, false);
    if (value != 0) {
        if(owner->sheet()->isRestoring()) {
            expression = StringExpression::create(owner->sheet(),value);
            setUsed(EXPRESSION_SET, true);
            signaller.tryInvoke();
            return;
        }
        if (*value == '=') {
            try {
                expr = owner->parse(value + 1, 0, true);
            }
            catch (Base::Exception & e) {
                expr = App::StringExpression::create(owner->sheet(), value);
                setParseException(e.what());
            }
        }
        else if (*value == '\'')
            expr = App::StringExpression::create(owner->sheet(), value + 1);
        else if (*value != '\0') {
            char * end;
            errno = 0;
            double float_value = strtod(value, &end);
            if (!*end && errno == 0)
                expr = App::NumberExpression::create(owner->sheet(), Quantity(float_value));
            else {
                try {
                    expr = owner->parse(value);
                    // owner->eval(expr.get());
                }
                catch (Base::Exception &e) {
                    expr = App::StringExpression::create(owner->sheet(), value);
                }
            }
        }
    }

    try {
        setExpression(std::move(expr));
        signaller.tryInvoke();
    } catch (Base::Exception &e) {
        if(value) {
            std::string _value;
            if(*value != '=') {
                _value = "=";
                _value += value;
                value = _value.c_str();
            }
            setExpression(App::StringExpression::create(owner->sheet(), value));
            setParseException(e.what());
        }
    }
}

/**
  * Set alignment of this cell. Alignment is the or'ed value of
  * vertical and horizontal alignment, given by the constants
  * defined in the class.
  *
  */

void Cell::setAlignment(int _alignment)
{
    if (_alignment != alignment) {
        PropertySheet::AtomicPropertyChange signaller(*owner);

        alignment = _alignment;
        setUsed(ALIGNMENT_SET, alignment != (ALIGNMENT_HIMPLIED | ALIGNMENT_LEFT | ALIGNMENT_VIMPLIED | ALIGNMENT_VCENTER));
        signaller.tryInvoke();
    }
}

/**
  * Get alignment.
  *
  */

bool Cell::getAlignment(int & _alignment) const
{
    _alignment = alignment;
    return isUsed(ALIGNMENT_SET);
}

/**
  * Set style to the given set \a _style.
  *
  */

void Cell::setStyle(const std::set<std::string> & _style)
{
    if (_style != style) {
        PropertySheet::AtomicPropertyChange signaller(*owner);

        style = _style;
        setUsed(STYLE_SET, style.size() > 0);

        signaller.tryInvoke();
    }
}

/**
  * Get the style of the cell.
  *
  */

bool Cell::getStyle(std::set<std::string> & _style) const
{
    _style = style;
    return isUsed(STYLE_SET);
}

/**
  * Set foreground (i.e text) color of the cell to \a color.
  *
  */

void Cell::setForeground(const App::Color &color)
{
    if (color != foregroundColor) {
        PropertySheet::AtomicPropertyChange signaller(*owner);

        foregroundColor = color;
        setUsed(FOREGROUND_COLOR_SET, foregroundColor != App::Color(0, 0, 0, 1));

        signaller.tryInvoke();
    }
}

/**
  * Get foreground color of the cell.
  *
  */

bool Cell::getForeground(App::Color &color) const
{
    color = foregroundColor;
    return isUsed(FOREGROUND_COLOR_SET);
}

/**
  * Set background color of the cell to \a color.
  *
  */

void Cell::setBackground(const App::Color &color)
{
    if (color != backgroundColor) {
        PropertySheet::AtomicPropertyChange signaller(*owner);

        backgroundColor = color;
        setUsed(BACKGROUND_COLOR_SET, backgroundColor != App::Color(1, 1, 1, 0));

        signaller.tryInvoke();
    }
}

/**
  * Get the background color of the cell into \a color.
  *
  * @returns true if the background color was previously set.
  *
  */

bool Cell::getBackground(App::Color &color) const
{
    color = backgroundColor;
    return isUsed(BACKGROUND_COLOR_SET);
}

/**
  * Set the display unit for the cell.
  *
  */

void Cell::setDisplayUnit(const std::string &unit)
{
    DisplayUnit newDisplayUnit;
    if (unit.size() > 0) {
        auto e = Expression::parseUnit(owner->sheet(), unit.c_str());
        if (!e)
            throw Base::UnitsMismatchError("Invalid unit");
        UnitExpression *expr = static_cast<UnitExpression*>(e.get());
        newDisplayUnit = DisplayUnit(unit, expr->getUnit(), expr->getScaler());
    }

    if (newDisplayUnit != displayUnit) {
        PropertySheet::AtomicPropertyChange signaller(*owner);

        displayUnit = newDisplayUnit;
        setUsed(DISPLAY_UNIT_SET, !displayUnit.isEmpty());

        signaller.tryInvoke();
    }
}

/**
  * Get the display unit for the cell into unit.
  *
  * @returns true if the display unit was previously set.
  *
  */

bool Cell::getDisplayUnit(DisplayUnit &unit) const
{
    unit = displayUnit;
    return isUsed(DISPLAY_UNIT_SET);
}

void Cell::setAlias(const std::string &n)
{
    if (alias != n) {
        PropertySheet::AtomicPropertyChange signaller(*owner);

        owner->revAliasProp.erase(alias);

        alias = n;

        // Update owner
        if (alias != "") {
            owner->aliasProp[address] = n;
            owner->revAliasProp[n] = address;
        }
        else
            owner->aliasProp.erase(address);

        setUsed(ALIAS_SET, !alias.empty());

        signaller.tryInvoke();
    }
}

bool Cell::getAlias(std::string &n) const
{
    n = alias;
    return isUsed(ALIAS_SET);
}

/**
  * Set the computed unit for the cell to \a unit.
  *
  */

void Cell::setComputedUnit(const Base::Unit &unit)
{
    PropertySheet::AtomicPropertyChange signaller(*owner);

    computedUnit = unit;
    setUsed(COMPUTED_UNIT_SET, !computedUnit.isEmpty());

    signaller.tryInvoke();
}

/**
  * Get the computed unit into \a unit.
  *
  * @returns true if the computed unit was previously set.
  *
  */

bool Cell::getComputedUnit(Base::Unit & unit) const
{
    unit = computedUnit;
    return isUsed(COMPUTED_UNIT_SET);
}

/**
  * Set the cell's row and column span to \a rows and \a columns. This
  * is done when cells are merged.
  *
  */

void Cell::setSpans(int rows, int columns)
{
    if (rows != rowSpan || columns != colSpan) {
        PropertySheet::AtomicPropertyChange signaller(*owner);

        rowSpan = (rows == -1 ? 1 : rows);
        colSpan = (columns == -1 ? 1 : columns);
        setUsed(SPANS_SET, (rowSpan != 1 || colSpan != 1) );
        setUsed(SPANS_UPDATED);
        signaller.tryInvoke();
    }
}

/**
  * Get the row and column spans for the cell into \a rows and \a columns.
  *
  */

bool Cell::getSpans(int &rows, int &columns) const
{
    rows = rowSpan;
    columns = colSpan;
    return isUsed(SPANS_SET);
}

void Cell::setException(const std::string &e)
{
    if(e.size() && owner && owner->sheet()) {
        FC_ERR(owner->sheet()->getFullName() << '.' 
                << address.toString() << ": " << e);
    }
    exceptionStr = e;
    setUsed(EXCEPTION_SET);
}

void Cell::setParseException(const std::string &e)
{
    if(e.size() && owner && owner->sheet()) {
        FC_ERR(owner->sheet()->getFullName() << '.' 
                << address.toString() << ": " << e);
    }
    exceptionStr = e;
    setUsed(PARSE_EXCEPTION_SET);
}

void Cell::setResolveException(const std::string &e)
{
    if(e.size() && owner && owner->sheet()) {
        FC_ERR(owner->sheet()->getFullName() << '.' 
                << address.toString() << ": " << e);
    }
    exceptionStr = e;
    setUsed(RESOLVE_EXCEPTION_SET);
}

void Cell::clearResolveException()
{
    setUsed(RESOLVE_EXCEPTION_SET, false);
}

void Cell::clearException()
{
    if (!isUsed(PARSE_EXCEPTION_SET))
        exceptionStr = "";
    setUsed(EXCEPTION_SET, false);
}

void Cell::clearDirty()
{
    owner->clearDirty(address);
}

/**
  * Move the cell to a new position given by \a _row and \a _col.
  *
  */

void Cell::moveAbsolute(CellAddress newAddress)
{
    address = newAddress;
}

/**
  * Restore cell contents from \a reader.
  *
  */

void Cell::restore(Base::XMLReader &reader, bool checkAlias)
{
    const char* style = reader.hasAttribute("style") ? reader.getAttribute("style") : 0;
    const char* alignment = reader.hasAttribute("alignment") ? reader.getAttribute("alignment") : 0;
    const char* content = reader.hasAttribute("content") ? reader.getAttribute("content") : "";
    const char* foregroundColor = reader.hasAttribute("foregroundColor") ? reader.getAttribute("foregroundColor") : 0;
    const char* backgroundColor = reader.hasAttribute("backgroundColor") ? reader.getAttribute("backgroundColor") : 0;
    const char* displayUnit = reader.hasAttribute("displayUnit") ? reader.getAttribute("displayUnit") : 0;
    const char* alias = reader.hasAttribute("alias") ? reader.getAttribute("alias") : 0;
    const char* rowSpan = reader.hasAttribute("rowSpan") ? reader.getAttribute("rowSpan") : 0;
    const char* colSpan = reader.hasAttribute("colSpan") ? reader.getAttribute("colSpan") : 0;
    int mode = reader.hasAttribute("editMode")?reader.getAttributeAsInteger("editMode"):(int)EditNormal;


    // Don't trigger multiple updates below; wait until everything is loaded by calling unfreeze() below.
    PropertySheet::AtomicPropertyChange signaller(*owner);

    if (content) {
        setContent(content);
    }
    if (style) {
        using namespace boost;
        std::set<std::string> styleSet;

        escaped_list_separator<char> e('\0', '|', '\0');
        std::string line = std::string(style);
        tokenizer<escaped_list_separator<char> > tok(line, e);

        for(tokenizer<escaped_list_separator<char> >::iterator i = tok.begin(); i != tok.end();++i)
            styleSet.insert(*i);
        setStyle(styleSet);
    }
    if (alignment) {
        int alignmentCode = 0;
        using namespace boost;

        escaped_list_separator<char> e('\0', '|', '\0');
        std::string line = std::string(alignment);
        tokenizer<escaped_list_separator<char> > tok(line, e);

        for(tokenizer<escaped_list_separator<char> >::iterator i = tok.begin(); i != tok.end();++i)
            alignmentCode = decodeAlignment(*i, alignmentCode);

        setAlignment(alignmentCode);
    }
    if (foregroundColor) {
        App::Color color = decodeColor(foregroundColor, App::Color(0, 0, 0, 1));

        setForeground(color);
    }
    if (backgroundColor) {
        App::Color color = decodeColor(backgroundColor, App::Color(1, 1, 1, 1));

        setBackground(color);
    }
    if (displayUnit)
        setDisplayUnit(displayUnit);
    if (alias && (!checkAlias || !owner->revAliasProp.count(alias)))
        setAlias(alias);

    if (rowSpan || colSpan) {
        int rs = rowSpan ? atoi(rowSpan) : 1;
        int cs = colSpan ? atoi(colSpan) : 1;

        setSpans(rs, cs);
    }

    editMode = (EditMode)mode;
}

/**
  * Save cell contents into \a writer.
  *
  */

void Cell::save(Base::Writer &writer) const {
    save(writer.Stream(),writer.ind(),false);
}

void Cell::save(std::ostream &os, const char *indent, bool noContent) const {
    if (!isUsed())
        return;

    os << indent << "<Cell ";

    if (!noContent) {
        os << "address=\"" << address.toString() << "\" ";

        if(isUsed(EXPRESSION_SET)) {
            std::string content;
            getStringContent(content,true);
            os << "content=\"" << App::Property::encodeAttribute(content) << "\" ";
        }
    }

    if (isUsed(ALIGNMENT_SET))
        os << "alignment=\"" << encodeAlignment(alignment) << "\" ";

    if (isUsed(STYLE_SET))
        os << "style=\"" << encodeStyle(style) << "\" ";

    if (isUsed(FOREGROUND_COLOR_SET))
        os << "foregroundColor=\"" << encodeColor(foregroundColor) << "\" ";

    if (isUsed(BACKGROUND_COLOR_SET))
        os << "backgroundColor=\"" << encodeColor(backgroundColor) << "\" ";

    if (isUsed(DISPLAY_UNIT_SET))
        os << "displayUnit=\"" << App::Property::encodeAttribute(displayUnit.stringRep) << "\" ";

    if (isUsed(ALIAS_SET))
        os << "alias=\"" << App::Property::encodeAttribute(alias) << "\" ";

    if (isUsed(SPANS_SET)) {
        os << "rowSpan=\"" << rowSpan<< "\" ";
        os << "colSpan=\"" << colSpan << "\" ";
    }

    if(editMode) 
        os << "editMode=\"" << editMode << "\" ";

    os << "/>";
    if(!noContent)
        os << std::endl;
}

/**
  * Update the \a used member variable with mask (bitwise or'ed).
  *
  */

void Cell::setUsed(int mask, bool state)
{
    if (state)
        used |= mask;
    else
        used &= ~mask;

    if(owner)
        owner->setDirty(address);
}

/**
  * Determine whether the bits in \a mask are set in the \a used member variable.
  *
  */

bool Cell::isUsed(int mask) const
{
    return (used & mask) == mask;
}

/**
  * Determine if the any of the contents of the cell is set a non-default value.
  *
  */

bool Cell::isUsed() const
{
    return used != 0;
}

void Cell::visit(App::ExpressionVisitor &v)
{
    if (expression)
        expression->visit(v);
}

/**
  * Decode alignment into its internal value.
  *
  * @param itemStr   Alignment as a string
  * @param alignment Current alignment. This is or'ed with the one in \a itemStr.
  *
  * @returns New alignment.
  *
  */

int Cell::decodeAlignment(const std::string & itemStr, int alignment)
{
    if (itemStr == "himplied") {
        if(!(alignment & ALIGNMENT_HORIZONTAL))
            alignment |= ALIGNMENT_LEFT;
        alignment |= Cell::ALIGNMENT_HIMPLIED;
    } else if (itemStr == "left")
        alignment = (alignment & ~Cell::ALIGNMENT_HORIZONTAL) | Cell::ALIGNMENT_LEFT;
    else if (itemStr == "center")
        alignment = (alignment & ~Cell::ALIGNMENT_HORIZONTAL) | Cell::ALIGNMENT_HCENTER;
    else if (itemStr == "right")
        alignment = (alignment & ~Cell::ALIGNMENT_HORIZONTAL) | Cell::ALIGNMENT_RIGHT;
    else if (itemStr == "vimplied") {
        if(!(alignment & ALIGNMENT_VERTICAL))
            alignment |= ALIGNMENT_VCENTER;
        alignment |= Cell::ALIGNMENT_VIMPLIED;
    } else if (itemStr == "top")
        alignment = (alignment & ~Cell::ALIGNMENT_VERTICAL) | Cell::ALIGNMENT_TOP;
    else if (itemStr == "vcenter")
        alignment = (alignment & ~Cell::ALIGNMENT_VERTICAL) | Cell::ALIGNMENT_VCENTER;
    else if (itemStr == "bottom")
        alignment = (alignment & ~Cell::ALIGNMENT_VERTICAL) | Cell::ALIGNMENT_BOTTOM;
    else if(itemStr.size())
        throw Base::ValueError("Invalid alignment.");

    return alignment;
}

/**
  * Encode internal alignment value as a string.
  *
  * @param alignment Alignment as a binary value.
  *
  * @returns Alignment represented as a string.
  *
  */

std::string Cell::encodeAlignment(int alignment)
{
    std::string s;

    if (alignment & Cell::ALIGNMENT_LEFT)
        s += "left";
    if (alignment & Cell::ALIGNMENT_HCENTER)
        s += "center";
    if (alignment & Cell::ALIGNMENT_RIGHT)
        s += "right";
    if (alignment & Cell::ALIGNMENT_HIMPLIED)
        s += "|himplied";

    if (alignment & Cell::ALIGNMENT_VERTICAL)
        s += "|";

    if (alignment & Cell::ALIGNMENT_TOP)
        s += "top";
    if (alignment & Cell::ALIGNMENT_VCENTER)
        s += "vcenter";
    if (alignment & Cell::ALIGNMENT_BOTTOM)
        s += "bottom";
    if (alignment & Cell::ALIGNMENT_VIMPLIED)
        s += "|vimplied";

    return s;
}

/**
  * Encode \a color as a \#rrggbbaa string.
  *
  * @param color Color to encode.
  *
  * @returns String with encoded color.
  *
  */

std::string Cell::encodeColor(const App::Color & color)
{
    std::stringstream tmp;

    tmp << "#"
        << std::hex << std::setw(2) << std::setfill('0') << int(color.r * 255.0)
        << std::hex << std::setw(2) << std::setfill('0') << int(color.g * 255.0)
        << std::hex << std::setw(2) << std::setfill('0') << int(color.b * 255.0)
        << std::hex << std::setw(2) << std::setfill('0') << int(color.a * 255.0);

    return tmp.str();
}

/**
  * Encode set of styles as a string.
  *
  * @param style Set of string describing the style.
  *
  * @returns Set encoded as a string.
  *
  */

std::string Cell::encodeStyle(const std::set<std::string> & style)
{
    std::string s;
    std::set<std::string>::const_iterator j = style.begin();
    std::set<std::string>::const_iterator j_end = style.end();

    while (j != j_end) {
        s += *j;
        ++j;
        if (j != j_end)
            s += "|";
    }

    return s;
}

/**
  * Decode a string of the format \#rrggbb or \#rrggbbaa into a Color.
  *
  * @param color        The color to decode.
  * @param defaultColor A default color in case the decoding fails.
  *
  * @returns Decoded color.
  *
  */

App::Color Cell::decodeColor(const std::string & color, const App::Color & defaultColor)
{
    if (color.size() == 7 || color.size() == 9) {
        App::Color c;

        if (color[0] != '#')
            return defaultColor;
        unsigned int value = strtoul(color.c_str() + 1, 0, 16);

        if (color.size() == 7)
            value = (value << 8) | 0xff;

        c.setPackedValue(value);
        return c;
    }
    else
        return defaultColor;
}

Cell::EditMode Cell::getEditMode() const {
    return hasException()?EditNormal:editMode;
}

void Cell::setEditData(const char *data) {
    if(hasException() && editMode!=EditNormal)
        FC_THROWM(Base::ExpressionError,exceptionStr);

    switch(editMode) {
    case EditButton: {
        if(!owner || !owner->getContainer()) 
            FC_THROWM(Base::RuntimeError,"Invalid cell '" << address.toString() << "'");
        auto prop = dynamic_cast<App::PropertyPythonObject*>(
                owner->getContainer()->getPropertyByName(address.toString().c_str()));
        if(prop) {
            Base::PyGILStateLocker lock;
            Py::Object obj(static_cast<App::PropertyPythonObject*>(prop)->getPyObject(),true);
            if(obj.isCallable()) {
                try {
                    Py::Callable(obj).apply();
                }catch(Py::Exception &) {
                    Base::PyException::ThrowException();
                }
                return;
            }
        }
        FC_THROWM(Base::TypeError,"Expects the cell '" << address.toString() << 
                "' evaluates to a Python callable");
        break;
    }
    case EditCombo: {
        if(!data || !data[0])
            FC_THROWM(Base::ValueError,"invalid value");
        auto expr = SimpleStatement::cast<ListExpression>(expression.get());
        if(expr && expr->getSize()>=2) {
            App::ExpressionPtr e(expr->copy());
            static_cast<App::ListExpression&>(*e).setItem(1,
                    App::StringExpression::create(
                        dynamic_cast<App::DocumentObject*>(owner->getContainer()),data));
            setExpression(std::move(e));
            if(expr->getSize()>=3) {
                Base::PyGILStateLocker lock;
                try {
                    Py::Sequence seq(App::pyObjectFromAny(expression->getValueAsAny()));
                    Py::Object item(seq[2]);
                    if(item.isCallable()) {
                        Py::Tuple args(2);
                        args.setItem(0,Py::String(address.toString()));
                        args.setItem(1,seq);
                        Py::Callable(item).apply(args);
                    }
                }catch(Py::Exception &) {
                    Base::PyException::ThrowException();
                }
            }
            return;
        }
        FC_THROWM(Base::TypeError,"Expects the cell '" << address.toString() << 
                "' contains a list expression of [mapping,obj]");
        break;
    }
    case EditLabel: {
        if(!data)
            FC_THROWM(Base::ValueError,"invalid value");
        auto expr = SimpleStatement::cast<App::ListExpression>(expression.get());
        if(expr && expr->getSize()>=1) {
            App::ExpressionPtr e(expr->copy());
            static_cast<App::ListExpression&>(*e).setItem(0,
                    App::StringExpression::create(
                        dynamic_cast<App::DocumentObject*>(owner->getContainer()),data));
            setExpression(std::move(e));
            return;
        }
        FC_THROWM(Base::TypeError,"Expects the cell '" << address.toString() << 
                "' to be a list expression of [string...]");
        break;
    }
    default:
        setContent(data);
        break;
    }
}

std::vector<std::string> Cell::getEditData(bool silent) const {
    std::vector<std::string> res;
    switch(getEditMode()) {
    case EditButton: {
        if(!owner || !owner->getContainer()) {
            if(silent) break;
            FC_THROWM(Base::RuntimeError,"Invalid cell '" << address.toString() << "'");
        }
        auto prop = owner->getContainer()->getPropertyByName(address.toString().c_str());
        if(prop && prop->isDerivedFrom(App::PropertyPythonObject::getClassTypeId())) {
            Base::PyGILStateLocker lock;
            Py::Object obj(static_cast<App::PropertyPythonObject*>(prop)->getPyObject(),true);
            std::string title;
            if(obj.hasAttr("__doc__"))
                title = obj.getAttr("__doc__").as_string();
            if(title.size())
                res.push_back(std::move(title));
            else if(isUsed(ALIAS_SET))
                res.push_back(alias);
            else
                res.push_back(address.toString());
            return res;
        }
        if(!silent)
            FC_THROWM(Base::TypeError,"Expects the cell '" << address.toString() << 
                    "' evaluates to a Python callable");
        break;
    }
    case EditCombo: 
    case EditLabel: {
        if(!owner || !owner->getContainer()) {
            if(silent) break;
            FC_THROWM(Base::RuntimeError,"Invalid cell '" << address.toString() << "'");
        }
        auto prop = owner->getContainer()->getPropertyByName(address.toString().c_str());
        if(prop && prop->isDerivedFrom(App::PropertyPythonObject::getClassTypeId())) {
            Base::PyGILStateLocker lock;
            Py::Object obj(static_cast<App::PropertyPythonObject*>(prop)->getPyObject(),true);
            if(obj.isSequence()) {
                Py::Sequence seq(obj);
                if(editMode == EditLabel) {
                    if(seq.size()>=1 && seq[0].isString()) {
                        res.push_back(Py::Object(seq[0].ptr()).as_string());
                        return res;
                    }
                }else if(seq.size()>=2 && seq[0].isMapping()) {
                    res.push_back(Py::Object(seq[1].ptr()).as_string());
                    Py::Mapping map(seq[0].ptr());
                    for(auto it=map.begin();it!=map.end();++it) {
                        const auto &value = *it;
                        res.push_back(value.first.as_string());
                    }
                    return res;
                }
            }
        }
        if(!silent)
            FC_THROWM(Base::TypeError,"Expects the cell '" << address.toString() << 
                    "' contains a list expression of [mapping,obj]");
        break;
    }
    default: {
        std::string s;
        if(getStringContent(s))
            res.push_back(std::move(s));
    }}
    return res;
}

void Cell::setEditMode(EditMode mode) {
    if(editMode == mode) 
        return;

    if(mode!=Cell::EditNormal) {
        if(!owner || !owner->getContainer()) 
            FC_THROWM(Base::RuntimeError,"Invalid cell '" << address.toString() << "'");
        auto prop = owner->getContainer()->getPropertyByName(address.toString().c_str());
        if(!prop || !prop->isDerivedFrom(App::PropertyPythonObject::getClassTypeId()))
            FC_THROWM(Base::TypeError,"Expects the cell '" << address.toString() << 
                    "' evaluates to a Python object");

        Base::PyGILStateLocker lock;
        Py::Object obj(static_cast<App::PropertyPythonObject*>(prop)->getPyObject(),true);
        if(mode == Cell::EditButton) {
            if(!obj.isCallable()) {
                FC_THROWM(Base::TypeError,"Expects the cell '" << address.toString() << 
                        "' evaluates to a Python callable");
            }
        }else if(mode == Cell::EditCombo){
            bool valid = false;
            auto expr = SimpleStatement::cast<ListExpression>(expression.get());
            if(expr && expr->getSize()>=2) {
                Py::Sequence seq(obj);
                valid = seq.size()>=2 && seq[0].isMapping() && seq[1].isString();
            }
            if(!valid)
                FC_THROWM(Base::TypeError,"Expects the cell '" << address.toString() << 
                        "' contains a list expression of [mapping,string]");
        }else if(mode == Cell::EditLabel){
            bool valid = false;
            auto expr = SimpleStatement::cast<ListExpression>(expression.get());
            if(expr && expr->getSize()>=1) {
                Py::Sequence seq(obj);
                valid = seq.size()>=1 && seq[0].isString();
            }
            if(!valid)
                FC_THROWM(Base::TypeError,"Expects the cell '" << address.toString() << 
                        "' contains a list expression [string...]");
        }else
            FC_THROWM(Base::ValueError,"Unknown edit mode");
    }

    PropertySheet::AtomicPropertyChange signaler(*owner);
    editMode = mode;
    signaler.tryInvoke();
}
