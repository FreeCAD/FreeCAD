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

class PropertySheet : public App::Property, private App::AtomicPropertyChangeInterface<PropertySheet> {
    TYPESYSTEM_HEADER();
public:

    PropertySheet(Sheet * _owner = 0);

    ~PropertySheet();

    virtual Property *Copy(void) const;

    virtual void Paste(const Property &from);

    virtual void Save (Base::Writer & writer) const;

    virtual void Restore(Base::XMLReader & reader);

    Cell *createCell(App::CellAddress address);

    void setValue() { }

    void setContent(App::CellAddress address, const char * value);

    void setAlignment(App::CellAddress address, int _alignment);

    void setStyle(App::CellAddress address, const std::set<std::string> & _style);

    void setForeground(App::CellAddress address, const App::Color &color);

    void setBackground(App::CellAddress address, const App::Color &color);

    void setDisplayUnit(App::CellAddress address, const std::string & unit);

    void setAlias(App::CellAddress address, const std::string &alias);

    void setComputedUnit(App::CellAddress address, const Base::Unit & unit);

    void setSpans(App::CellAddress address, int rows, int columns);

    void clear(App::CellAddress address);

    void clear();

    Cell * getValue(App::CellAddress key);

    const Cell * getValue(App::CellAddress key) const;

    const Cell * getValueFromAlias(const std::string &alias) const;

    bool isValidAlias(const std::string &candidate);

    std::set<App::CellAddress> getUsedCells() const;

    Sheet * sheet() const { return owner; }

    const std::set<App::CellAddress> & getDirty() { return dirty; }

    void setDirty(App::CellAddress address);

    void clearDirty(App::CellAddress key) { dirty.erase(key); }

    void clearDirty() { dirty.clear(); purgeTouched(); }

    bool isDirty() const { return dirty.size() > 0; }

    void moveCell(App::CellAddress currPos, App::CellAddress newPos, std::map<App::ObjectIdentifier, App::ObjectIdentifier> &renames);

    void insertRows(int row, int count);

    void removeRows(int row, int count);

    void insertColumns(int col, int count);

    void removeColumns(int col, int count);

    virtual unsigned int getMemSize (void) const;

    bool mergeCells(App::CellAddress from, App::CellAddress to);

    void splitCell(App::CellAddress address);

    void getSpans(App::CellAddress address, int &rows, int &cols) const;

    bool isMergedCell(App::CellAddress address) const;

    bool isHidden(App::CellAddress address) const;

    const std::set< App::CellAddress > & getDeps(const std::string & name) const;

    const std::set<std::string> &getDeps(App::CellAddress pos) const;

    const std::set<App::DocumentObject*> & getDocDeps() const { return docDeps; }

    void recomputeDependencies(App::CellAddress key);

    PyObject *getPyObject(void);

    void resolveAll();

    void invalidateDependants(const App::DocumentObject *docObj);

    void renamedDocumentObject(const App::DocumentObject *docObj);

    void renamedDocument(const App::Document *doc);

    void renameObjectIdentifiers(const std::map<App::ObjectIdentifier, App::ObjectIdentifier> &paths);

    void deletedDocumentObject(const App::DocumentObject *docObj);

    void documentSet();

private:

    PropertySheet(const PropertySheet & other);

    /* friends */

    friend class AtomicPropertyChange;

    friend class SheetObserver;

    friend class Cell;

    friend class Sheet;

    Cell *cellAt(App::CellAddress address);

    Cell *nonNullCellAt(App::CellAddress address);

    const Cell *cellAt(App::CellAddress address) const;

    bool colSortFunc(const App::CellAddress &a, const App::CellAddress &b);

    bool rowSortFunc(const App::CellAddress &a, const App::CellAddress &b);

    /*! Set of cells that have been marked dirty */
    std::set<App::CellAddress> dirty;

    /*! Cell data in this property */
    std::map<App::CellAddress, Cell*> data;

    /*! Merged cells; cell -> anchor cell */
    std::map<App::CellAddress, App::CellAddress> mergedCells;

    /*! Owner of this property */
    Sheet * owner;

    /*
     * Cell dependency tracking
     */

    void addDependencies(App::CellAddress key);

    void removeDependencies(App::CellAddress key);

    void recomputeDependants(const App::Property * prop);

    void recomputeDependants(const App::DocumentObject * docObj);

    void rebuildDocDepList();

    /*! Cell dependencies, i.e when a change occurs to property given in key,
      the set of addresses needs to be recomputed.
      */
    std::map<std::string, std::set< App::CellAddress > > propertyNameToCellMap;

    /*! Properties this cell depends on */
    std::map<App::CellAddress, std::set< std::string > > cellToPropertyNameMap;

    /*! Cell dependencies, i.e when a change occurs to documentObject given in key,
      the set of addresses needs to be recomputed.
      */
    std::map<std::string, std::set< App::CellAddress > > documentObjectToCellMap;

    /*! DocumentObject this cell depends on */
    std::map<App::CellAddress, std::set< std::string > > cellToDocumentObjectMap;

    /*! Other document objects the sheet depends on */
    std::set<App::DocumentObject*> docDeps;

    /*! Name of document objects, used for renaming */
    std::map<const App::DocumentObject*, std::string> documentObjectName;

    /*! Name of documents, used for renaming */
    std::map<const App::Document*, std::string> documentName;

    /*! Mapping of cell position to alias property */
    std::map<App::CellAddress, std::string> aliasProp;

    /*! Mapping of alias property to cell position */
    std::map<std::string, App::CellAddress> revAliasProp;

    /*! The associated python object */
    Py::Object PythonObject;
};

}
#endif // PROPERTYSHEET_H
