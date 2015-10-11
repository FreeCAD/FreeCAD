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

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/assign.hpp>
#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Property.h>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/PyObjectBase.h>
#include "PropertySheet.h"
#include "Sheet.h"
#include "SpreadsheetExpression.h"
#include "Utils.h"
#include <PropertySheetPy.h>

using namespace App;
using namespace Base;
using namespace Spreadsheet;

namespace Spreadsheet {

class RelabelDocumentObjectExpressionVisitor : public ExpressionVisitor {
public:

    RelabelDocumentObjectExpressionVisitor(const std::string & _oldName, const std::string & _newName)
        : oldName(_oldName)
        , newName(_newName)
    {
    }

    void visit(Expression * node) {
        VariableExpression *expr = freecad_dynamic_cast<VariableExpression>(node);

        if (expr)
            expr->renameDocumentObject(oldName, newName);
    }

private:
    std::string oldName;
    std::string newName;
};

class BuildDocDepsExpressionVisitor : public ExpressionVisitor {
public:

    BuildDocDepsExpressionVisitor(std::set<App::DocumentObject*> & _docDeps)
        : docDeps(_docDeps)
    {

    }

    void visit(Expression * node) {
        VariableExpression *expr = freecad_dynamic_cast<VariableExpression>(node);

        if (expr) {
            try {
                const App::Property * prop = expr->getProperty();
                App::DocumentObject * docObj = freecad_dynamic_cast<App::DocumentObject>(prop->getContainer());

                if (docObj)
                    docDeps.insert(docObj);
            }
            catch (const Base::Exception &) {
                // Ignore this type of exception; it means that the property was not found, which is ok here
            }
        }
    }

private:
    std::set<App::DocumentObject*> & docDeps;
};

class RelabelDocumentExpressionVisitor : public ExpressionVisitor {
public:

    RelabelDocumentExpressionVisitor(const std::string & _oldName, const std::string & _newName)
        : oldName(_oldName)
        , newName(_newName)
    {
    }

    void visit(Expression * node) {
        VariableExpression *expr = freecad_dynamic_cast<VariableExpression>(node);

        if (expr)
            expr->renameDocument(oldName, newName);
    }

private:
    std::string oldName;
    std::string newName;
};

}

TYPESYSTEM_SOURCE(Spreadsheet::PropertySheet , App::Property);

void PropertySheet::clear()
{
    std::map<CellAddress, Cell* >::iterator i = data.begin();

    /* Clear cells */
    while (i != data.end()) {
        delete i->second;
        setDirty(i->first);
        ++i;
    }

    /* Clear from map */
    data.clear();

    mergedCells.clear();

    propertyNameToCellMap.clear();
    documentObjectToCellMap.clear();
    docDeps.clear();
    aliasProp.clear();
    revAliasProp.clear();
}

Cell *PropertySheet::getValue(CellAddress key)
{
    std::map<CellAddress, Cell*>::const_iterator i = data.find(key);

    if (i == data.end())
        return 0;
    else
        return i->second;
}

std::set<CellAddress> PropertySheet::getUsedCells() const
{
    std::set<CellAddress> usedSet;

    for (std::map<CellAddress, Cell*>::const_iterator i = data.begin(); i != data.end(); ++i) {
        if (i->second->isUsed())
            usedSet.insert(i->first);
    }

    return usedSet;
}

void PropertySheet::setDirty(CellAddress address)
{
    /* Merged cells will automatically force an update of the top left cell
       to be consistent. */
    std::map<CellAddress, CellAddress>::const_iterator i = mergedCells.find(address);
    if (i != mergedCells.end())
        address = i->second;

    dirty.insert(address);
}

Cell * PropertySheet::createCell(CellAddress address)
{
    Cell * cell = new Cell(address, this);

    data[address] = cell;

    return cell;
}

PropertySheet::PropertySheet(Sheet *_owner)
    : Property()
    , owner(_owner)
    , signalCounter(0)
{
}

