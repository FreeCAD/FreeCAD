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

#ifndef PROPERTYSHEET_H
#define PROPERTYSHEET_H

#include <map>
#include <App/DocumentObserver.h>
#include <App/DocumentObject.h>
#include <App/Property.h>
#include <App/PropertyLinks.h>
#include "Cell.h"

namespace Spreadsheet
{

class Sheet;
class PropertySheet;
class SheetObserver;

class PropertySheet : public App::Property {
    TYPESYSTEM_HEADER();
public:

    PropertySheet(Sheet * _owner = 0);

    PropertySheet(const PropertySheet & other);

    ~PropertySheet();

    virtual Property *Copy(void) const;

    virtual void Paste(const Property &from);

    virtual void Save (Base::Writer & writer) const;

    virtual void Restore(Base::XMLReader & reader);

    Cell *createCell(CellAddress address);

    void setValue() { }

    void setContent(CellAddress address, const char * value);

    void setAlignment(CellAddress address, int _alignment);

    void setStyle(CellAddress address, const std::set<std::string> & _style);

    void setForeground(CellAddress address, const App::Color &color);

    void setBackground(CellAddress address, const App::Color &color);

    void setDisplayUnit(CellAddress address, const std::string & unit);

    void setAlias(CellAddress address, const std::string &alias);

    void setComputedUnit(CellAddress address, const Base::Unit & unit);

    void setSpans(CellAddress address, int rows, int columns);

    void clear(CellAddress address);

    void clear();

    Cell * getValue(CellAddress key);

    std::set<CellAddress> getUsedCells() const;

    Sheet * sheet() const { return owner; }

    const std::set<CellAddress> & getDirty() { return dirty; }

    void setDirty(CellAddress address);

    void clearDirty(CellAddress key) { dirty.erase(key); }

    void clearDirty() { dirty.clear(); purgeTouched(); }

    bool isDirty() const { return dirty.size() > 0; }

    void moveCell(CellAddress currPos, CellAddress newPos);

    void insertRows(int row, int count);

    void removeRows(int row, int count);

    void insertColumns(int col, int count);

    void removeColumns(int col, int count);

    unsigned int getMemSize (void);

    bool mergeCells(CellAddress from, CellAddress to);

    void splitCell(CellAddress address);

    void getSpans(CellAddress address, int &rows, int &cols) const;

    bool isMergedCell(CellAddress address) const;

    bool isHidden(CellAddress address) const;

    const std::set< CellAddress > & getDeps(const std::string & name) const;

    const std::set<std::string> &getDeps(CellAddress pos) const;

    const std::vector<App::DocumentObject*> & getDocDeps() const { return docDeps; }

    class Signaller {
    public:
        Signaller(PropertySheet & sheet);
        ~Signaller();
    private:
        PropertySheet & mSheet;
    };

    void recomputeDependencies(CellAddress key);

    PyObject *getPyObject(void);

    void resolveAll();

    void invalidateDependants(const App::DocumentObject *docObj);

    void renamedDocumentObject(const App::DocumentObject *docObj);

    void renamedDocument(const App::Document *doc);

private:

    friend class Signaller;

    friend class SheetObserver;

    Cell *cellAt(CellAddress address);

    Cell *nonNullCellAt(CellAddress address);

    const Cell *cellAt(CellAddress address) const;

    bool colSortFunc(const CellAddress &a, const CellAddress &b);

    bool rowSortFunc(const CellAddress &a, const CellAddress &b);

    friend class Cell;

    std::map<CellAddress, Cell*> data;

    std::set<CellAddress> dirty;

    /* Merged cells; cell -> anchor cell */
    std::map<CellAddress, CellAddress> mergedCells;

    Sheet * owner;

    /*
     * Cell dependency tracking
     */

    void addDependencies(CellAddress key);

    void removeDependencies(CellAddress key);

    void recomputeDependants(const App::Property * prop);

    void recomputeDependants(const App::DocumentObject * docObj);

    /* Cell dependencies, i.e when a change occurs to property given in key,
      the set of addresses needs to be recomputed.
      */
    std::map<std::string, std::set< CellAddress > > propertyNameToCellMap;

    /* Properties this cell depends on */
    std::map<CellAddress, std::set< std::string > > cellToPropertyNameMap;

    /* Cell dependencies, i.e when a change occurs to documentObject given in key,
      the set of addresses needs to be recomputed.
      */
    std::map<std::string, std::set< CellAddress > > documentObjectToCellMap;

    /* DocumentObject this cell depends on */
    std::map<CellAddress, std::set< std::string > > cellToDocumentObjectMap;

    /* Other document objects the sheet depends on */
    std::vector<App::DocumentObject*> docDeps;

    /* Name of document objects, used for renaming */
    std::map<const App::DocumentObject*, std::string> documentObjectName;

    /* Name of documents, used for renaming */
    std::map<const App::Document*, std::string> documentName;

    int signalCounter;

    Py::Object PythonObject;
};

}
#endif // PROPERTYSHEET_H
