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

FC_LOG_LEVEL_INIT("Spreadsheet", true, true);

using namespace App;
using namespace Base;
using namespace Spreadsheet;

TYPESYSTEM_SOURCE(Spreadsheet::PropertySheet , App::PropertyXLinkContainer);

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
    , documentName(other.documentName)
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
    AtomicPropertyChange signaller(*this);

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

    mergedCells = froms->mergedCells;
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

    PropertyXLinkContainer::Save(writer);

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
        PropertyXLinkContainer::Restore(reader);

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
    signaller.tryInvoke();
}

void PropertySheet::copyCells(Base::Writer &writer, const std::vector<Range> &ranges) const {
    writer.Stream() << "<?xml version='1.0' encoding='utf-8'?>" << std::endl;
    writer.Stream() << "<Cells count=\"" << ranges.size() << "\">" << std::endl;
    writer.incInd();
    for(auto range : ranges) {
        auto r = range;
        int count = 0;
        do {
            if(getValue(*r))
                ++count;
        }while(r.next());
        writer.Stream() << writer.ind() << "<Range from=\"" << range.fromCellString()
            << "\" to=\"" << range.toCellString() << "\" count=\"" << count << "\">" << std::endl;
        writer.incInd();
        do {
            auto cell = getValue(*range);
            if(cell)
                cell->save(writer);
        }while(range.next());
        writer.decInd();
        writer.Stream() << writer.ind() << "</Range>" << std::endl;
    }
    writer.decInd();
    writer.Stream() << "</Cells>" << std::endl;
}