PropertySheet::PropertySheet(const PropertySheet &other)
    : dirty(other.dirty)
    , mergedCells(other.mergedCells)
    , owner(other.owner)
    , propertyNameToCellMap(other.propertyNameToCellMap)
    , documentObjectToCellMap(other.documentObjectToCellMap)
    , signalCounter(0)
{
    std::map<CellAddress, Cell* >::const_iterator i = other.data.begin();

    /* Copy cells */
    while (i != other.data.end()) {
        data[i->first] = new Cell(*i->second);
        ++i;
    }
}

PropertySheet::~PropertySheet()
{
    clear();
}

App::Property *PropertySheet::Copy(void) const
{
    return new PropertySheet(*this);
}

void PropertySheet::Paste(const Property &from)
{
    Signaller signaller(*this);

    const PropertySheet * froms = static_cast<const PropertySheet*>(&from);

    std::map<CellAddress, Cell* >::iterator icurr = data.begin();

    /* Mark all first */
    while (icurr != data.end()) {
        icurr->second->mark();
        ++icurr;
    }

    std::map<CellAddress, Cell* >::const_iterator ifrom = froms->data.begin();
    while (ifrom != froms->data.end()) {
        std::map<CellAddress, Cell* >::iterator i = data.find(ifrom->first);

        if (i != data.end()) {
            *(data[ifrom->first]) = *(ifrom->second); // Exists; assign cell directly
            recomputeDependencies(ifrom->first);
        }
        else {
            data[ifrom->first] = new Cell(*(ifrom->second)); // Doesn't exist, copy using Cell's copy constructor
        }

        /* Set dirty */
        setDirty(ifrom->first);

        ++ifrom;
    }

    /* Remove all that are still marked */
    icurr = data.begin();
    while (icurr != data.end()) {
        Cell * cell = icurr->second;

        if (cell->isMarked())
            clear(ifrom->first);
        ++icurr;
    }

    mergedCells = froms->mergedCells;
}

void PropertySheet::Save(Base::Writer &writer) const
{
    // Save cell contents
    int count = 0;

    std::map<CellAddress, Cell*>::const_iterator ci = data.begin();
    while (ci != data.end()) {
        if (ci->second->isUsed())
            ++count;
        ++ci;
    }

    writer.Stream() << writer.ind() << "<Cells Count=\"" << count << "\">" << std::endl;
    writer.incInd();
    ci = data.begin();
    while (ci != data.end()) {
        ci->second->save(writer);
        ++ci;
    }

    writer.decInd();
    writer.Stream() << writer.ind() << "</Cells>" << std::endl;
}

void PropertySheet::Restore(Base::XMLReader &reader)
{
    int Cnt;

    reader.readElement("Cells");
    Cnt = reader.getAttributeAsInteger("Count");
    for (int i = 0; i < Cnt; i++) {
        reader.readElement("Cell");

        const char* strAddress = reader.hasAttribute("address") ? reader.getAttribute("address") : 0;

        try {
            CellAddress address(strAddress);
            Cell * cell = createCell(address);

            cell->restore(reader);

            int rows, cols;
            if (cell->getSpans(rows, cols) && (rows > 1 || cols > 1)) {
                mergeCells(address, CellAddress(address.row() + rows - 1, address.col() + cols - 1));
            }
        }
        catch (const Base::Exception &) {
            // Something is wrong, skip this cell
        }
        catch (...) {
        }
    }
    reader.readEndElement("Cells");
}

Cell * PropertySheet::cellAt(CellAddress address)
{
    std::map<CellAddress, CellAddress>::const_iterator j = mergedCells.find(address);

    // address actually inside a merged cell
    if (j != mergedCells.end()) {
        std::map<CellAddress, Cell*>::const_iterator i = data.find(j->second);
        assert(i != data.end());

        return i->second;
    }

    std::map<CellAddress, Cell*>::const_iterator i = data.find(address);

    if (i == data.end())
        return 0;
    else
        return i->second;
}

const Cell * PropertySheet::cellAt(CellAddress address) const
{
    std::map<CellAddress, CellAddress>::const_iterator j = mergedCells.find(address);

    // address actually inside a merged cell
    if (j != mergedCells.end()) {
        std::map<CellAddress, Cell*>::const_iterator i = data.find(j->second);
        assert(i != data.end());

        return i->second;
    }

    std::map<CellAddress, Cell*>::const_iterator i = data.find(address);

    if (i == data.end())
        return 0;
    else
        return i->second;
}

