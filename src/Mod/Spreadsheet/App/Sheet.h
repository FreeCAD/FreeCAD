/***************************************************************************
 *   Copyright (c) 2013 Eivind Kvedalen (eivind@kvedalen.name              *
 *   Copyright (c) 2011 Jrgen Riegel (juergen.riegel@web.de)               *
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

#include <App/DocumentObject.h>
#include <App/DocumentObserver.h>
#include <App/PropertyFile.h>
#include <App/PropertyUnits.h>
#include <App/DynamicProperty.h>
#include <App/Material.h>
#include <Base/Unit.h>
#include <boost/signal.hpp>
#include <boost/bind.hpp>
#include <map>

namespace App {
class Property;
class DynamicProperty;
class DocumentObserver;
}

namespace Spreadsheet
{

class Sheet;
class Expression;

class SheetObserver : public App::DocumentObserver {
public:
    SheetObserver(App::Document* document, Sheet *_sheet);
    virtual void slotCreatedDocument(const App::Document& Doc);
    virtual void slotDeletedDocument(const App::Document& Doc);
    virtual void slotCreatedObject(const App::DocumentObject& Obj);
    virtual void slotDeletedObject(const App::DocumentObject& Obj);
    virtual void slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop);
    void ref();
    bool unref();
private:
    int refCount;
    Sheet * sheet;
};

class SpreadsheetExport Sheet : public App::DocumentObject
{
    PROPERTY_HEADER(Sheet::Sheet);

protected:
    typedef unsigned int CellPos;

public:

    class DisplayUnit {
    public:

        std::string stringRep;
        Base::Unit unit;
        double scaler;

        DisplayUnit(const std::string _stringRep = "", const Base::Unit _unit = Base::Unit(), double _scaler = 0.0)
            : stringRep(_stringRep)
            , unit(_unit)
            , scaler(_scaler)
        {
        }
    };

    /// Constructor
    Sheet();
    virtual ~Sheet();

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "SpreadsheetGui::ViewProviderSheet";
    }

    bool importFromFile(const std::string & filename, char delimiter = '\t', char quoteChar = '\0', char escapeChar = '\\');

    bool exportToFile(const std::string & filename, char delimiter = '\t', char quoteChar = '\0', char escapeChar = '\\') const;

    bool mergeCells(const std::string & from, const std::string & to);

    void splitCell(const std::string & address);

    const std::string &getCellString(int row, int col) const;

    const Expression * getCell(int row, int col) const;

    const Expression * getCell(const std::string & cell) const;

    void setCell(const char *address, const char *value);

    void setCell(int row, int col, const char *value);

    void setCell(int row, int col, const Expression *expression, const char *value);

    void setAlignment(int row, int col, int _alignment);

    bool getAlignment(int row, int col, int & _alignment) const;

    void setStyle(int row, int col, const std::set<std::string> & _style);

    bool getStyle(int row, int col, std::set<std::string> & style) const;

    void setForeground(int row, int col, const App::Color &color);

    bool getForeground(int row, int col, App::Color &color) const;

    void setBackground(int row, int col, const App::Color &color);

    bool getBackground(int row, int col, App::Color &color) const;

    void setUnit(int row, int col, const std::string & unit);

    bool getUnit(int row, int col, DisplayUnit &unit) const;

    const Base::Unit & getComputedUnit(int row, int col) const;

    bool clearAll();

    bool clear(const char *address, bool all = true);

    void getSpans(int row, int col, int & rows, int & cols) const;

    bool isMergedCell(int row, int col) const;

    void setColumnWidth(int col, int width);

    int getColumnWidth(int col) const;

    void setRowHeight(int row, int height);

    int getRowHeight(int row) const;

    std::vector<std::string> getUsedCells() const;

    void Save (Base::Writer &writer) const;

    void Restore(Base::XMLReader &reader);

    PyObject *getPyObject();

    App::Property *getPropertyByName(const char *name) const;

    virtual App::DocumentObjectExecReturn *recompute(void);

    virtual short mustExecute(void) const;

    App::DocumentObjectExecReturn *execute(void);

    void getCellAddress(const App::Property *prop, int & row, int & col);

    // Signals

    boost::signal<void (int, int)> cellUpdated;

    boost::signal<void (int, int)> cellSpanChanged;

    boost::signal<void (int, int)> columnWidthChanged;

    boost::signal<void (int, int)> rowHeightChanged;

    // Static members

    static void addressToRowCol(const char *address, int &row, int &col);

    static std::string toAddress(int row, int col);

    static int decodeAlignment(const std::string &itemStr, int alignment);

    static std::string encodeAlignment(int alignment);

    static std::string encodeStyle(const std::set<std::string> &style);

    static App::Color decodeColor(const std::string &color, const App::Color &defaultColor);

protected:

    App::Property *getProperty(CellPos key) const;

    const std::string getPropertyName(const App::Property *prop) const;

    const Expression *getCell(CellPos key) const;

    void updateProperty(CellPos key) const;

    void setStringProperty(CellPos key, const char *value) const;

    void setFloatProperty(CellPos key, double value) const;

    void setQuantityProperty(CellPos key, double value, const Base::Unit &unit) const;

    static CellPos addressToCellPos(const char *address);

    static std::string toAddress(CellPos key);

    static inline CellPos encodePos(int row, int col) {
        return (row << 16) | col;
    }

    static inline void decodePos(CellPos pos, int &row, int &col) {
        row = (pos >> 16) & 0xffff;
        col = pos & 0xffff;
    }

    void addDependencies(const Expression *expression, CellPos key);

    void removeDependencies(const Expression *expression, CellPos key);

    void recomputeDependants(const App::Property * prop);

    static std::string encodeAttribute(const std::string &str);

    static std::string encodeColor(const App::Color &color);

    static std::string rowName(int row);

    int decodeRow(const char *row);

    static std::string columnName(int col);

    int decodeColumn(const char *col);

    static const int MAX_ROWS;
    static const int MAX_COLUMNS;

    /* Parsed expressions for used cells */
    std::map<CellPos, const Expression* > cells;

    /* String contents, stored as typed in by user */
    std::map<CellPos, std::string> stringCells;

    /* alignment */

    std::map<CellPos, int> alignment;

    /* style */
    std::map<CellPos, std::set<std::string> > style;

    /* foreground color */
    std::map<CellPos, App::Color> foregroundColor;

    /* background color */
    std::map<CellPos, App::Color> backgroundColor;

    /* unit */
    std::map<CellPos, DisplayUnit> displayUnit;

    /* computedunit */
    mutable std::map<CellPos, Base::Unit> computedUnit;

    /* Properties for used cells */
    mutable App::DynamicProperty props;

    /* Mapping of properties to cell position */
    mutable std::map<const App::Property*, CellPos > propAddress;

    /* Cell dependencies, i.e when a change occurs to property given in key,
      the set of addresses needs to be recomputed.
      */
    mutable std::map<std::string, std::set< CellPos > > deps;

    /* Cells we are currently computing, used to resolve circular dependencies */
    mutable std::set<CellPos> isComputing;

    /* Document observers to track changes to external properties */
    typedef std::map<std::string, SheetObserver* > ObserverMap;
    ObserverMap observers;

    /* Merged cells; cell -> anchor cell */
    std::map<CellPos, CellPos> mergedCells;

    /* Merged cells; anchor cell -> (rows, cols) span */
    std::map<CellPos, std::pair<int, int> > span;

    /* Column widths */
    std::map<int, int> columnWidths;

    /* Row heights */
    std::map<int, int> rowHeights;

    friend class SheetObserver;
};

} //namespace Spreadsheet


#endif // Spreadsheet_Spreadsheet_H