void PropertySheet::pasteCells(XMLReader &reader, const CellAddress &addr) {
    AtomicPropertyChange signaller(*this);

    bool first = true;
    int roffset=0,coffset=0;

    reader.readElement("Cells");
    int rangeCount = reader.getAttributeAsInteger("count");

    for(;rangeCount;--rangeCount) {
        reader.readElement("Range");
        CellAddress from(reader.getAttribute("from"));
        CellAddress to(reader.getAttribute("to"));
        int cellCount = reader.getAttributeAsInteger("count");
        Range range(from,to);
        bool hasCells = !!cellCount;
        for(;cellCount;--cellCount) {
            reader.readElement("Cell");
            CellAddress src(reader.getAttribute("address"));
            if(first) {
                first = false;
                roffset = addr.row() - from.row();
                coffset = addr.col() - from.col();
            }else
                range.next();
            while(src!=*range) {
                CellAddress dst(*range);
                dst.setRow(dst.row()+roffset);
                dst.setCol(dst.col()+coffset);
                owner->clear(dst);
                owner->cellUpdated(dst);
                range.next();
            }
            CellAddress dst(src.row()+roffset, src.col()+coffset);
            auto cell = owner->getNewCell(dst);
            cell->setSpans(-1,-1);
            cell->restore(reader,true);
            int rows, cols;
            if (cell->getSpans(rows, cols) && (rows > 1 || cols > 1)) 
                mergeCells(dst, CellAddress(dst.row() + rows - 1, dst.col() + cols - 1));

            if(roffset || coffset) {
                OffsetCellsExpressionVisitor<PropertySheet> visitor(*this, roffset, coffset);
                cell->visit(visitor);
                if(visitor.changed())
                    recomputeDependencies(dst);
            }
            dirty.insert(dst);
            owner->cellUpdated(dst);
        }
        if(!hasCells || range.next()) {
            do {
                CellAddress dst(*range);
                dst.setRow(dst.row()+roffset);
                dst.setCol(dst.col()+coffset);
                owner->clear(dst);
                owner->cellUpdated(dst);
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
    if (alias.size() > 0 && !isValidAlias(alias))
        throw Base::ValueError("Invalid alias");

    const Cell * aliasedCell = getValueFromAlias(alias);
    Cell * cell = nonNullCellAt(address);

    if (aliasedCell != 0 && cell != aliasedCell)
        throw Base::ValueError("Alias already defined.");

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

    if (oldAlias.size() > 0 && alias.size() > 0) {
        std::map<App::ObjectIdentifier, App::ObjectIdentifier> m;

        App::ObjectIdentifier key(owner, oldAlias);
        App::ObjectIdentifier value(owner, alias);

        m[key] = value;

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

    AtomicPropertyChange signaller(*this);

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
    signaller.tryInvoke();
}

void PropertySheet::moveCell(CellAddress currPos, CellAddress newPos, std::map<App::ObjectIdentifier, App::ObjectIdentifier> & renames)
{
    std::map<CellAddress, Cell*>::const_iterator i = data.find(currPos);
    std::map<CellAddress, Cell*>::const_iterator j = data.find(newPos);

    AtomicPropertyChange signaller(*this);

    if (j != data.end())
        clear(newPos);

    if (i != data.end()) {
        Cell * cell = i->second;
        int rows, columns;

        // Get merged cell data
        cell->getSpans(rows, columns);

        // Remove merged cell data
        splitCell(currPos);

        std::string alias;
        if(cell->getAlias(alias)) {
            owner->aliasRemoved(currPos, alias);
            cell->setAlias("");
        }

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

        if(alias.size())
            cell->setAlias(alias);
        
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
    std::sort(keys.begin(), keys.end(), boost::bind(&PropertySheet::rowSortFunc, this, _1, _2));

    MoveCellsExpressionVisitor<PropertySheet> visitor(*this, 
            CellAddress(row, CellAddress::MAX_COLUMNS), count, 0);

    AtomicPropertyChange signaller(*this);
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

void PropertySheet::removeRows(int row, int count)
{
    std::vector<CellAddress> keys;
    std::map<App::ObjectIdentifier, App::ObjectIdentifier> renames;

    /* Copy all keys from cells map */
    boost::copy(data | boost::adaptors::map_keys, std::back_inserter(keys));

    /* Sort them */
    std::sort(keys.begin(), keys.end(), boost::bind(&PropertySheet::rowSortFunc, this, _1, _2));

    MoveCellsExpressionVisitor<PropertySheet> visitor(*this, 
            CellAddress(row + count - 1, CellAddress::MAX_COLUMNS), -count, 0);

    AtomicPropertyChange signaller(*this);
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

void PropertySheet::removeColumns(int col, int count)
{
    std::vector<CellAddress> keys;
    std::map<App::ObjectIdentifier, App::ObjectIdentifier> renames;

    /* Copy all keys from cells map */
    boost::copy(data | boost::adaptors::map_keys, std::back_inserter(keys));

    /* Sort them */
    std::sort(keys.begin(), keys.end(), boost::bind(&PropertySheet::colSortFunc, this, _1, _2));

    MoveCellsExpressionVisitor<PropertySheet> visitor(*this, 
            CellAddress(CellAddress::MAX_ROWS, col + count - 1), 0, -count);

    AtomicPropertyChange signaller(*this);
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

    for(auto &dep : expression->getDeps()) {

        App::DocumentObject *docObj = dep.first;
        App::Document *doc = docObj->getDocument();

        std::string docName = doc->Label.getValue();
        std::string docObjName = docName + "#" + docObj->getNameInDocument();

        documentName[doc] = docName;

        owner->observeDocument(doc);

        documentObjectToCellMap[docObjName].insert(key);
        cellToDocumentObjectMap[key].insert(docObjName);
        ++updateCount;

        for(auto &props : dep.second) {
            std::string propName = docObjName + "." + props.first;
            FC_LOG("dep " << key.toString() << " -> " << propName);

            // Insert into maps
            propertyNameToCellMap[propName].insert(key);
            cellToPropertyNameMap[key].insert(propName);

            // Also an alias?
            if (docObj==owner && props.first.size()) {
                std::map<std::string, CellAddress>::const_iterator j = revAliasProp.find(props.first);

                if (j != revAliasProp.end()) {
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
        ++updateCount;
    }
}

/**
  * Recompute any cells that depend on \a prop.
  *
  */

void PropertySheet::recomputeDependants(const App::DocumentObject *owner, const char *propName)
{
    const char * nameInDoc = owner->getNameInDocument();
    if (!nameInDoc)
        return;
    const char * docName = owner->getDocument()->Label.getValue();

    // First, search without actual property name for sub-object/link
    // references, i.e indirect references. The depenedecies of these
    // references are too complex to track exactly, so we only track the
    // top parent object instead, and mark the involved expression
    // whenever the top parent changes.
    std::string fullName = std::string(docName) + "#" + std::string(nameInDoc) + ".";
    auto it = propertyNameToCellMap.find(fullName);
    if (it != propertyNameToCellMap.end()) {
        for(auto &cell : it->second)
            setDirty(cell);
    }

    if (propName) {
        // Now, we check for direct property references
        it = propertyNameToCellMap.find(fullName + propName);
        if (it != propertyNameToCellMap.end()) {
            for(auto &cell : it->second)
                setDirty(cell);
        }
    }
}

void PropertySheet::onBreakLink(App::DocumentObject *obj) {
    invalidateDependants(obj);
}

void PropertySheet::hasSetChildValue(App::Property &prop) {
    ++updateCount;
    PropertyXLinkContainer::hasSetChildValue(prop);
}

void PropertySheet::invalidateDependants(const App::DocumentObject *docObj)
{
    depConnections.erase(docObj);

    const char * docName = docObj->getDocument()->Label.getValue();
    const char * docObjName = docObj->getNameInDocument();

    // Recompute cells that depend on this cell
    std::string fullName = std::string(docName) + "#" + std::string(docObjName);
    std::map<std::string, std::set< CellAddress > >::const_iterator i = documentObjectToCellMap.find(fullName);

    if (i == documentObjectToCellMap.end())
        return;

    // Touch to force recompute
    touch();
    
    for(const auto &address : i->second) {
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
                &PropertySheet::slotChangedObject, this, _1, _2));
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

void PropertySheet::renamedDocument(const App::Document * doc)
{
    if (documentName.find(doc) == documentName.end())
        return;

    std::map<CellAddress, Cell* >::iterator i = data.begin();

    while (i != data.end()) {
        RelabelDocumentExpressionVisitor<PropertySheet> v(*this, documentName[doc], doc->Label.getValue());
        i->second->visit(v);
        if(v.changed()) {
            v.reset();
            recomputeDependencies(i->first);
            setDirty(i->first);
        }
        ++i;
    }
}

void PropertySheet::renameObjectIdentifiers(const std::map<App::ObjectIdentifier, App::ObjectIdentifier> &paths)
{
    RenameObjectIdentifierExpressionVisitor<PropertySheet> v(*this, paths, *this);

    for (std::map<CellAddress, Cell*>::iterator it = data.begin(); it != data.end(); ++it)
        it->second->visit(v);
}

void PropertySheet::deletedDocumentObject(const App::DocumentObject *docObj)
{
    (void)docObj;
    // This function is only used in SheetObserver, which is obselete.
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
    documentName[owner->getDocument()] = owner->getDocument()->Label.getValue();
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
    if(!updateCount || 
       !owner || !owner->getNameInDocument() || owner->isRestoring() ||
       this!=&owner->cells ||
       testFlag(LinkDetached)) 
    {
        PropertyXLinkContainer::hasSetValue();
        return;
    }

    updateCount = 0;

    std::set<App::DocumentObject*> deps;
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

    PropertyXLinkContainer::hasSetValue();
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

void PropertySheet::afterRestore()
{
    Base::FlagToggler<bool> flag(restoring);
    AtomicPropertyChange signaller(*this);
    for(auto &d : data)
        d.second->afterRestore();

    PropertyXLinkContainer::afterRestore();

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
    std::unique_ptr<AtomicPropertyChange> signaler;

    for(auto &d : data) {
        auto expr = d.second->expression.get();
        if(!expr)
            continue;
        try {
            bool need_adjust = false;
            for(auto docObj : expr->getDepObjects()) {
                if (docObj && docObj != owner && inList.count(docObj)) {
                    need_adjust = true;
                    break;
                }
            }
            if(!need_adjust)
                continue;
            if(!signaler)
                signaler.reset(new AtomicPropertyChange(*this));

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
    return !!signaler;
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

std::map<App::ObjectIdentifier, const App::Expression*> PropertySheet::getExpressions() const {
    std::map<App::ObjectIdentifier, const Expression*> res;
    for(auto &d : data) {
        if(d.second->expression)
            res[ObjectIdentifier(owner,d.first.toString())] = d.second->expression.get();
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