Cell * PropertySheet::nonNullCellAt(CellAddress address)
{
    std::map<CellAddress, CellAddress>::const_iterator j = mergedCells.find(address);

    if (j != mergedCells.end()) {
        std::map<CellAddress, Cell*>::const_iterator i = data.find(j->second);

        if (i == data.end())
            return createCell(address);
        else
            return i->second;
    }

    std::map<CellAddress, Cell*>::const_iterator i = data.find(address);

    if (i == data.end())
        return createCell(address);
    else
        return i->second;
}

void PropertySheet::setContent(CellAddress address, const char *value)
{
    Cell * cell = nonNullCellAt(address);

    assert(cell != 0);

    cell->setContent(value);
}

void PropertySheet::setAlignment(CellAddress address, int _alignment)
{
    nonNullCellAt(address)->setAlignment(_alignment);
}

void PropertySheet::setStyle(CellAddress address, const std::set<std::string> &_style)
{
    assert(nonNullCellAt(address) != 0);
    nonNullCellAt(address)->setStyle(_style);
}

void PropertySheet::setForeground(CellAddress address, const App::Color &color)
{
    assert(nonNullCellAt(address) != 0);
    nonNullCellAt(address)->setForeground(color);
}

void PropertySheet::setBackground(CellAddress address, const App::Color &color)
{
    assert(nonNullCellAt(address) != 0);
    nonNullCellAt(address)->setBackground(color);
}

void PropertySheet::setDisplayUnit(CellAddress address, const std::string &unit)
{
    assert(nonNullCellAt(address) != 0);
    nonNullCellAt(address)->setDisplayUnit(unit);
}


void PropertySheet::setAlias(CellAddress address, const std::string &alias)
{
    Cell * cell = nonNullCellAt(address);
    assert(cell != 0);

    /* Mark cells depending on this cell dirty; they need to be resolved when an alias changes or disappears */
    const char * docName = owner->getDocument()->Label.getValue();
    const char * docObjName = owner->getNameInDocument();
    std::string fullName = std::string(docName) + "#" + std::string(docObjName) + "." + address.toString();

    std::map<std::string, std::set< CellAddress > >::const_iterator j = propertyNameToCellMap.find(fullName);
    if (j != propertyNameToCellMap.end()) {
        std::set< CellAddress >::const_iterator k = j->second.begin();

        while (k != j->second.end()) {
            setDirty(*k);
            ++k;
        }
    }

    std::string oldAlias;

    if (cell->getAlias(oldAlias))
        owner->aliasRemoved(address, oldAlias);

    cell->setAlias(alias);

    if (oldAlias.size() > 0) {
        std::map<App::ObjectIdentifier, App::ObjectIdentifier> m;

        m[App::ObjectIdentifier(owner, oldAlias)] = App::ObjectIdentifier(owner, alias);

        owner->getDocument()->renameObjectIdentifiers(m);
    }

}

void PropertySheet::setComputedUnit(CellAddress address, const Base::Unit &unit)
{
    assert(nonNullCellAt(address) != 0);
    nonNullCellAt(address)->setComputedUnit(unit);
}

void PropertySheet::setSpans(CellAddress address, int rows, int columns)
{
    assert(nonNullCellAt(address) != 0);
    nonNullCellAt(address)->setSpans(rows, columns);
}

void PropertySheet::clear(CellAddress address)
{
    std::map<CellAddress, Cell*>::iterator i = data.find(address);

    if (i == data.end())
        return;

    Signaller signaller(*this);

    // Spit cell to clean up mergeCells map; all data is in first cell anyway
    splitCell(address);

    // Delete Cell object
    removeDependencies(address);
    delete i->second;

    // Mark as dirty
    dirty.insert(i->first);

    // Remove alias if it exists
    std::map<CellAddress, std::string>::iterator j = aliasProp.find(address);
    if (j != aliasProp.end()) {
        revAliasProp.erase(j->second);
        aliasProp.erase(j);
    }

    // Erase from internal struct
    data.erase(i);

    rebuildDocDepList();
}

