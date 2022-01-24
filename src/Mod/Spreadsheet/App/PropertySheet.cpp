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

#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/assign.hpp>
#include <boost_bind_bind.hpp>
#include <boost/regex.hpp>
#include <Base/Console.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Property.h>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Tools.h>
#include <Base/PyObjectBase.h>
#include "PropertySheet.h"
#include "Sheet.h"
#include "Utils.h"
#include <PropertySheetPy.h>
#include <App/ExpressionVisitors.h>
#include <App/ExpressionParser.h>

FC_LOG_LEVEL_INIT("Spreadsheet", true, true)

using namespace App;
using namespace Base;
using namespace Spreadsheet;
namespace bp = boost::placeholders;

TYPESYSTEM_SOURCE(Spreadsheet::PropertySheet , App::PropertyExpressionContainer)

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
    cellToPropertyNameMap.clear();
    documentObjectToCellMap.clear();
    cellToDocumentObjectMap.clear();
    aliasProp.clear();
    revAliasProp.clear();

    clearDeps();
}

Cell *PropertySheet::getValue(CellAddress key)
{
    std::map<CellAddress, Cell*>::const_iterator i = data.find(key);

    if (i == data.end())
        return 0;
    else
        return i->second;
}

const Cell *PropertySheet::getValue(CellAddress key) const
{
    std::map<CellAddress, Cell*>::const_iterator i = data.find(key);

    if (i == data.end())
        return 0;
    else
        return i->second;
}

Cell * PropertySheet::getValueFromAlias(const std::string &alias)
{
    std::map<std::string, CellAddress>::const_iterator it = revAliasProp.find(alias);

    if (it != revAliasProp.end())
        return getValue(it->second);
    else
        return 0;
}

const Cell * PropertySheet::getValueFromAlias(const std::string &alias) const
{
    std::map<std::string, CellAddress>::const_iterator it = revAliasProp.find(alias);

    if (it != revAliasProp.end())
        return getValue(it->second);
    else
        return 0;
}

