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

#ifndef Spreadsheet_Spreadsheet_H
#define Spreadsheet_Spreadsheet_H

#ifdef signals
#undef signals
#define signals signals
#endif

#include <map>

#include <App/DocumentObject.h>
#include <App/DynamicProperty.h>
#include <App/FeaturePython.h>
#include <App/PropertyUnits.h>
#include <App/Range.h>
#include <Base/Unit.h>

#include "PropertyColumnWidths.h"
#include "PropertyRowHeights.h"
#include "PropertySheet.h"


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
class SpreadsheetExport PropertySpreadsheetQuantity: public App::PropertyQuantity
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertySpreadsheetQuantity() = default;
    ~PropertySpreadsheetQuantity() override = default;

    Property* Copy() const override;
    void Paste(const Property& from) override;
};

class SpreadsheetExport Sheet: public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Spreadsheet::Sheet);

public:
    /// Constructor
    Sheet();
    ~Sheet() override;

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return "SpreadsheetGui::ViewProviderSheet";
    }

    bool importFromFile(const std::string& filename,
                        char delimiter = '\t',
                        char quoteChar = '\0',
                        char escapeChar = '\\');

    bool getCharsFromPrefs(char& delimiter, char& quote, char& escape, std::string& errMsg);

    bool exportToFile(const std::string& filename,
                      char delimiter = '\t',
                      char quoteChar = '\0',
                      char escapeChar = '\\') const;

    bool mergeCells(const App::Range& range);

    void splitCell(App::CellAddress address);

    Cell* getCell(App::CellAddress address);

    Cell* getNewCell(App::CellAddress address);

    enum Border
    {
        BorderTop = 1,
        BorderLeft = 2,
        BorderBottom = 4,
        BorderRight = 8,
        BorderAll = 15,
    };
    unsigned getCellBindingBorder(App::CellAddress address) const;

    PropertySheet::BindingType getCellBinding(App::Range& range,
                                              App::ExpressionPtr* pStart = nullptr,
                                              App::ExpressionPtr* pEnd = nullptr,
                                              App::ObjectIdentifier* pTarget = nullptr) const;

    void setCell(const char* address, const char* value);

    void setCell(App::CellAddress address, const char* value);

    void clearAll();

    void clear(App::CellAddress address, bool all = true);

    void getSpans(App::CellAddress address, int& rows, int& cols) const;

    bool isMergedCell(App::CellAddress address) const;

    App::CellAddress getAnchor(App::CellAddress address) const;

    void setColumnWidth(int col, int width);

    int getColumnWidth(int col) const;

    void setRowHeight(int row, int height);

    int getRowHeight(int row) const;

    std::vector<std::string> getUsedCells() const;

    void insertColumns(int col, int count);

    void removeColumns(int col, int count);

    void insertRows(int row, int count);

    void removeRows(int row, int count);

    void setContent(App::CellAddress address, const char* value);

    void setAlignment(App::CellAddress address, int alignment);

    void setStyle(App::CellAddress address, const std::set<std::string>& style);

    void setForeground(App::CellAddress address, const App::Color& color);

    void setBackground(App::CellAddress address, const App::Color& color);

    void setDisplayUnit(App::CellAddress address, const std::string& unit);

    void setComputedUnit(App::CellAddress address, const Base::Unit& unit);

    void setAlias(App::CellAddress address, const std::string& alias);

    std::string getAddressFromAlias(const std::string& alias) const;

    bool isValidAlias(const std::string& candidate);

    void setSpans(App::CellAddress address, int rows, int columns);

    std::set<std::string> dependsOn(App::CellAddress address) const;

    void providesTo(App::CellAddress address, std::set<std::string>& result) const;

    bool hasCell(const std::vector<App::Range>& ranges) const;
    PyObject* getPyObject() override;

    PropertySheet* getCells()
    {
        return &cells;
    }

    App::Property* getPropertyByName(const char* name) const override;

    App::Property* getDynamicPropertyByName(const char* name) const override;

    void
    getPropertyNamedList(std::vector<std::pair<const char*, App::Property*>>& List) const override;

    short mustExecute() const override;

    App::DocumentObjectExecReturn* execute() override;

    bool getCellAddress(const App::Property* prop, App::CellAddress& address);

    App::CellAddress getCellAddress(const char* name, bool silent = false) const;

    App::Range getRange(const char* name, bool silent = false) const;

    std::map<int, int> getColumnWidths() const;

    std::map<int, int> getRowHeights() const;

    std::string getRow(int offset = 0) const;

    std::string getColumn(int offset = 0) const;

    void touchCells(App::Range range);

    void recomputeCells(App::Range range);

    // Signals

    boost::signals2::signal<void(App::CellAddress)> cellUpdated;

    boost::signals2::signal<void(App::Range)> rangeUpdated;

    boost::signals2::signal<void(App::CellAddress)> cellSpanChanged;

    boost::signals2::signal<void(int, int)> columnWidthChanged;

    boost::signals2::signal<void(int, int)> rowHeightChanged;

    void observeDocument(App::Document* document);

    void renameObjectIdentifiers(
        const std::map<App::ObjectIdentifier, App::ObjectIdentifier>& paths) override;

    void setCopyOrCutRanges(const std::vector<App::Range>& ranges, bool copy = true);
    const std::vector<App::Range>& getCopyOrCutRange(bool copy = true) const;
    unsigned getCopyOrCutBorder(App::CellAddress address, bool copy = true) const;

protected:
    void onChanged(const App::Property* prop) override;

    void updateColumnsOrRows(bool horizontal, int section, int count);

    std::set<App::CellAddress> providesTo(App::CellAddress address) const;

    void onDocumentRestored() override;

    void recomputeCell(App::CellAddress p);

    App::Property* getProperty(App::CellAddress key) const;

    App::Property* getProperty(const char* addr) const;

    void updateProperty(App::CellAddress key);

    App::Property* setStringProperty(App::CellAddress key, const std::string& value);

    App::Property* setObjectProperty(App::CellAddress key, Py::Object obj);

    App::Property* setFloatProperty(App::CellAddress key, double value);

    App::Property* setIntegerProperty(App::CellAddress key, long value);

    App::Property* setQuantityProperty(App::CellAddress key, double value, const Base::Unit& unit);

    void onSettingDocument() override;

    void updateBindings();

    /* Properties for used cells */
    App::DynamicProperty& props;

    /* Mapping of properties to cell position */
    std::map<const App::Property*, App::CellAddress> propAddress;

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
    using ObserverMap = std::map<std::string, SheetObserver*>;
    ObserverMap observers;

    int currentRow = -1;
    int currentCol = -1;

    std::vector<App::Range> boundRanges;

    std::vector<App::Range> copyCutRanges;
    bool hasCopyRange = false;

    friend class SheetObserver;

    friend class PropertySheet;
};

using SheetPython = App::FeaturePythonT<Sheet>;

}  // namespace Spreadsheet


#endif  // Spreadsheet_Spreadsheet_H