void PropertySheet::moveCell(CellAddress currPos, CellAddress newPos)
{
    std::map<CellAddress, Cell*>::const_iterator i = data.find(currPos);
    std::map<CellAddress, Cell*>::const_iterator j = data.find(newPos);

    Signaller signaller(*this);

    if (j != data.end())
        clear(newPos);

    if (i != data.end()) {
        Cell * cell = i->second;

        // Remove from old
        removeDependencies(currPos);
        data.erase(currPos);
        setDirty(currPos);

        // Insert into new spot
        cell->moveAbsolute(newPos);
        data[newPos] = cell;
        addDependencies(newPos);
        setDirty(newPos);

        rebuildDocDepList();
    }
}

/**
 * @brief The RewriteExpressionVisitor class
 *
 * A class that visits each node of an expressions, and possibly
 * rewrites variables. This is a helper class to rewrite expressions
 * when rows or columns are either inserted or removed, to make
 * sure that formulas referencing cells being moved to a new locations
 * will still be valid, i.e rewritten.
 *
 */

class RewriteExpressionVisitor : public ExpressionVisitor {
public:
    RewriteExpressionVisitor(CellAddress address, int rowCount, int colCount)
        : mRow(address.row())
        , mCol(address.col())
        , mRowCount(rowCount)
        , mColCount(colCount)
        , mChanged(false) { }
    ~RewriteExpressionVisitor() { }

    void reset() { mChanged = false; }

    bool changed() const { return mChanged; }

    void visit(Expression * node) {
        VariableExpression *varExpr = freecad_dynamic_cast<VariableExpression>(node);
        RangeExpression *rangeExpr = freecad_dynamic_cast<RangeExpression>(node);


        if (varExpr) {
            static const boost::regex e("(\\${0,1})([A-Za-z]+)(\\${0,1})([0-9]+)");
            boost::cmatch cm;

            if (boost::regex_match(varExpr->name().c_str(), cm, e)) {
                const boost::sub_match<const char *> colstr = cm[2];
                const boost::sub_match<const char *> rowstr = cm[4];
                int thisRow, thisCol;

                thisCol = decodeColumn(colstr.str());
                thisRow = decodeRow(rowstr.str());

                if (thisRow >= mRow || thisCol >= mCol) {
                    thisRow += mRowCount;
                    thisCol += mColCount;
                    varExpr->setName(columnName(thisCol) + rowName(thisRow));
                    mChanged = true;
                }
            }
        }
        else if (rangeExpr) {
            Range r = rangeExpr->getRange();
            CellAddress from(r.from());
            CellAddress to(r.to());

            if (from.row() >= mRow || from.col() >= mCol) {
                from = CellAddress(from.row() + mRowCount, from.col() + mColCount);
                mChanged = true;
            }
            if (to.row() >= mRow || to.col() >= mCol) {
                to = CellAddress(to.row() + mRowCount, to.col() + mColCount);
                mChanged = true;
            }
            rangeExpr->setRange(Range(from, to));
        }
    }
private:
    int mRow;
    int mCol;
    int mRowCount;
    int mColCount;
    bool mChanged;
};

void PropertySheet::insertRows(int row, int count)
{
    std::vector<CellAddress> keys;

    /* Copy all keys from cells map */
    boost::copy( data | boost::adaptors::map_keys, std::back_inserter(keys));

    /* Sort them */
    std::sort(keys.begin(), keys.end(), boost::bind(&PropertySheet::rowSortFunc, this, _1, _2));

    RewriteExpressionVisitor visitor(CellAddress(row, CellAddress::MAX_COLUMNS), count, 0);

    Signaller signaller(*this);
    for (std::vector<CellAddress>::const_reverse_iterator i = keys.rbegin(); i != keys.rend(); ++i) {
        std::map<CellAddress, Cell*>::iterator j = data.find(*i);

        assert(j != data.end());

        Cell * cell = j->second;

        // Visit each cell to make changes to expressions if necessary
        visitor.reset();
        cell->visit(visitor);
        if (visitor.changed()) {
            setDirty(*i);
            recomputeDependencies(*i);
        }

        if (i->row() >= row)
            moveCell(*i, CellAddress(i->row() + count, i->col()));
    }
}

