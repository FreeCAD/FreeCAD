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

#ifndef Spreadsheet_Spreadsheet_H
#define Spreadsheet_Spreadsheet_H

#ifdef signals
#undef signals
#define signals signals
#endif

#include <App/DocumentObject.h>
#include <App/DocumentObserver.h>
#include <App/PropertyFile.h>
#include <App/PropertyUnits.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <App/DynamicProperty.h>
#include <App/Material.h>
#include <Base/Unit.h>
#include <map>
#include "PropertySheet.h"
#include "PropertyColumnWidths.h"
#include "PropertyRowHeights.h"
#include "Utils.h"

namespace Spreadsheet
{

class Sheet;
class Cell;
class Expression;
class Range;
class SheetObserver;


class SpreadsheetExport Sheet : public App::DocumentObject
{
    PROPERTY_HEADER(Sheet::Sheet);

public:

    /// Constructor
    Sheet();
    virtual ~Sheet();

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "SpreadsheetGui::ViewProviderSheet";
    }

    bool importFromFile(const std::string & filename, char delimiter = '\t', char quoteChar = '\0', char escapeChar = '\\');

    bool exportToFile(const std::string & filename, char delimiter = '\t', char quoteChar = '\0', char escapeChar = '\\') const;

    bool mergeCells(const Range &range);

    void splitCell(CellAddress address);

    Cell * getCell(CellAddress address);

    Cell *getNewCell(CellAddress address);

    void setCell(const char *address, const char *value);

    void setCell(CellAddress address, const char *value);

    void clearAll();

    void clear(CellAddress address, bool all = true);

    void getSpans(CellAddress address, int & rows, int & cols) const;

    bool isMergedCell(CellAddress address) const;

    void setColumnWidth(int col, int width);

    int getColumnWidth(int col) const;

    void setRowHeight(int row, int height);

    int getRowHeight(int row) const;

    std::vector<std::string> getUsedCells() const;

    void insertColumns(int col, int count);

    void removeColumns(int col, int count);

    void insertRows(int row, int count);

    void removeRows(int row, int count);

    void setContent(CellAddress address, const char * value);

    void setAlignment(CellAddress address, int _alignment);

    void setStyle(CellAddress address, const std::set<std::string> & _style);

    void setForeground(CellAddress address, const App::Color &color);

    void setBackground(CellAddress address, const App::Color &color);

    void setDisplayUnit(CellAddress address, const std::string & unit);

    void setComputedUnit(CellAddress address, const Base::Unit & unit);

    void setAlias(CellAddress address, const std::string & alias);

    void setSpans(CellAddress address, int rows, int columns);

    std::set<std::string> dependsOn(CellAddress address) const;

    void providesTo(CellAddress address, std::set<std::string> & result) const;

    PyObject *getPyObject();

    App::Property *getPropertyByName(const char *name) const;

    const char* getPropertyName(const App::Property* prop) const;

    virtual short mustExecute(void) const;

    App::DocumentObjectExecReturn *execute(void);

    void getCellAddress(const App::Property *prop, CellAddress &address);

    std::map<int, int> getColumnWidths() const;

    std::map<int, int> getRowHeights() const;

    void setPosition(CellAddress address);

    // Signals

    boost::signal<void (Spreadsheet::CellAddress)> cellUpdated;

    boost::signal<void (Spreadsheet::CellAddress)> cellSpanChanged;

    boost::signal<void (int, int)> columnWidthChanged;

    boost::signal<void (int, int)> rowHeightChanged;

    boost::signal<void (CellAddress)> positionChanged;


    /** @name Access properties */
    //@{
    App::Property* addDynamicProperty(
        const char* type, const char* name=0,
        const char* group=0, const char* doc=0,
        short attr=0, bool ro=false, bool hidden=false) {
        return props.addDynamicProperty(type, name, group, doc, attr, ro, hidden);
    }
    virtual bool removeDynamicProperty(const char* name) {
        return props.removeDynamicProperty(name);
    }
    std::vector<std::string> getDynamicPropertyNames() const {
        return props.getDynamicPropertyNames();
    }
    App::Property *getDynamicPropertyByName(const char* name) const {
        return props.getDynamicPropertyByName(name);
    }
    virtual void addDynamicProperties(const App::PropertyContainer* cont) {
        return props.addDynamicProperties(cont);
    }
    /// get all properties of the class (including properties of the parent)
    virtual void getPropertyList(std::vector<App::Property*> &List) const {
        props.getPropertyList(List);
    }
    /// get all properties of the class (including parent)
    void getPropertyMap(std::map<std::string,App::Property*> &Map) const {
        props.getPropertyMap(Map);
    }

    short getPropertyType(const App::Property *prop) const {
        return props.getPropertyType(prop);
    }

    /// get the name of a property
    virtual const char* getName(const App::Property* prop) const {
        return props.getPropertyName(prop);
    }
    //@}

    void observeDocument(App::Document *document);

protected:

    void providesTo(CellAddress address, std::set<CellAddress> & result) const;

    void onDocumentRestored();

    void onRelabledDocument(const App::Document & document);

    void onRenamedDocument(const App::Document & document);

    void recomputeCell(CellAddress p);

    App::Property *getProperty(CellAddress key) const;

    App::Property *getProperty(const char * addr) const;

    void updateProperty(CellAddress key);

    App::Property *setStringProperty(CellAddress key, const std::string & value) ;

    App::Property *setFloatProperty(CellAddress key, double value);

    App::Property *setQuantityProperty(CellAddress key, double value, const Base::Unit &unit);

    static std::string toAddress(CellAddress key);

    void moveCell(CellAddress currPos, CellAddress newPos);

    void renamedDocumentObject(const App::DocumentObject * docObj);

    /* Properties for used cells */
    App::DynamicProperty props;

    /* Mapping of properties to cell position */
    std::map<const App::Property*, CellAddress > propAddress;

    /* Mapping of cell position to alias property */
    std::map<CellAddress, App::Property*> aliasProp;

    /* Set of cells with errors */
    std::set<CellAddress> cellErrors;

    /* Properties */

    /* Cell data */
    PropertySheet cells;

    /* Column widths */
    PropertyColumnWidths columnWidths;

    /* Row heights */
    PropertyRowHeights rowHeights;

    App::PropertyInteger currRow;
    App::PropertyInteger currColumn;

    /* Dependencies to other documents */
    App::PropertyLinkList docDeps;

    /* Document observers to track changes to external properties */
    typedef std::map<std::string, SheetObserver* > ObserverMap;
    ObserverMap observers;

    boost::BOOST_SIGNALS_NAMESPACE::scoped_connection onRelabledDocumentConnection;
    boost::BOOST_SIGNALS_NAMESPACE::scoped_connection onRenamedDocumentConnection;

    friend class SheetObserver;

    friend class PropertySheet;
};

} //namespace Spreadsheet


#endif // Spreadsheet_Spreadsheet_H
