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
#include <App/Range.h>
#include <App/FeaturePython.h>
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
class SheetObserver;

/** Spreadsheet quantity property
 * This is a property for quantities, and unlike its ancestor implements
 * Copy() and Paste() methods. It is used by the spreadsheet to
 * create aliases in a generic way.
 */
class SpreadsheetExport PropertySpreadsheetQuantity : public App::PropertyQuantity
{
    TYPESYSTEM_HEADER();
public:
    PropertySpreadsheetQuantity(void){}
    virtual ~PropertySpreadsheetQuantity(){}

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);
};

class SpreadsheetExport Sheet : public App::DocumentObject
{
    PROPERTY_HEADER(Spreadsheet::Sheet);

public:
    App::PropertyIntegerSet hiddenRows;
    App::PropertyIntegerSet hiddenColumns;
    App::PropertyBool PythonMode;

    /// Constructor
    Sheet();
    virtual ~Sheet();

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "SpreadsheetGui::ViewProviderSheet";
    }

    bool importFromFile(const std::string & filename, char delimiter = '\t', char quoteChar = '\0', char escapeChar = '\\');

    bool exportToFile(const std::string & filename, char delimiter = '\t', char quoteChar = '\0', char escapeChar = '\\') const;

    bool mergeCells(const App::Range &range);

    void splitCell(App::CellAddress address);

    Cell * getCell(App::CellAddress address);

    Cell *getNewCell(App::CellAddress address);

    void setCell(const char *address, const char *value);

    void setCell(App::CellAddress address, const char *value);

    void clearAll();

    void clear(App::CellAddress address, bool all = true);

    void getSpans(App::CellAddress address, int & rows, int & cols) const;

    bool isMergedCell(App::CellAddress address) const;

    void setColumnWidth(int col, int width);

    int getColumnWidth(int col) const;

    void setRowHeight(int row, int height);

    int getRowHeight(int row) const;

    std::vector<std::string> getUsedCells() const;

    void insertColumns(int col, int count);

    void removeColumns(int col, int count);

    void insertRows(int row, int count);

    void removeRows(int row, int count);

    void setContent(App::CellAddress address, const char * value);

    void setAlignment(App::CellAddress address, int alignment);

    void setStyle(App::CellAddress address, const std::set<std::string> & style);

    void setForeground(App::CellAddress address, const App::Color &color);

    void setBackground(App::CellAddress address, const App::Color &color);

    void setDisplayUnit(App::CellAddress address, const std::string & unit);

    void setComputedUnit(App::CellAddress address, const Base::Unit & unit);

    void setAlias(App::CellAddress address, const std::string & alias);

    std::string getAddressFromAlias(const std::string & alias) const;

    bool isValidAlias(const std::string &candidate);

    void setSpans(App::CellAddress address, int rows, int columns);

    std::set<std::string> dependsOn(App::CellAddress address) const;

    void providesTo(App::CellAddress address, std::set<std::string> & result) const;

    void editCell(App::CellAddress address, const char *data);

    bool hasCell(const std::vector<App::Range> &ranges) const;

    PyObject *getPyObject();

    PropertySheet *getCells() { return &cells; }

    App::Property *getPropertyByName(const char *name) const;

    const char* getPropertyName(const App::Property* prop) const;

    virtual short mustExecute(void) const;

    App::DocumentObjectExecReturn *execute(void);

    bool getCellAddress(const App::Property *prop, App::CellAddress &address);

    std::map<int, int> getColumnWidths() const;

    std::map<int, int> getRowHeights() const;

    std::string getRow(int offset=0) const;

    std::string getColumn(int offset=0) const;

    void touchCells(App::Range range);

    // Signals

    boost::signals2::signal<void (App::CellAddress)> cellUpdated;

    boost::signals2::signal<void (App::CellAddress)> cellSpanChanged;

    boost::signals2::signal<void (int, int)> columnWidthChanged;

    boost::signals2::signal<void (int, int)> rowHeightChanged;

    /** @name Access properties */
    //@{
    App::Property* addDynamicProperty(
        const char* type, const char* name=0,
        const char* group=0, const char* doc=0,
        short attr=0, bool ro=false, bool hidden=false) {
        return props.addDynamicProperty(type, name, group, doc, attr, ro, hidden);
    }
    virtual bool removeDynamicProperty(const char* name) {
        App::DocumentObject::onAboutToRemoveProperty(name);
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

    /// get the group of a property
    const char* getPropertyGroup(const App::Property* prop) const {
        return props.getPropertyGroup(prop);
    }

    /// get the documentation of a property
    const char* getPropertyDocumentation(const App::Property* prop) const {
        return props.getPropertyDocumentation(prop);
    }

    /// get the name of a property
    virtual const char* getName(const App::Property* prop) const {
        return props.getPropertyName(prop);
    }
    //@}

    void observeDocument(App::Document *document);

    virtual void renameObjectIdentifiers(const std::map<App::ObjectIdentifier, App::ObjectIdentifier> & paths);

protected:

    virtual void onChanged(const App::Property *prop);

    void updateColumnsOrRows(bool horizontal, int section, int count) ;

    std::set<App::CellAddress> providesTo(App::CellAddress address) const;

    void onDocumentRestored();

    void onRelabledDocument(const App::Document & document);

    void onRenamedDocument(const App::Document & document);

    void recomputeCell(App::CellAddress p);

    App::Property *getProperty(App::CellAddress key) const;

    App::Property *getProperty(const char * addr) const;

    void updateAlias(App::CellAddress key);

    void updateProperty(App::CellAddress key);

    App::Property *setStringProperty(App::CellAddress key, const std::string & value) ;

    App::Property *setObjectProperty(App::CellAddress key, Py::Object obj) ;

    App::Property *setFloatProperty(App::CellAddress key, double value);

    App::Property *setIntegerProperty(App::CellAddress key, long value);

    App::Property *setQuantityProperty(App::CellAddress key, double value, const Base::Unit &unit);

    void aliasRemoved(App::CellAddress address, const std::string &alias);

    void removeAliases();

    virtual void onSettingDocument();

    /* Properties for used cells */
    App::DynamicProperty props;

    /* Mapping of properties to cell position */
    std::map<const App::Property*, App::CellAddress > propAddress;

    /* Removed (unprocessed) aliases */
    std::map<App::CellAddress, std::string> removedAliases;

    /* Set of cells with errors */
    std::set<App::CellAddress> cellErrors;

    /* Properties */

    /* Cell data */
    PropertySheet cells;

    /* Column widths */
    PropertyColumnWidths columnWidths;

    /* Row heights */
    PropertyRowHeights rowHeights;

    /* Document observers to track changes to external properties */
    typedef std::map<std::string, SheetObserver* > ObserverMap;
    ObserverMap observers;

    boost::signals2::scoped_connection onRelabledDocumentConnection;
    boost::signals2::scoped_connection onRenamedDocumentConnection;

    int currentRow = -1;
    int currentCol = -1;

    friend class SheetObserver;

    friend class PropertySheet;
};

typedef App::FeaturePythonT<Sheet> SheetPython;

} //namespace Spreadsheet


#endif // Spreadsheet_Spreadsheet_H