/**
  * Sort function to sort two cell positions according to their row position.
  *
  */

bool PropertySheet::rowSortFunc(const CellAddress & a, const CellAddress & b) {
    if (a.row() < b.row())
        return true;
    else
        return false;
}

void PropertySheet::removeRows(int row, int count)
{
    std::vector<CellAddress> keys;

    /* Copy all keys from cells map */
    boost::copy(data | boost::adaptors::map_keys, std::back_inserter(keys));

    /* Sort them */
    std::sort(keys.begin(), keys.end(), boost::bind(&PropertySheet::rowSortFunc, this, _1, _2));

    RewriteExpressionVisitor visitor(CellAddress(row + count, CellAddress::MAX_COLUMNS), -count, 0);

    Signaller signaller(*this);
    for (std::vector<CellAddress>::const_iterator i = keys.begin(); i != keys.end(); ++i) {
        std::map<CellAddress, Cell*>::iterator j = data.find(*i);

        assert(j != data.end());

        Cell * cell = j->second;

        // Visit each cell to make changes to expressions if necessary
        visitor.reset();
        cell->visit(visitor);
        if (visitor.changed()) {
            setDirty(*i);
            recomputeDependencies(*i);
        }

        if (i->row() >= row && i->row() < row + count)
            clear(*i);
        else if (i->row() >= row + count)
            moveCell(*i, CellAddress(i->row() - count, i->col()));
    }
}

void PropertySheet::insertColumns(int col, int count)
{

    std::vector<CellAddress> keys;

    /* Copy all keys from cells map */
    boost::copy(data | boost::adaptors::map_keys, std::back_inserter(keys));

    /* Sort them */
    std::sort(keys.begin(), keys.end());

    RewriteExpressionVisitor visitor(CellAddress(CellAddress::MAX_ROWS, col), 0, count);

    Signaller signaller(*this);
    for (std::vector<CellAddress>::const_reverse_iterator i = keys.rbegin(); i != keys.rend(); ++i) {
        std::map<CellAddress, Cell*>::iterator j = data.find(*i);

        assert(j != data.end());

        Cell * cell = j->second;

        // Visit each cell to make changes to expressions if necessary
        visitor.reset();
        cell->visit(visitor);
        if (visitor.changed()) {
            setDirty(*i);
            recomputeDependencies(*i);
        }

        if (i->col() >= col)
            moveCell(*i, CellAddress(i->row(), i->col() + count));
    }
}

/**
  * Sort function to sort two cell positions according to their column position.
  *
  */

bool PropertySheet::colSortFunc(const CellAddress & a, const CellAddress & b) {
    if (a.col() < b.col())
        return true;
    else
        return false;
}

void PropertySheet::removeColumns(int col, int count)
{
    std::vector<CellAddress> keys;

    /* Copy all keys from cells map */
    boost::copy(data | boost::adaptors::map_keys, std::back_inserter(keys));

    /* Sort them */
    std::sort(keys.begin(), keys.end(), boost::bind(&PropertySheet::colSortFunc, this, _1, _2));

    RewriteExpressionVisitor visitor(CellAddress(CellAddress::MAX_ROWS, col + count), 0, -count);

    Signaller signaller(*this);
    for (std::vector<CellAddress>::const_iterator i = keys.begin(); i != keys.end(); ++i) {
        std::map<CellAddress, Cell*>::iterator j = data.find(*i);

        assert(j != data.end());

        Cell * cell = j->second;

        // Visit each cell to make changes to expressions if necessary
        visitor.reset();
        cell->visit(visitor);
        if (visitor.changed()) {
            setDirty(*i);
            recomputeDependencies(*i);
        }

        if (i->col() >= col && i->col() < col + count)
            clear(*i);
        else if (i->col() >= col + count)
            moveCell(*i, CellAddress(i->row(), i->col() - count));
    }
}