bool PropertySheet::isValidAlias(const std::string &candidate)
{
    static const boost::regex gen("^[A-Za-z][_A-Za-z0-9]*$");
    boost::cmatch cm;

    /* Check if it is used before */
    if (getValueFromAlias(candidate) != 0)
        return false;

    /* Check to make sure it doesn't clash with a predefined unit */
    if (ExpressionParser::isTokenAUnit(candidate))
        return false;

    /* Check to make sure it doesn't match a cell reference */
    if (boost::regex_match(candidate.c_str(), cm, gen)) {
        static const boost::regex e("\\${0,1}([A-Z]{1,2})\\${0,1}([0-9]{1,5})");

        if (boost::regex_match(candidate.c_str(), cm, e)) {
            const boost::sub_match<const char *> colstr = cm[1];
            const boost::sub_match<const char *> rowstr = cm[2];

            // A valid cell address?
            if (App::validRow(rowstr.str()) >= 0 && App::validColumn(colstr.str()) >= 0)
                return false;
        }
        return true;
    }
    else
        return false;
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

void PropertySheet::setDirty()
{
    AtomicPropertyChange signaller(*this);
    for(auto &address : getUsedCells()) {
        auto cell = cellAt(address);
        std::string content;
        if(cell && cell->getStringContent(content,false)) {
            cell->setContent(content.c_str());
        }
    }
}

Cell * PropertySheet::createCell(CellAddress address)
{
    Cell * cell = new Cell(address, this);

    data[address] = cell;

    return cell;
}

PropertySheet::PropertySheet(Sheet *_owner)
    : owner(_owner)
    , updateCount(0)
{
}

PropertySheet::PropertySheet(const PropertySheet &other)
    : dirty(other.dirty)
    , mergedCells(other.mergedCells)
    , owner(other.owner)
    , propertyNameToCellMap(other.propertyNameToCellMap)
    , cellToPropertyNameMap(other.cellToPropertyNameMap)
    , documentObjectToCellMap(other.documentObjectToCellMap)
    , cellToDocumentObjectMap(other.cellToDocumentObjectMap)
    , aliasProp(other.aliasProp)
    , revAliasProp(other.revAliasProp)
    , updateCount(other.updateCount)
{
    std::map<CellAddress, Cell* >::const_iterator i = other.data.begin();

    /* Copy cells */
    while (i != other.data.end()) {
        data[i->first] = new Cell(this, *i->second);
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
    const PropertySheet &froms = dynamic_cast<const PropertySheet&>(from);

    AtomicPropertyChange signaller(*this);

    std::map<CellAddress, Cell* >::iterator icurr = data.begin();

    /* Mark all first */
    while (icurr != data.end()) {
        icurr->second->mark();
        ++icurr;
    }

    std::map<CellAddress, Cell* >::const_iterator ifrom = froms.data.begin();
    while (ifrom != froms.data.end()) {
        std::map<CellAddress, Cell* >::iterator i = data.find(ifrom->first);

        if (i != data.end()) {
            *(data[ifrom->first]) = *(ifrom->second); // Exists; assign cell directly
        }
        else {
            data[ifrom->first] = new Cell(this, *(ifrom->second)); // Doesn't exist, copy using Cell's copy constructor
        }
        recomputeDependencies(ifrom->first);

        /* Set dirty */
        setDirty(ifrom->first);
        ++ifrom;
    }

    /* Remove all that are still marked */
    icurr = data.begin();
    while (icurr != data.end()) {
        Cell * cell = icurr->second;

        if (cell->isMarked()) {
            std::map<CellAddress, Cell* >::iterator next = icurr;

            ++next;
            clear(icurr->first);
            icurr = next;
        }
        else
            ++icurr;
    }

    mergedCells = froms.mergedCells;
    signaller.tryInvoke();
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

    writer.Stream() << writer.ind() << "<Cells Count=\"" << count
        << "\" xlink=\"1\">" << std::endl;

    writer.incInd();

    PropertyExpressionContainer::Save(writer);

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

    AtomicPropertyChange signaller(*this);

    reader.readElement("Cells");
    Cnt = reader.getAttributeAsInteger("Count");

    if(reader.hasAttribute("xlink") && reader.getAttributeAsInteger("xlink"))
        PropertyExpressionContainer::Restore(reader);

    for (int i = 0; i < Cnt; i++) {
        reader.readElement("Cell");

        const char* strAddress = reader.hasAttribute("address") ? reader.getAttribute("address") : "";

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
    signaller.tryInvoke();
}

void PropertySheet::copyCells(Base::Writer& writer, const std::vector<Range>& ranges) const {
    writer.Stream() << "<?xml version='1.0' encoding='utf-8'?>" << std::endl;
    writer.Stream() << "<Cells count=\"" << ranges.size() << "\">" << std::endl;
    writer.incInd();
    for (auto range : ranges) {
        writer.Stream() << writer.ind() << "<Range from=\"" << range.fromCellString()
            << "\" to=\"" << range.toCellString() << "\" count=\"" << range.size() << "\">" << std::endl;
        writer.incInd();
        do {
            auto cell = getValue(*range);
            if (cell && cell->isUsed()) {
                cell->save(writer);
            }
            else {
                // The cell is empty, so when it's pasted it needs to clear the existing contents
                writer.Stream() << writer.ind() << "<Cell "
                    << "address=\"" << (*range).toString() << "\" "
                    << "content = \"\" />";
            }
        } while (range.next());
        writer.decInd();
        writer.Stream() << writer.ind() << "</Range>" << std::endl;
    }
    writer.decInd();
    writer.Stream() << "</Cells>" << std::endl;
}

void PropertySheet::pasteCells(XMLReader &reader, Range dstRange) {
    reader.readElement("Cells");
    int rangeCount = reader.getAttributeAsInteger("count");
    if(rangeCount<=0)
        return;

    int dstRows = dstRange.rowCount();
    int dstCols = dstRange.colCount();
    CellAddress dstFrom = dstRange.from();

    int roffset,coffset;

    AtomicPropertyChange signaller(*this);
    for(int ri=0; ri < rangeCount; ++ri) {
        reader.readElement("Range");
        CellAddress from(reader.getAttribute("from"));
        CellAddress to(reader.getAttribute("to"));
        int cellCount = reader.getAttributeAsInteger("count");

        Range range(from,to);

        CellAddress addr(dstFrom);
        if(ri == 0) {
            roffset = addr.row() - from.row();
            coffset = addr.col() - from.col();
        }

        int rcount,ccount;
        if(rangeCount>1) {
            rcount = 1;
            ccount = 1;
        } else {
            rcount = dstRows/range.rowCount();
            if(rcount == 0)
                rcount = 1;
            ccount = dstCols/range.colCount();
            if(ccount == 0)
                ccount = 1;
        }
        for(int ci=0; ci < cellCount; ++ci) {
            reader.readElement("Cell");
            CellAddress src(reader.getAttribute("address"));

            if(ci)
                range.next();

            while(src!=*range) {
                for(int r=0; r < rcount; ++r) {
                    for(int c=0; c < ccount; ++c) {
                        CellAddress dst(range.row()+roffset+r*range.rowCount(),
                                        range.column()+coffset+c*range.colCount());
                        if(!dst.isValid())
                            continue;
                        owner->clear(dst);
                        owner->cellUpdated(dst);
                    }
                }
                range.next();
            }

            CellAddress newCellAddr;
            for(int r=0; r < rcount; ++r) {
                for(int c=0; c < ccount; ++c) {
                    CellAddress dst(src.row()+roffset+r*range.rowCount(),
                                    src.col()+coffset+c*range.colCount());
                    if(!dst.isValid())
                        continue;

                    auto cell = owner->getNewCell(dst);
                    cell->setSpans(-1,-1);

                    int roffset_cur, coffset_cur;
                    if(!newCellAddr.isValid()) {
                        roffset_cur = roffset;
                        coffset_cur = coffset;
                        newCellAddr = dst;
                        cell->restore(reader,true);
                    } else {
                        roffset_cur = r*range.rowCount();
                        coffset_cur = c*range.colCount();
                        auto newCell = owner->getCell(newCellAddr);
                        const Expression *expr;
                        if(!newCell || !(expr=newCell->getExpression(true))) {
                            FC_THROWM(Base::RuntimeError, "Failed to copy cell "
                                    << getFullName() << '.' << dst.toString()
                                    << " from " << newCellAddr.toString());
                        }
                        cell->setExpression(ExpressionPtr(expr->copy()));
                    }

                    int rows, cols;
                    if (cell->getSpans(rows, cols) && (rows > 1 || cols > 1)) 
                        mergeCells(dst, CellAddress(dst.row() + rows - 1, dst.col() + cols - 1));

                    if(roffset_cur || coffset_cur) {
                        OffsetCellsExpressionVisitor<PropertySheet> visitor(*this, roffset_cur, coffset_cur);
                        cell->visit(visitor);
                        if(visitor.changed())
                            recomputeDependencies(dst);
                    }
                    dirty.insert(dst);
                    owner->cellUpdated(dst);
                }
            }
        }
        if(cellCount == 0 || range.next()) {
            do {
                for(int r=0; r < rcount; ++r) {
                    for(int c=0; c < ccount; ++c) {
                        CellAddress dst(range.row()+roffset+r*range.rowCount(),
                                        range.column()+coffset+c*range.colCount());
                        if(!dst.isValid())
                            continue;
                        owner->clear(dst);
                        owner->cellUpdated(dst);
                    }
                }
            }while(range.next());
        }
    }
    signaller.tryInvoke();
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
    Cell * cell = nonNullCellAt(address);
    assert(cell != 0);
    if (cell->address != address) return; //Reject alignment change for merged cell except top-left one
    cell->setAlignment(_alignment);
}

void PropertySheet::setStyle(CellAddress address, const std::set<std::string> &_style)
{
    Cell * cell = nonNullCellAt(address);
    assert(cell != 0);
    cell->setStyle(_style);
}

void PropertySheet::setForeground(CellAddress address, const App::Color &color)
{
    Cell * cell = nonNullCellAt(address);
    assert(cell != 0);
    cell->setForeground(color);
}

void PropertySheet::setBackground(CellAddress address, const App::Color &color)
{
    Cell * cell = nonNullCellAt(address);
    assert(cell != 0);
    cell->setBackground(color);
}

void PropertySheet::setDisplayUnit(CellAddress address, const std::string &unit)
{
    Cell * cell = nonNullCellAt(address);
    assert(cell != 0);
    cell->setDisplayUnit(unit);
}


void PropertySheet::setAlias(CellAddress address, const std::string &alias)
{
    if (alias.size() > 0 && !isValidAlias(alias))
        throw Base::ValueError("Invalid alias");

    const Cell * aliasedCell = getValueFromAlias(alias);
    Cell * cell = nonNullCellAt(address);
    assert(cell != 0);

    if(aliasedCell == cell)
        return;

    if (aliasedCell)
        throw Base::ValueError("Alias already defined.");

    AtomicPropertyChange signaller(*this);

    /* Mark cells depending on this cell dirty; they need to be resolved when an alias changes or disappears */
    std::string fullName = owner->getFullName() + "." + address.toString();

    std::map<std::string, std::set< CellAddress > >::const_iterator j = propertyNameToCellMap.find(fullName);
    if (j != propertyNameToCellMap.end()) {
        std::set< CellAddress >::const_iterator k = j->second.begin();

        while (k != j->second.end()) {
            setDirty(*k);
            ++k;
        }
    }

    std::string oldAlias;
    cell->getAlias(oldAlias);
    cell->setAlias(alias);

    if (oldAlias.size() > 0) {
        std::map<App::ObjectIdentifier, App::ObjectIdentifier> m;

        App::ObjectIdentifier key(owner, oldAlias);
        App::ObjectIdentifier value(owner, alias.empty()?address.toString():alias);

        m[key] = value;

        owner->getDocument()->renameObjectIdentifiers(m);
    }

    signaller.tryInvoke();
}

void PropertySheet::setComputedUnit(CellAddress address, const Base::Unit &unit)
{
    Cell * cell = nonNullCellAt(address);
    assert(cell != 0);
    cell->setComputedUnit(unit);
}

void PropertySheet::setSpans(CellAddress address, int rows, int columns)
{
    Cell * cell = nonNullCellAt(address);
    assert(cell != 0);
    cell->setSpans(rows, columns);
}

void PropertySheet::clearAlias(CellAddress address)
{
    // Remove alias if it exists
    std::map<CellAddress, std::string>::iterator j = aliasProp.find(address);
    if (j != aliasProp.end()) {
        revAliasProp.erase(j->second);
        aliasProp.erase(j);
    }
}

void PropertySheet::clear(CellAddress address, bool toClearAlias)
{
    std::map<CellAddress, Cell*>::iterator i = data.find(address);

    if (i == data.end())
        return;

    AtomicPropertyChange signaller(*this);

    // Spit cell to clean up mergeCells map; all data is in first cell anyway
    splitCell(address);

    // Delete Cell object
    removeDependencies(address);
    delete i->second;

    // Mark as dirty
    dirty.insert(i->first);

    if (toClearAlias)
        clearAlias(address);

    // Erase from internal struct
    data.erase(i);
    signaller.tryInvoke();
}

void PropertySheet::moveAlias(CellAddress currPos, CellAddress newPos)
{
    std::map<CellAddress, std::string>::iterator j = aliasProp.find(currPos);
    if (j != aliasProp.end()) {
        aliasProp[newPos] = j->second;
        revAliasProp[j->second] = newPos;
        aliasProp.erase(currPos);
    }
}

void PropertySheet::moveCell(CellAddress currPos, CellAddress newPos, std::map<App::ObjectIdentifier, App::ObjectIdentifier> & renames)
{
    std::map<CellAddress, Cell*>::const_iterator i = data.find(currPos);
    std::map<CellAddress, Cell*>::const_iterator j = data.find(newPos);

    AtomicPropertyChange signaller(*this);

    if (j != data.end()) {
        // do not clear alias because we have moved them already
        clear(newPos, false);
    }

    if (i != data.end()) {
        Cell * cell = i->second;
        int rows, columns;

        // Get merged cell data
        cell->getSpans(rows, columns);

        // Remove merged cell data
        splitCell(currPos);

        // Remove from old
        removeDependencies(currPos);
        data.erase(currPos);
        setDirty(currPos);

        // Insert into new spot
        cell->moveAbsolute(newPos);
        data[newPos] = cell;

        if (rows > 1 || columns > 1) {
            CellAddress toPos(newPos.row() + rows - 1, newPos.col() + columns - 1);

            mergeCells(newPos, toPos);
        }
        else
            cell->setSpans(-1, -1);

        addDependencies(newPos);

        setDirty(newPos);

        renames[ObjectIdentifier(owner, currPos.toString())] = ObjectIdentifier(owner, newPos.toString());
    }
    signaller.tryInvoke();
}

void PropertySheet::insertRows(int row, int count)
{
    std::vector<CellAddress> keys;
    std::map<App::ObjectIdentifier, App::ObjectIdentifier> renames;

    /* Copy all keys from cells map */
    boost::copy( data | boost::adaptors::map_keys, std::back_inserter(keys));

    /* Sort them */
    std::sort(keys.begin(), keys.end(), boost::bind(&PropertySheet::rowSortFunc, this, bp::_1, bp::_2));

    MoveCellsExpressionVisitor<PropertySheet> visitor(*this, 
            CellAddress(row, CellAddress::MAX_COLUMNS), count, 0);

    AtomicPropertyChange signaller(*this);

    // move all the aliases first so dependencies can be calculated correctly
    for (std::vector<CellAddress>::const_reverse_iterator i = keys.rbegin(); i != keys.rend(); ++i) {
        if (i->row() >= row)
            moveAlias(*i, CellAddress(i->row() + count, i->col()));
    }

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
            moveCell(*i, CellAddress(i->row() + count, i->col()), renames);
    }

    const App::DocumentObject * docObj = static_cast<const App::DocumentObject*>(getContainer());
    owner->getDocument()->renameObjectIdentifiers(renames, [docObj](const App::DocumentObject * obj) { return obj != docObj; });
    signaller.tryInvoke();
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

std::vector<CellAddress> PropertySheet::getRows(int row, int count) const
{
    std::vector<CellAddress> keys;

    for (const auto &i : data) {
        auto key = i.first;
        if (key.row() >= row && key.row() < row + count)
            keys.push_back(key);
    }
    return keys;
}

void PropertySheet::removeRows(int row, int count)
{
    std::vector<CellAddress> keys;
    std::map<App::ObjectIdentifier, App::ObjectIdentifier> renames;

    /* Copy all keys from cells map */
    boost::copy(data | boost::adaptors::map_keys, std::back_inserter(keys));

    /* Sort them */
    std::sort(keys.begin(), keys.end(), boost::bind(&PropertySheet::rowSortFunc, this, bp::_1, bp::_2));

    MoveCellsExpressionVisitor<PropertySheet> visitor(*this, 
            CellAddress(row + count - 1, CellAddress::MAX_COLUMNS), -count, 0);

    AtomicPropertyChange signaller(*this);

    // move all the aliases first so dependencies can be calculated correctly
    for (std::vector<CellAddress>::const_iterator i = keys.begin(); i != keys.end(); ++i) {
        if (i->row() >= row && i->row() < row + count)
            clearAlias(*i);
        else if (i->row() >= row + count)
            moveAlias(*i, CellAddress(i->row() - count, i->col()));
    }

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
            clear(*i, false);  // aliases were cleared earlier
        else if (i->row() >= row + count)
            moveCell(*i, CellAddress(i->row() - count, i->col()), renames);
    }

    const App::DocumentObject * docObj = static_cast<const App::DocumentObject*>(getContainer());
    owner->getDocument()->renameObjectIdentifiers(renames, [docObj](const App::DocumentObject * obj) { return obj != docObj; });
    signaller.tryInvoke();
}

void PropertySheet::insertColumns(int col, int count)
{
    std::vector<CellAddress> keys;
    std::map<App::ObjectIdentifier, App::ObjectIdentifier> renames;

    /* Copy all keys from cells map */
    boost::copy(data | boost::adaptors::map_keys, std::back_inserter(keys));

    /* Sort them */
    std::sort(keys.begin(), keys.end());

    MoveCellsExpressionVisitor<PropertySheet> visitor(*this, 
            CellAddress(CellAddress::MAX_ROWS, col), 0, count);

    AtomicPropertyChange signaller(*this);

    // move all the aliases first so dependencies can be calculated correctly
    for (std::vector<CellAddress>::const_reverse_iterator i = keys.rbegin(); i != keys.rend(); ++i) {
        if (i->col() >= col)
            moveAlias(*i, CellAddress(i->row(), i->col() + count));
    }

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
            moveCell(*i, CellAddress(i->row(), i->col() + count), renames);
    }

    const App::DocumentObject * docObj = static_cast<const App::DocumentObject*>(getContainer());
    owner->getDocument()->renameObjectIdentifiers(renames, [docObj](const App::DocumentObject * obj) { return obj != docObj; });
    signaller.tryInvoke();
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

std::vector<CellAddress> PropertySheet::getColumns(int column, int count) const
{
    std::vector<CellAddress> keys;

    for (const auto &i : data) {
        auto key = i.first;
        if (key.col() >= column && key.col() < column + count)
            keys.push_back(key);
    }
    return keys;
}

void PropertySheet::removeColumns(int col, int count)
{
    std::vector<CellAddress> keys;
    std::map<App::ObjectIdentifier, App::ObjectIdentifier> renames;

    /* Copy all keys from cells map */
    boost::copy(data | boost::adaptors::map_keys, std::back_inserter(keys));

    /* Sort them */
    std::sort(keys.begin(), keys.end(), boost::bind(&PropertySheet::colSortFunc, this, bp::_1, bp::_2));

    MoveCellsExpressionVisitor<PropertySheet> visitor(*this, 
            CellAddress(CellAddress::MAX_ROWS, col + count - 1), 0, -count);

    AtomicPropertyChange signaller(*this);

    // move all the aliases first so dependencies can be calculated correctly
    for (std::vector<CellAddress>::const_iterator i = keys.begin(); i != keys.end(); ++i) {
        if (i->col() >= col && i->col() < col + count)
            clearAlias(*i);
        else if (i->col() >= col + count)
            moveAlias(*i, CellAddress(i->row(), i->col() - count));
    }

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
            clear(*i, false);  // aliases were cleared earlier
        else if (i->col() >= col + count)
            moveCell(*i, CellAddress(i->row(), i->col() - count), renames);
    }

    const App::DocumentObject * docObj = static_cast<const App::DocumentObject*>(getContainer());
    owner->getDocument()->renameObjectIdentifiers(renames, [docObj](const App::DocumentObject * obj) { return obj != docObj; } );
    signaller.tryInvoke();
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

    AtomicPropertyChange signaller(*this);

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
    signaller.tryInvoke();

    return true;
}

void PropertySheet::splitCell(CellAddress address)
{
    int rows, cols;
    std::map<CellAddress, CellAddress>::const_iterator i = mergedCells.find(address);

    if (i == mergedCells.end())
        return;

    CellAddress anchor = i->second;
    AtomicPropertyChange signaller(*this);
    cellAt(anchor)->getSpans(rows, cols);

    for (int r = anchor.row(); r <= anchor.row() + rows; ++r)
        for (int c = anchor.col(); c <= anchor.col() + cols; ++c) {
            setDirty(CellAddress(r, c));
            mergedCells.erase(CellAddress(r, c));
        }

    setSpans(anchor, -1, -1);
    signaller.tryInvoke();
}

void PropertySheet::getSpans(CellAddress address, int & rows, int & cols) const
{
    std::map<CellAddress, CellAddress>::const_iterator i = mergedCells.find(address);

    if (i != mergedCells.end()) {
        CellAddress anchor = i->second;

        if (anchor == address)
            cellAt(anchor)->getSpans(rows, cols);
        else
            rows = cols = 1;
    }
    else {
        rows = cols = 1;
    }
}

App::CellAddress Spreadsheet::PropertySheet::getAnchor(App::CellAddress address) const
{
    if (auto anchor = mergedCells.find(address); anchor != mergedCells.end())
        return anchor->second;
    else
        return address;
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

    for(auto &var : expression->getIdentifiers()) {
        for(auto &dep : var.first.getDep(true)) {
            App::DocumentObject *docObj = dep.first;
            App::Document *doc = docObj->getDocument();

            std::string docObjName = docObj->getFullName();

            owner->observeDocument(doc);

            documentObjectToCellMap[docObjName].insert(key);
            cellToDocumentObjectMap[key].insert(docObjName);
            ++updateCount;

            for(auto &name : dep.second) {
                std::string propName = docObjName + "." + name;
                FC_LOG("dep " << key.toString() << " -> " << name);

                // Insert into maps
                propertyNameToCellMap[propName].insert(key);
                cellToPropertyNameMap[key].insert(propName);

                // Also an alias?
                if (name.size() && docObj->isDerivedFrom(Sheet::getClassTypeId())) {
                    auto other = static_cast<Sheet*>(docObj);
                    auto j = other->cells.revAliasProp.find(name);

                    if (j != other->cells.revAliasProp.end()) {
                        propName = docObjName + "." + j->second.toString();
                        FC_LOG("dep " << key.toString() << " -> " << propName);

                        // Insert into maps
                        propertyNameToCellMap[propName].insert(key);
                        cellToPropertyNameMap[key].insert(propName);
                    }
                }
            }
        }
    }
}

/**
  * Remove dependencies given by \a expression for cell at \a key.
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

            //assert(k != propertyNameToCellMap.end());
            if (k != propertyNameToCellMap.end())
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

            //assert(k != documentObjectToCellMap.end());
            if (k != documentObjectToCellMap.end()) {
                k->second.erase(key);

                if (k->second.size() == 0)
                    documentObjectToCellMap.erase(*j);
            }

            ++j;
        }

        cellToDocumentObjectMap.erase(i2);
        ++updateCount;
    }
}

/**
  * Recompute any cells that depend on \a prop.
  *
  */

void PropertySheet::recomputeDependants(const App::DocumentObject *owner, const char *propName)
{
    auto itD = _Deps.find(const_cast<App::DocumentObject*>(owner));
    if(itD!=_Deps.end() && itD->second) {
        // Check for hidden reference. Because a hidden reference is not
        // protected by cyclic dependency checking, we need to take special
        // care to prevent it from misbehave.
        Sheet *sheet = Base::freecad_dynamic_cast<Sheet>(getContainer());
        if(!sheet || sheet->testStatus(App::ObjectStatus::Recompute2)
                || !owner || owner->testStatus(App::ObjectStatus::Recompute2))
            return;
    }

    // First, search without actual property name for sub-object/link
    // references, i.e indirect references. The dependencies of these
    // references are too complex to track exactly, so we only track the
    // top parent object instead, and mark the involved expression
    // whenever the top parent changes.
    std::string fullName = owner->getFullName() + ".";
    auto it = propertyNameToCellMap.find(fullName);
    if (it != propertyNameToCellMap.end()) {
        for(auto &cell : it->second)
            setDirty(cell);
    }

    if (propName && *propName) {
        // Now, we check for direct property references
        it = propertyNameToCellMap.find(fullName + propName);
        if (it != propertyNameToCellMap.end()) {
            for(auto &cell : it->second)
                setDirty(cell);
        }
    }
}

void PropertySheet::breakLink(App::DocumentObject *obj, bool clear) {
    AtomicPropertyChange signaller(*this,false);
    PropertyExpressionContainer::breakLink(obj,clear);
}

void PropertySheet::onBreakLink(App::DocumentObject *obj) {
    invalidateDependants(obj);
}

void PropertySheet::hasSetChildValue(App::Property &prop) {
    ++updateCount;
    PropertyExpressionContainer::hasSetChildValue(prop);
}

void PropertySheet::invalidateDependants(const App::DocumentObject *docObj)
{
    depConnections.erase(docObj);

    // Recompute cells that depend on this cell
    auto iter = documentObjectToCellMap.find(docObj->getFullName());
    if (iter == documentObjectToCellMap.end())
        return;

    // Touch to force recompute
    touch();

    AtomicPropertyChange signaller(*this);

    for(const auto &address : iter->second) {
        Cell * cell = getValue(address);
        cell->setResolveException("Unresolved dependency");
        setDirty(address);
    }
}

void PropertySheet::slotChangedObject(const App::DocumentObject &obj, const App::Property &prop) {
    recomputeDependants(&obj, prop.getName());
}

void PropertySheet::onAddDep(App::DocumentObject *obj) {
    depConnections[obj] = obj->signalChanged.connect(boost::bind(
                &PropertySheet::slotChangedObject, this, bp::_1, bp::_2));
}

void PropertySheet::onRemoveDep(App::DocumentObject *obj) {
    depConnections.erase(obj);
}

void PropertySheet::renamedDocumentObject(const App::DocumentObject * docObj)
{
#if 1
    (void)docObj;
#else
    if (documentObjectName.find(docObj) == documentObjectName.end())
        return;

    std::map<CellAddress, Cell* >::iterator i = data.begin();

    while (i != data.end()) {
        RelabelDocumentObjectExpressionVisitor<PropertySheet> v(*this, docObj);
        i->second->visit(v);
        if(v.changed()) {
            v.reset();
            recomputeDependencies(i->first);
            setDirty(i->first);
        }
        ++i;
    }
#endif
}

void PropertySheet::onRelabeledDocument(const App::Document &doc)
{
    RelabelDocumentExpressionVisitor v(doc);
    for(auto &c : data) 
        c.second->visit(v);
}

void PropertySheet::renameObjectIdentifiers(const std::map<App::ObjectIdentifier, App::ObjectIdentifier> &paths)
{
    RenameObjectIdentifierExpressionVisitor<PropertySheet> v(*this, paths, *this);
    for(auto &c : data) {
        c.second->visit(v);
        if(v.changed()) {
            v.reset();
            recomputeDependencies(c.first);
            setDirty(c.first);
        }
    }
}

void PropertySheet::deletedDocumentObject(const App::DocumentObject *docObj)
{
    (void)docObj;
    // This function is only used in SheetObserver, which is obsolete.
    //
    // if(docDeps.erase(const_cast<App::DocumentObject*>(docObj))) {
    //     const App::DocumentObject * docObj = dynamic_cast<const App::DocumentObject*>(getContainer());
    //     if(docObj && docObj->getDocument()!=docObj->getDocument()) {
    //         for(auto it=xlinks.begin();it!=xlinks.end();++it) {
    //             if(it->getValue() == docObj) {
    //                 xlinks.erase(it);
    //                 break;
    //             }
    //         }
    //     }
    // }
}

void PropertySheet::documentSet()
{
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
    AtomicPropertyChange signaller(*this);

    removeDependencies(key);
    addDependencies(key);
    signaller.tryInvoke();
}

void PropertySheet::hasSetValue()
{
    if(updateCount == 0 || 
       !owner || !owner->getNameInDocument() || owner->isRestoring() ||
       this!=&owner->cells ||
       testFlag(LinkDetached)) 
    {
        PropertyExpressionContainer::hasSetValue();
        return;
    }

    updateCount = 0;

    std::map<App::DocumentObject*,bool> deps;
    std::vector<std::string> labels;
    unregisterElementReference();
    UpdateElementReferenceExpressionVisitor<PropertySheet> v(*this);
    for(auto &d : data) {
        auto expr = d.second->expression.get();
        if(expr) {
            expr->getDepObjects(deps,&labels);
            if(!restoring)
                expr->visit(v);
        }
    }
    registerLabelReferences(std::move(labels));

    updateDeps(std::move(deps));

    PropertyExpressionContainer::hasSetValue();
}

PyObject *PropertySheet::getPyObject()
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new PropertySheetPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

void PropertySheet::setPyObject(PyObject *obj) {
    if(!obj || !PyObject_TypeCheck(obj, &PropertySheetPy::Type))
        throw Base::TypeError("Invalid type");
    if(obj != PythonObject.ptr())
        Paste(*static_cast<PropertySheetPy*>(obj)->getPropertySheetPtr());
}

PyObject *PropertySheet::getPyValue(PyObject *key) {
    assert(key);

    PY_TRY {
        std::string addr = Py::Object(key).as_string();
        CellAddress caddr = getCellAddress(addr.c_str(),true);
        if(caddr.isValid()) {
            auto prop = owner->getPropertyByName(caddr.toString().c_str());
            if(prop)
                return prop->getPyObject();
            Py_Return;
        }

        Range range = getRange(Py::Object(key).as_string().c_str(), false);
        if(!range.from().isValid() || !range.to().isValid())
            return Py::new_reference_to(Py::Tuple());

        Py::Tuple res(range.size());
        int i = 0;
        do {
            addr = range.address();
            auto prop = owner->getPropertyByName(addr.c_str());
            res.setItem(i++,prop?Py::asObject(prop->getPyObject()):Py::Object());
        } while(range.next());

        return Py::new_reference_to(res);

    } PY_CATCH
}

void PropertySheet::afterRestore()
{
    Base::FlagToggler<bool> flag(restoring);
    AtomicPropertyChange signaller(*this);

    PropertyExpressionContainer::afterRestore();
    {
        ObjectIdentifier::DocumentMapper mapper(this->_DocMap);
        for(auto &d : data)
            d.second->afterRestore();
    }

    for(auto &v : _XLinks) {
        auto &xlink = *v.second;
        if(!xlink.checkRestore())
            continue;
        auto iter = documentObjectToCellMap.find(xlink.getValue()->getFullName());
        if(iter == documentObjectToCellMap.end())
            continue;
        touch();
        for(const auto &address : iter->second)
            setDirty(address);
    }
    signaller.tryInvoke();
}

void PropertySheet::onContainerRestored() {
    Base::FlagToggler<bool> flag(restoring);
    unregisterElementReference();
    UpdateElementReferenceExpressionVisitor<PropertySheet> v(*this);
    for(auto &d : data) {
        auto expr = d.second->expression.get();
        if(expr)
            expr->visit(v);
    }
}

bool PropertySheet::adjustLink(const std::set<DocumentObject*> &inList) {
    AtomicPropertyChange signaller(*this,false);
    bool changed = false;

    for(auto &d : data) {
        auto expr = d.second->expression.get();
        if(!expr)
            continue;
        try {
            bool need_adjust = false;
            for(auto &v : expr->getDepObjects()) {
                auto docObj = v.first;
                if (v.second && docObj && docObj!=owner && inList.count(docObj)) {
                    need_adjust = true;
                    break;
                }
            }
            if(!need_adjust)
                continue;

            signaller.aboutToChange();
            changed = true;

            removeDependencies(d.first);
            expr->adjustLinks(inList);
            addDependencies(d.first);

        }catch(Base::Exception &e) {
            addDependencies(d.first);
            std::ostringstream ss;
            ss << "Failed to adjust link for " << owner->getFullName() << " in expression "
                << expr->toString() << ": " << e.what();
            throw Base::RuntimeError(ss.str());
        }
    }
    return changed;
}

void PropertySheet::updateElementReference(DocumentObject *feature,bool reverse,bool notify) 
{
    (void)notify;
    if(!feature)
        unregisterElementReference();
    UpdateElementReferenceExpressionVisitor<PropertySheet> visitor(*this,feature,reverse);
    for(auto &d : data) {
        auto expr = d.second->expression.get();
        if(!expr)
            continue;
        expr->visit(visitor);
    }
    if(feature && visitor.changed()) {
        auto owner = dynamic_cast<App::DocumentObject*>(getContainer());
        if(owner)
            owner->onUpdateElementReference(this);
    }
}

bool PropertySheet::referenceChanged() const {
    return false;
}

Property *PropertySheet::CopyOnImportExternal(
        const std::map<std::string,std::string> &nameMap) const 
{
    std::map<CellAddress,std::unique_ptr<Expression> > changed;
    for(auto &d : data) {
        auto e = d.second->expression.get();
        if(!e) continue;
        auto expr = e->importSubNames(nameMap);
        if(!expr)
            continue;
        changed[d.first] = std::move(expr);
    }
    if(changed.empty())
        return 0;
    std::unique_ptr<PropertySheet> copy(new PropertySheet(*this));
    for(auto &change : changed) 
        copy->data[change.first]->setExpression(std::move(change.second));
    return copy.release();
}

Property *PropertySheet::CopyOnLabelChange(App::DocumentObject *obj, 
        const std::string &ref, const char *newLabel) const
{
    std::map<CellAddress,std::unique_ptr<Expression> > changed;
    for(auto &d : data) {
        auto e = d.second->expression.get();
        if(!e) continue;
        auto expr = e->updateLabelReference(obj,ref,newLabel);
        if(!expr)
            continue;
        changed[d.first] = std::move(expr);
    }
    if(changed.empty())
        return 0;
    std::unique_ptr<PropertySheet> copy(new PropertySheet(*this));
    for(auto &change : changed) 
        copy->data[change.first]->setExpression(std::move(change.second));
    return copy.release();
}

Property *PropertySheet::CopyOnLinkReplace(const App::DocumentObject *parent, 
        App::DocumentObject *oldObj, App::DocumentObject *newObj) const
{
    std::map<CellAddress,std::unique_ptr<Expression> > changed;
    for(auto &d : data) {
        auto e = d.second->expression.get();
        if(!e) continue;
        auto expr = e->replaceObject(parent,oldObj,newObj);
        if(!expr)
            continue;
        changed[d.first] = std::move(expr);
    }
    if(changed.empty())
        return 0;
    std::unique_ptr<PropertySheet> copy(new PropertySheet(*this));
    for(auto &change : changed) 
        copy->data[change.first]->setExpression(std::move(change.second));
    return copy.release();
}

std::map<App::ObjectIdentifier, const App::Expression*> PropertySheet::getExpressions() const {
    std::map<App::ObjectIdentifier, const Expression*> res;
    for(auto &d : data) {
        if(d.second->expression) {
            res[ObjectIdentifier(owner,d.first.toString())] = d.second->getExpression(true);
        }
    }
    return res;
}

void PropertySheet::setExpressions(
        std::map<App::ObjectIdentifier, App::ExpressionPtr> &&exprs) 
{
    AtomicPropertyChange signaller(*this);
    for(auto &v : exprs) {
        CellAddress addr(v.first.getPropertyName().c_str());
        auto &cell = data[addr];
        if(!cell) {
            if(!v.second)
                continue;
            cell = new Cell(addr,this);
        }
        if(!v.second)
            clear(addr);
        else
            cell->setExpression(std::move(v.second));
    }
    signaller.tryInvoke();
}

App::CellAddress PropertySheet::getCellAddress(const char *addr, bool silent) const {
    assert(addr);
    CellAddress caddr;
    const Cell * cell = getValueFromAlias(addr);
    if(cell)
        return cell->getAddress();
    else
        return stringToAddress(addr,silent);
}

App::Range PropertySheet::getRange(const char *range, bool silent) const {
    assert(range);
    const char *sep = strchr(range,':');
    CellAddress from,to;
    if(!sep) 
        from = to = getCellAddress(range,silent);
    else {
        std::string addr(range,sep);

        auto findCell = [this, &addr](CellAddress caddr, int r, int c) -> CellAddress {
            if(!getValue(caddr))
                return CellAddress();
            if(addr == "-")
                r = 0;
            else
                c = 0;
            for(;;) {
                caddr.setRow(caddr.row()+r);
                caddr.setCol(caddr.col()+c);
                if(!caddr.isValid() || !getValue(caddr)) 
                    break;
            }
            caddr.setRow(caddr.row()-r);
            caddr.setCol(caddr.col()-c);
            return caddr;
        };

        if(addr == "-" || addr == "|") {
            to = getCellAddress(sep+1,silent);
            return Range(findCell(to,-1,-1), from);
        } else {
            from = getCellAddress(addr.c_str(),silent);
            addr = sep+1;
            if(addr == "-" || addr == "|")
                return Range(from, findCell(from,1,1));
            to = getCellAddress(addr.c_str(),silent);
        } 
    }

    if(!from.isValid() || !to.isValid())
        return App::Range(App::CellAddress(),App::CellAddress());
    return App::Range(from,to);
}

bool PropertySheet::isBindingPath(const ObjectIdentifier &path,
        CellAddress *from, CellAddress *to, bool *href) const
{
    const auto &comps = path.getComponents();
    if (comps.size()!=4 
            || !comps[2].isSimple()
            || !comps[3].isSimple()
            || (comps[1].getName()!="Bind"
                && comps[1].getName()!="BindHREF"
                && comps[1].getName()!="BindHiddenRef")
            || path.getProperty() != this)
    {
        return false;
    }
    if(href)
        *href = (comps[1].getName()=="BindHREF" || comps[1].getName()=="BindHiddenRef");
    if(from)
        *from = CellAddress(comps[2].getName());
    if(to)
        *to = CellAddress(comps[3].getName());
    return true;
}

PropertySheet::BindingType PropertySheet::getBinding(
        const Range &range, ExpressionPtr *pStart, ExpressionPtr *pEnd) const
{
    if(!owner)
        return BindingNone;

    for(int href=0;href<2;++href) {
        ObjectIdentifier path(*this);
        path << ObjectIdentifier::SimpleComponent(href?"BindHiddenRef":"Bind");
        path << ObjectIdentifier::SimpleComponent(range.from().toString().c_str());
        path << ObjectIdentifier::SimpleComponent(range.to().toString().c_str());
        auto res = owner->getExpression(path);
        if(res.expression && res.expression->isDerivedFrom(FunctionExpression::getClassTypeId()))
        {
            auto expr = static_cast<FunctionExpression*>(res.expression.get());
            if(href) {
                if((expr->getFunction()!=FunctionExpression::HIDDENREF
                            && expr->getFunction()!=FunctionExpression::HREF)
                        || expr->getArgs().size()!=1 
                        || !expr->getArgs().front()->isDerivedFrom(FunctionExpression::getClassTypeId()))
                    continue;
                expr = static_cast<FunctionExpression*>(expr->getArgs().front());
            }

            if(expr->getFunction() == FunctionExpression::TUPLE && expr->getArgs().size()==3) {
                if(pStart)
                    pStart->reset(expr->getArgs()[1]->copy());
                if(pEnd)
                    pEnd->reset(expr->getArgs()[2]->copy());
                return href?BindingHiddenRef:BindingNormal;
            }
        }
    }
    return BindingNone;
}

void PropertySheet::setPathValue(const ObjectIdentifier &path, const boost::any &value)
{
    if(!owner)
        FC_THROWM(Base::RuntimeError, "Invalid state");

    bool href = false;
    CellAddress from,to;
    if(!isBindingPath(path,&from,&to,&href)) {
        FC_THROWM(Base::IndexError, "Invalid binding of '" << path.toString()
                << "' in " << getFullName());
    }

    Base::PyGILStateLocker lock;
    Py::Object pyValue = pyObjectFromAny(value);

    if(pyValue.isSequence()) {
        Py::Sequence seq(pyValue);
        if(seq.size()==3 
                && PyObject_TypeCheck(seq[0].ptr(),&PropertySheetPy::Type)
                && Py::Object(seq[1].ptr()).isString()
                && Py::Object(seq[2].ptr()).isString())
        {
            AtomicPropertyChange signaller(*this,false);
            auto other = static_cast<PropertySheetPy*>(seq[0].ptr())->getPropertySheetPtr();
            auto otherOwner = Base::freecad_dynamic_cast<App::DocumentObject>(other->getContainer());
            if(!otherOwner)
                FC_THROWM(Base::RuntimeError, "Invalid binding of '" << other->getFullName()
                        << " in " << getFullName());

            App::CellAddress targetFrom = other->getCellAddress(
                Py::Object(seq[1].ptr()).as_string().c_str(), false);

            App::CellAddress targetTo = other->getCellAddress(
                Py::Object(seq[2].ptr()).as_string().c_str(), false);

            App::Range range(from,to);
            App::Range rangeTarget(targetFrom,targetTo);

            std::string expr(href?"href(":"");
            if(other != this) {
                if(otherOwner->getDocument() == owner->getDocument())
                    expr = otherOwner->getNameInDocument();
                else
                    expr = otherOwner->getFullName();
            }
            expr += ".";
            std::size_t exprSize = expr.size();

            do {
                CellAddress target(*rangeTarget);
                CellAddress source(*range);
                if(other == this && source.row() >= targetFrom.row()
                        && source.row() <= targetTo.row()
                        && source.col() >= targetFrom.col()
                        && source.col() <= targetTo.col())
                    continue;

                Cell *dst = other->getValue(target);
                Cell *src = getValue(source);
                if(!dst) {
                    if(src) {
                        signaller.aboutToChange();
                        owner->clear(source);
                        owner->cellUpdated(source);
                    }
                    continue;
                }

                if(!src) {
                    signaller.aboutToChange();
                    src = createCell(source);
                }

                std::string alias;
                if(this!=other && dst->getAlias(alias)) {
                    auto *oldCell = getValueFromAlias(alias);
                    if(oldCell && oldCell!=dst) {
                        signaller.aboutToChange();
                        oldCell->setAlias("");
                    }
                    std::string oldAlias;
                    if(!src->getAlias(oldAlias) || oldAlias!=alias) {
                        signaller.aboutToChange();
                        setAlias(source,alias);
                    }
                }

                expr.resize(exprSize);
                expr += rangeTarget.address();
                if(href)
                    expr += ")";
                auto e = App::ExpressionPtr(App::Expression::parse(owner,expr));
                auto e2 = src->getExpression();
                if(!e2 || !e->isSame(*e2,false)) {
                    signaller.aboutToChange();
                    src->setExpression(std::move(e));
                }

            } while(range.next() && rangeTarget.next());
            owner->rangeUpdated(range);
            signaller.tryInvoke();
            return;
        }
    }

    FC_THROWM(Base::TypeError, "Invalid path value '"
            << "' for " << getFullName());
}

const boost::any PropertySheet::getPathValue(const App::ObjectIdentifier & path) const {
    if(isBindingPath(path))
        return boost::any();
    return path.getValue();
}
