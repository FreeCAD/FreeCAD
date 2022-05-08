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

#ifndef PROPERTYSHEET_H
#define PROPERTYSHEET_H

#include <map>
#include <App/DocumentObserver.h>
#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <App/PropertyLinks.h>
#include <Base/SmartPtrPy.h>
#include "Cell.h"

namespace Spreadsheet
{

class Sheet;
class PropertySheet;
class SheetObserver;

class SpreadsheetExport PropertySheet : public App::PropertyExpressionContainer
                                      , private App::AtomicPropertyChangeInterface<PropertySheet> {
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:

    PropertySheet(Sheet * _owner = nullptr);

    ~PropertySheet();

    virtual std::map<App::ObjectIdentifier, const App::Expression*> getExpressions() const override;
    virtual void setExpressions(std::map<App::ObjectIdentifier, App::ExpressionPtr> &&exprs) override;
    virtual void onRelabeledDocument(const App::Document &doc) override;

    virtual void updateElementReference(
            App::DocumentObject *feature,bool reverse=false,bool notify=false) override;
    virtual bool referenceChanged() const override;
    virtual bool adjustLink(const std::set<App::DocumentObject *> &inList) override;
    virtual Property *CopyOnImportExternal(const std::map<std::string,std::string> &nameMap) const override;
    virtual Property *CopyOnLabelChange(App::DocumentObject *obj, 
                        const std::string &ref, const char *newLabel) const override;
    virtual Property *CopyOnLinkReplace(const App::DocumentObject *parent,
                        App::DocumentObject *oldObj, App::DocumentObject *newObj) const override;
    virtual void breakLink(App::DocumentObject *obj, bool clear) override;

    virtual void afterRestore() override;
    virtual void onContainerRestored() override;

    virtual Property *Copy(void) const override;

    virtual void Paste(const Property &from) override;

    virtual void Save (Base::Writer & writer) const override;

    virtual void Restore(Base::XMLReader & reader) override;

    void copyCells(Base::Writer &writer, const std::vector<App::Range> &ranges) const;

    void pasteCells(Base::XMLReader &reader, App::Range dstRange);

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

    void clear(App::CellAddress address, bool toClearAlias=true);

    void clear();

    Cell * getValue(App::CellAddress key);

    const Cell * getValue(App::CellAddress key) const;

    Cell * getValueFromAlias(const std::string &alias);

    const Cell * getValueFromAlias(const std::string &alias) const;

    bool isValidAlias(const std::string &candidate);

    std::vector<App::CellAddress> getUsedCells() const;

    std::vector<App::CellAddress> getNonEmptyCells() const;

    Sheet * sheet() const { return owner; }

    const std::set<App::CellAddress> & getDirty() { return dirty; }

    void setDirty(App::CellAddress address);

    void setDirty();

    void clearDirty(App::CellAddress key) { dirty.erase(key); }

    void clearDirty() { dirty.clear(); purgeTouched(); }

    bool isDirty() const { return dirty.size() > 0; }

    void pasteCells(const std::map<App::CellAddress, std::string> &cells, int rowOffset, int colOffset);

    void insertRows(int row, int count);

    std::vector<App::CellAddress> getRows(int row, int count) const;

    void removeRows(int row, int count);

    void insertColumns(int col, int count);

    std::vector<App::CellAddress> getColumns(int column, int count) const;

    void removeColumns(int col, int count);

    virtual unsigned int getMemSize (void) const override;

    bool mergeCells(App::CellAddress from, App::CellAddress to);

    void splitCell(App::CellAddress address);

    void getSpans(App::CellAddress address, int &rows, int &cols) const;

    App::CellAddress getAnchor(App::CellAddress address) const;

    bool isMergedCell(App::CellAddress address) const;

    bool isHidden(App::CellAddress address) const;

    const std::set< App::CellAddress > & getDeps(const std::string & name) const;

    const std::set<std::string> &getDeps(App::CellAddress pos) const;

    void recomputeDependencies(App::CellAddress key);

    PyObject *getPyObject(void) override;
    void setPyObject(PyObject *) override;

    PyObject *getPyValue(PyObject *key);

    void invalidateDependants(const App::DocumentObject *docObj);

    void renamedDocumentObject(const App::DocumentObject *docObj);
    void renameObjectIdentifiers(const std::map<App::ObjectIdentifier, App::ObjectIdentifier> &paths);

    void deletedDocumentObject(const App::DocumentObject *docObj);

    void documentSet();

    App::CellAddress getCellAddress(const char *addr, bool silent=false) const;
    App::Range getRange(const char *range, bool silent=false) const;

    std::string getRow(int offset=0) const;

    std::string getColumn(int offset=0) const;

    virtual void setPathValue(const App::ObjectIdentifier & path, const boost::any & value) override;
    virtual const boost::any getPathValue(const App::ObjectIdentifier & path) const override;

    unsigned getBindingBorder(App::CellAddress address) const;

    bool isBindingPath(const App::ObjectIdentifier &path,
            App::CellAddress *from=nullptr, App::CellAddress *to=nullptr, bool *href=nullptr) const;

    enum BindingType {
        BindingNone,
        BindingNormal,
        BindingHiddenRef,
    };
    BindingType getBinding(const App::Range &range,
            App::ExpressionPtr *pStart=nullptr, App::ExpressionPtr *pEnd=nullptr) const;

protected:
    virtual void hasSetValue() override;
    virtual void hasSetChildValue(App::Property &prop) override;
    virtual void onBreakLink(App::DocumentObject *obj) override;
    virtual void onAddDep(App::DocumentObject *obj) override;
    virtual void onRemoveDep(App::DocumentObject *obj) override;

private:

    PropertySheet(const PropertySheet & other);
    PropertySheet& operator= (const PropertySheet&);

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

    void clearAlias(App::CellAddress address);

    void moveAlias(App::CellAddress currPos, App::CellAddress newPos);

    void moveCell(App::CellAddress currPos, App::CellAddress newPos, std::map<App::ObjectIdentifier, App::ObjectIdentifier> &renames);

    /*
     * Cell dependency tracking
     */

    void addDependencies(App::CellAddress key);

    void removeDependencies(App::CellAddress key);

    void slotChangedObject(const App::DocumentObject &obj, const App::Property &prop);
    void recomputeDependants(const App::DocumentObject *obj, const char *propName);

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

    /*! Mapping of cell position to alias property */
    std::map<App::CellAddress, std::string> aliasProp;

    /*! Mapping of alias property to cell position */
    std::map<std::string, App::CellAddress> revAliasProp;

    /*! The associated python object */
    Py::SmartPtr PythonObject;

    std::map<const App::DocumentObject*, boost::signals2::scoped_connection> depConnections;

    int updateCount;
    bool restoring = false;
};

}
#endif // PROPERTYSHEET_H