unsigned int PropertySheet::getMemSize() const
{
    return sizeof(*this);
}


bool PropertySheet::mergeCells(CellAddress from, CellAddress to)
{
    // Check that this merge is not overlapping other merged cells
    for (int r = from.row(); r <= to.row(); ++r) {
        for (int c = from.col(); c <= to.col(); ++c) {
            if (mergedCells.find(CellAddress(r, c)) != mergedCells.end())
                return false;
        }
    }

    Signaller signaller(*this);

    // Clear cells that will be hidden by the merge
    for (int r = from.row(); r <= to.row(); ++r)
        for (int c = from.col(); c <= to.col(); ++c)
            if ( !(r == from.row() && c == from.col()) )
                clear(CellAddress(r, c));

    // Update internal structure to track merged cells
    for (int r = from.row(); r <= to.row(); ++r)
        for (int c = from.col(); c <= to.col(); ++c) {
            mergedCells[CellAddress(r, c)] = from;
            setDirty(CellAddress(r, c));
        }

    setSpans(from, to.row() - from.row() + 1, to.col() - from.col() + 1);

    return true;
}

void PropertySheet::splitCell(CellAddress address)
{
    int rows, cols;
    std::map<CellAddress, CellAddress>::const_iterator i = mergedCells.find(address);

    if (i == mergedCells.end())
        return;

    CellAddress anchor = i->second;
    Signaller signaller(*this);
    cellAt(anchor)->getSpans(rows, cols);

    for (int r = anchor.row(); r <= anchor.row() + rows; ++r)
        for (int c = anchor.col(); c <= anchor.col() + cols; ++c) {
            setDirty(CellAddress(r, c));
            mergedCells.erase(CellAddress(r, c));
        }

    setSpans(anchor, 1, 1);
}

void PropertySheet::getSpans(CellAddress address, int & rows, int & cols) const
{
    std::map<CellAddress, CellAddress>::const_iterator i = mergedCells.find(address);

    if (i != mergedCells.end()) {
        CellAddress anchor = i->second;

        cellAt(anchor)->getSpans(rows, cols);
    }
    else {
        rows = cols = 1;
    }
}

bool PropertySheet::isMergedCell(CellAddress address) const
{
    return mergedCells.find(address) != mergedCells.end();
}

bool PropertySheet::isHidden(CellAddress address) const
{
    std::map<CellAddress, CellAddress>::const_iterator i = mergedCells.find(address);

    return i != mergedCells.end() && i->second != address;
}

/**
  * Update dependencies of \a expression for cell at \a key.
  *
  * @param expression Expression to extract dependencies from
  * @param key        Address of cell containing the expression.
  */

void PropertySheet::addDependencies(CellAddress key)
{
    Cell * cell = getValue(key);

    if (!cell)
        return;

    cell->clearResolveException();

    const Expression * expression = cell->getExpression();

    if (expression == 0)
        return;

    std::set<ObjectIdentifier> expressionDeps;

    // Get dependencies from expression
    expression->getDeps(expressionDeps);

    std::set<ObjectIdentifier>::const_iterator i = expressionDeps.begin();
    while (i != expressionDeps.end()) {
        const Property * prop = i->getProperty();
        const App::DocumentObject * docObj = i->getDocumentObject();
        App::Document * doc = i->getDocument();

        std::string docName = doc ? doc->Label.getValue() : i->getDocumentName().getString();
        std::string docObjName = docName + "#" + (docObj ? docObj->getNameInDocument() : i->getDocumentObjectName().getString());
        std::string propName = docObjName + "." + i->getPropertyName();

        if (!prop)
            cell->setResolveException("Unresolved dependency");
        else {
            App::DocumentObject * docObject = freecad_dynamic_cast<App::DocumentObject>(prop->getContainer());

            documentObjectName[docObject] = docObject->Label.getValue();
            documentName[docObject->getDocument()] = docObject->getDocument()->Label.getValue();
        }

        // Observe document to trach changes to the property
        if (doc)
            owner->observeDocument(doc);

        // Insert into maps
        propertyNameToCellMap[propName].insert(key);
        cellToPropertyNameMap[key].insert(propName);

        // Also an alias?
        if (docObj == owner) {
            std::map<std::string, CellAddress>::const_iterator j = revAliasProp.find(i->getPropertyName());

            if (j != revAliasProp.end()) {
                propName = docObjName + "." + j->second.toString();

                // Insert into maps
                propertyNameToCellMap[propName].insert(key);
                cellToPropertyNameMap[key].insert(propName);
            }
        }

        documentObjectToCellMap[docObjName].insert(key);
        cellToDocumentObjectMap[key].insert(docObjName);

        ++i;
    }
}

/**
  * Remove dependecies given by \a expression for cell at \a key.
  *
  * @param expression Expression to extract dependencies from
  * @param key        Address of cell containing the expression
  *
  */

void PropertySheet::removeDependencies(CellAddress key)
{
    /* Remove from Property <-> Key maps */

    std::map<CellAddress, std::set< std::string > >::iterator i1 = cellToPropertyNameMap.find(key);

    if (i1 != cellToPropertyNameMap.end()) {
        std::set< std::string >::const_iterator j = i1->second.begin();

        while (j != i1->second.end()) {
            std::map<std::string, std::set< CellAddress > >::iterator k = propertyNameToCellMap.find(*j);

            assert(k != propertyNameToCellMap.end());

            k->second.erase(key);
            ++j;
        }

        cellToPropertyNameMap.erase(i1);
    }

    /* Remove from DocumentObject <-> Key maps */

    std::map<CellAddress, std::set< std::string > >::iterator i2 = cellToDocumentObjectMap.find(key);

    if (i2 != cellToDocumentObjectMap.end()) {
        std::set< std::string >::const_iterator j = i2->second.begin();

        while (j != i2->second.end()) {
            std::map<std::string, std::set< CellAddress > >::iterator k = documentObjectToCellMap.find(*j);

            assert(k != documentObjectToCellMap.end());

            k->second.erase(key);

            if (k->second.size() == 0)
                documentObjectToCellMap.erase(*j);

            ++j;
        }

        cellToDocumentObjectMap.erase(i2);
    }
}

/**
  * Recompute any cells that depend on \a prop.
  *
  * @param prop Property that presumably has changed an triggers updates of other cells.
  *
  */

void PropertySheet::recomputeDependants(const Property *prop)
{
    App::DocumentObject * owner = freecad_dynamic_cast<App::DocumentObject>(prop->getContainer());
    const char * name = owner->getPropertyName(prop);

    assert(name != 0);

    if (name) {
        const char * docName = owner->getDocument()->Label.getValue();
        const char * nameInDoc = owner->getNameInDocument();

        if (nameInDoc) {
            // Recompute cells that depend on this cell
            std::string fullName = std::string(docName) + "#" + std::string(nameInDoc) + "." + std::string(name);
            std::map<std::string, std::set< CellAddress > >::const_iterator i = propertyNameToCellMap.find(fullName);

            if (i == propertyNameToCellMap.end())
                return;

            std::set<CellAddress>::const_iterator j = i->second.begin();
            std::set<CellAddress>::const_iterator end = i->second.end();

            while (j != end) {
                setDirty(*j);
                ++j;
            }
        }
    }
}

void PropertySheet::invalidateDependants(const App::DocumentObject *docObj)
{
    const char * docName = docObj->getDocument()->Label.getValue();
    const char * docObjName = docObj->getNameInDocument();

    // Touch to force recompute
    touch();

    // Recompute cells that depend on this cell
    std::string fullName = std::string(docName) + "#" + std::string(docObjName);
    std::map<std::string, std::set< CellAddress > >::const_iterator i = documentObjectToCellMap.find(fullName);

    if (i == documentObjectToCellMap.end())
        return;

    std::set<CellAddress> s = i->second;
    std::set<CellAddress>::const_iterator j = s.begin();
    std::set<CellAddress>::const_iterator end = s.end();

    while (j != end) {
        Cell * cell = getValue(*j);

        cell->setResolveException("Unresolved dependency");
        setDirty((*j));
        ++j;
    }
}

void PropertySheet::renamedDocumentObject(const App::DocumentObject * docObj)
{
    // Touch to force recompute
    touch();

    if (documentObjectName.find(docObj) == documentObjectName.end())
        return;

    std::map<CellAddress, Cell* >::iterator i = data.begin();

    Signaller signaller(*this);
    RelabelDocumentObjectExpressionVisitor v(documentObjectName[docObj], docObj->Label.getValue());

    while (i != data.end()) {
        i->second->visit(v);
        recomputeDependencies(i->first);
        setDirty(i->first);
        ++i;
    }
}

void PropertySheet::renamedDocument(const App::Document * doc)
{
    // Touch to force recompute
    touch();

    if (documentName.find(doc) == documentName.end())
        return;

    std::map<CellAddress, Cell* >::iterator i = data.begin();

    /* Resolve all cells */
    Signaller signaller(*this);
    RelabelDocumentExpressionVisitor v(documentName[doc], doc->Label.getValue());

    while (i != data.end()) {
        i->second->visit(v);
        recomputeDependencies(i->first);
        setDirty(i->first);
        ++i;
    }
}

void PropertySheet::deletedDocumentObject(const App::DocumentObject *docObj)
{
    docDeps.erase(const_cast<App::DocumentObject*>(docObj));
}

void PropertySheet::documentSet()
{
    documentName[owner->getDocument()] = owner->getDocument()->Label.getValue();
}

void PropertySheet::recomputeDependants(const App::DocumentObject *docObj)
{
    const char * docName = docObj->getDocument()->Label.getValue();
    const char * docObjName = docObj->getNameInDocument();

    // Touch to force recompute
    touch();

    // Recompute cells that depend on this cell
    std::string fullName = std::string(docName) + "#" + std::string(docObjName);
    std::map<std::string, std::set< CellAddress > >::const_iterator i = documentObjectToCellMap.find(fullName);

    if (i == documentObjectToCellMap.end())
        return;

    std::set<CellAddress>::const_iterator j = i->second.begin();
    std::set<CellAddress>::const_iterator end = i->second.end();

    while (j != end) {
        setDirty((*j));
        ++j;
    }
}

const std::set<CellAddress> &PropertySheet::getDeps(const std::string &name) const
{
    static std::set<CellAddress> empty;
    std::map<std::string, std::set< CellAddress > >::const_iterator i = propertyNameToCellMap.find(name);

    if (i != propertyNameToCellMap.end())
        return i->second;
    else
        return empty;
}

const std::set<std::string> &PropertySheet::getDeps(CellAddress pos) const
{
    static std::set<std::string> empty;
    std::map<CellAddress, std::set< std::string > >::const_iterator i = cellToPropertyNameMap.find(pos);

    if (i != cellToPropertyNameMap.end())
        return i->second;
    else
        return empty;
}

void PropertySheet::recomputeDependencies(CellAddress key)
{
    Signaller signaller(*this);

    removeDependencies(key);
    addDependencies(key);
    rebuildDocDepList();
}

void PropertySheet::rebuildDocDepList()
{
    Signaller signaller(*this);

    docDeps.clear();
    BuildDocDepsExpressionVisitor v(docDeps);

    std::map<CellAddress, Cell* >::iterator i = data.begin();

    /* Resolve all cells */
    while (i != data.end()) {
        i->second->visit(v);
        ++i;
    }
}

PyObject *PropertySheet::getPyObject()
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new PropertySheetPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

void PropertySheet::resolveAll()
{
    std::map<CellAddress, Cell* >::iterator i = data.begin();

    /* Resolve all cells */
    Signaller signaller(*this);
    while (i != data.end()) {
        recomputeDependencies(i->first);
        setDirty(i->first);
        ++i;
    }
    touch();
}

PropertySheet::Signaller::Signaller(PropertySheet &sheet)
    : mSheet(sheet)
{
    if (mSheet.signalCounter == 0)
        mSheet.aboutToSetValue();
    mSheet.signalCounter++;
}

PropertySheet::Signaller::~Signaller()
{
    mSheet.signalCounter--;
    if (mSheet.signalCounter == 0)
           mSheet.hasSetValue();
}
