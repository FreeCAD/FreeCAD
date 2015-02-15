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

#include <boost/tokenizer.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/assign.hpp>
#include <boost/graph/topological_sort.hpp>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DynamicProperty.h>
#include <Base/Exception.h>
#include <Base/Placement.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include "Expression.h"
#include "Sheet.h"
#include "SheetObserver.h"
#include "Utils.h"
#include "Range.h"
#include "SheetPy.h"
#include <ostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <boost/regex.hpp>
#include <boost/bind.hpp>
#include <deque>

using namespace Spreadsheet;
using namespace App;

PROPERTY_SOURCE(Spreadsheet::Sheet, App::DocumentObject)

typedef boost::adjacency_list <
boost::vecS,           // class OutEdgeListS  : a Sequence or an AssociativeContainer
boost::vecS,           // class VertexListS   : a Sequence or a RandomAccessContainer
boost::directedS,      // class DirectedS     : This is a directed graph
boost::no_property,    // class VertexProperty:
boost::no_property,    // class EdgeProperty:
boost::no_property,    // class GraphProperty:
boost::listS           // class EdgeListS:
> DependencyList;
typedef boost::graph_traits<DependencyList> Traits;
typedef Traits::vertex_descriptor Vertex;
typedef Traits::edge_descriptor Edge;

/**
  * Construct a new Sheet object.
  */

Sheet::Sheet()
    : App::DocumentObject()
    , props(this)
    , cells(this)
{
    ADD_PROPERTY_TYPE(docDeps, (0), "Spreadsheet", (App::PropertyType)(App::Prop_Transient|App::Prop_ReadOnly|App::Prop_Hidden), "Dependencies");
    ADD_PROPERTY_TYPE(cells, (), "Spreadsheet", (App::PropertyType)(App::Prop_ReadOnly|App::Prop_Hidden), "Cell contents");
    ADD_PROPERTY_TYPE(columnWidths, (), "Spreadsheet", (App::PropertyType)(App::Prop_ReadOnly|App::Prop_Hidden), "Column widths");
    ADD_PROPERTY_TYPE(rowHeights, (), "Spreadsheet", (App::PropertyType)(App::Prop_ReadOnly|App::Prop_Hidden), "Row heights");
    ADD_PROPERTY_TYPE(currRow, (0), "Spreadsheet", (App::PropertyType)(App::Prop_ReadOnly|App::Prop_Hidden), "Current row");
    ADD_PROPERTY_TYPE(currColumn, (0), "Spreadsheet", (App::PropertyType)(App::Prop_ReadOnly|App::Prop_Hidden), "Current column");

    docDeps.setSize(0);

    onRenamedDocumentConnection = App::GetApplication().signalRenameDocument.connect(boost::bind(&Spreadsheet::Sheet::onRenamedDocument, this, _1));
    onRelabledDocumentConnection = App::GetApplication().signalRelabelDocument.connect(boost::bind(&Spreadsheet::Sheet::onRelabledDocument, this, _1));
}

Sheet::~Sheet()
{
    clearAll();
}

/**
  * Clear all cells in the sheet.
  */

void Sheet::clearAll()
{
    cells.clear();

    std::vector<std::string> propNames = props.getDynamicPropertyNames();

    for (std::vector<std::string>::const_iterator i = propNames.begin(); i != propNames.end(); ++i)
        props.removeDynamicProperty((*i).c_str());

    propAddress.clear();

    for (ObserverMap::iterator i = observers.begin(); i != observers.end(); ++i)
        delete i->second;
}

/**
  * Import a file into the spreadsheet object.
  *
  * @param filename   Name of file to import
  * @param delimiter  The field delimiter charater used.
  * @param quoteChar  Quote character, if any (set to '\0' to disable).
  * @param escapeChar The escape character used, if any (set to '0' to disable).
  *
  * @returns True if successful, false if something failed.
  */

bool Sheet::importFromFile(const std::string &filename, char delimiter, char quoteChar, char escapeChar)
{
    std::ifstream file;
    int row = 0;

    PropertySheet::Signaller signaller(cells);

    clearAll();

    file.open(filename.c_str(), std::ios::in);

    if (file.is_open()) {
        std::string line;

        while (std::getline(file, line)) {
            using namespace boost;

            try {
                escaped_list_separator<char> e;
                int col = 0;

                if (quoteChar)
                    e = escaped_list_separator<char>(escapeChar, delimiter, quoteChar);
                else
                    e = escaped_list_separator<char>('\0', delimiter, '\0');

                tokenizer<escaped_list_separator<char> > tok(line, e);

                for(tokenizer<escaped_list_separator<char> >::iterator i = tok.begin(); i != tok.end();++i) {
                    if ((*i).size() > 0)
                        setCell(CellAddress(row, col), (*i).c_str());
                    col++;
                }
            }
            catch (...) {
                return false;
            }

            ++row;
        }
        file.close();
        return true;
    }
    else
        return false;
}

/**
  * Write an escaped version of the string \a s to the stream \a out.
  *
  * @param s          The string to write.
  * @param quoteChar  The quote character.
  * @param escapeChar The escape character.
  * @param stream     The stream to output the escaped string to.
  *
  */

static void writeEscaped(std::string const& s, char quoteChar, char escapeChar, std::ostream & out) {
  out << quoteChar;
  for (std::string::const_iterator i = s.begin(), end = s.end(); i != end; ++i) {
    unsigned char c = *i;
    if (c != quoteChar)
        out << c;
    else {
        out << escapeChar;
        out << c;
    }
  }
  out << quoteChar;
}

/**
  * Export spreadsheet data to file.
  *
  * @param filename   Filename to store data to.
  * @param delimiter  Field delimiter
  * @param quoteChar  Quote character ('\0' to disable)
  * @param escapeChar Escape character ('\0' to disable)
  *
  * @returns True if store is successful, false if something failed.
  *
  */

bool Sheet::exportToFile(const std::string &filename, char delimiter, char quoteChar, char escapeChar) const
{
    std::ofstream file;
    int prevRow = -1, prevCol = -1;

    file.open(filename.c_str(), std::ios::out | std::ios::ate | std::ios::binary);

    if (!file.is_open())
        return false;

    std::set<CellAddress> usedCells = cells.getUsedCells();
    std::set<CellAddress>::const_iterator i = usedCells.begin();

    while (i != usedCells.end()) {
        Property * prop = getProperty(*i);

        if (prevRow != -1 && prevRow != i->row()) {
            for (int j = prevRow; j < i->row(); ++j)
                file << std::endl;
            prevCol = 0;
        }
        if (prevCol != -1 && i->col() != prevCol) {
            for (int j = prevCol; j < i->col(); ++j)
                file << delimiter;
        }

        std::stringstream field;

        if (prop->isDerivedFrom((PropertyQuantity::getClassTypeId())))
            field << static_cast<PropertyQuantity*>(prop)->getValue();
        else if (prop->isDerivedFrom((PropertyFloat::getClassTypeId())))
            field << static_cast<PropertyFloat*>(prop)->getValue();
        else if (prop->isDerivedFrom((PropertyString::getClassTypeId())))
            field << static_cast<PropertyString*>(prop)->getValue();
        else
            assert(0);

        std::string str = field.str();

        if (quoteChar && str.find(quoteChar) != std::string::npos)
            writeEscaped(str, quoteChar, escapeChar, file);
        else
            file << str;

        prevRow = i->row();
        prevCol = i->col();
        ++i;
    }
    file << std::endl;
    file.close();

    return true;
}

/**
  * Merge a rectangle specified by \a from and \a to into one logical cell.
  * Data in all but the upper right cell are cleared when the cells are merged.
  *
  * @returns True if the cells were merged, false if the merge was unsuccessful.
  *
  */

bool Sheet::mergeCells(const Range & range)
{
    return cells.mergeCells(range.from(), range.to());
}

/**
  * Split a previously merged cell specified by \a address into its individual cells.
  * The address can point to any of the cells that make up the merged cell.
  *
  * @param address Address of cell to split
  *
  */

void Sheet::splitCell(CellAddress address)
{
    cells.splitCell(address);
}

/**
  * Get contents of the cell specified by \a key, or 0 if it is not defined
  *
  * @returns A CellContent object or 0.
  */

Cell *Sheet::getCell(CellAddress address)
{
    return cells.getValue(address);
}

/**
  * Get cell contents specified by (\a row, \a col).
  *
  */

Cell *Sheet::getNewCell(CellAddress address)
{
     Cell * cell = getCell(address);

    if (cell == 0)
        cell = cells.createCell(address);

    return cell;
}

/**
  * Convert given \a key address to string.
  *
  * @param key Address of cell.
  *
  * @returns Address given as a string.
  */

std::string Sheet::toAddress(CellAddress key)
{
    return Spreadsheet::addressToString(key);
}

/**
  * Set cell given by \a address to \a contents.
  *
  * @param address  Address of cell to set.
  * @param contents New contents of given cell.
  *
  */

void Sheet::setCell(const char * address, const char * contents)
{
    assert(address != 0 &&  contents != 0);

    setCell(CellAddress(address), contents);
}

/**
  * Set cell at \a row, \a col to \a expression. The original string given by the user
  * is specified in \a value. If a merged cell is specified, the upper right corner of the
  * merged cell must be specified.
  *
  * @param row        Row position of cell.
  * @param col        Column position of cell.
  * @param value      String value of original expression.
  *
  */

void Sheet::setCell(CellAddress address, const char * value)
{
    assert(value != 0);


    if (*value == '\0') {
        clear(address, false);
        return;
    }

    // Update expression, delete old first if necessary
    Cell * cell = getNewCell(address);

    if (cell->getExpression()) {
        setContent(address, 0);
    }
    setContent(address, value);

    // Recompute dependencies
    touch();
}

/**
  * Get the Python object for the Sheet.
  *
  * @returns The Python object.
  */

PyObject *Sheet::getPyObject(void)
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new SheetPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

/**
  * Get the Cell Property for the cell at \a key.
  *
  * @returns The Property object.
  *
  */

Property * Sheet::getProperty(CellAddress key) const
{
    return props.getDynamicPropertyByName(toAddress(key).c_str());
}

Property * Sheet::getProperty(const char * addr) const
{
    return props.getDynamicPropertyByName(addr);
}

/**
  * Get the address as (\a row, \a col) of the Property \a prop. This function
  * throws an exception if the property is not found.
  *
  */

void Sheet::getCellAddress(const Property *prop, CellAddress & address)
{
    std::map<const App::Property*, CellAddress >::const_iterator i = propAddress.find(prop);

    if (i != propAddress.end())
        address = i->second;
    else
        throw Base::Exception("Property is not a cell");
}

std::map<int, int> Sheet::getColumnWidths() const
{
    return columnWidths.getValues();
}

std::map<int, int> Sheet::getRowHeights() const
{
    return rowHeights.getValues();
}

void Sheet::setPosition(CellAddress address)
{
    currRow.setValue(address.row());
    currColumn.setValue(address.col());
    currRow.purgeTouched();
    currColumn.purgeTouched();
}

/**
  * Set the property for cell \key to a PropertyFloat with the value \a value.
  * If the Property exists, but of wrong type, the previous Property is destroyed and recreated as the correct type.
  *
  * @param key   The address of the cell we want to create a Property for
  * @param value The value we want to assign to the Property.
  *
  */

Property * Sheet::setFloatProperty(CellAddress key, double value)
{
    Property * prop = props.getPropertyByName(toAddress(key).c_str());
    PropertyFloat * floatProp;

    if (!prop || prop->getTypeId() != PropertyFloat::getClassTypeId()) {
        if (prop) {
            props.removeDynamicProperty(toAddress(key).c_str());
            propAddress.erase(prop);

            std::map<CellAddress, App::Property*>::iterator i = aliasProp.find(key);
            if (i != aliasProp.end()) {
                props.removeDynamicProperty(props.getPropertyName(i->second));
                aliasProp.erase(i);
            }
        }
        floatProp = freecad_dynamic_cast<PropertyFloat>(props.addDynamicProperty("App::PropertyFloat", toAddress(key).c_str(), 0, 0, Prop_ReadOnly | Prop_Transient, true, true));
        floatProp->StatusBits.set(3);
    }
    else
        floatProp = static_cast<PropertyFloat*>(prop);

    propAddress[floatProp] = key;
    floatProp->setValue(value);

    return floatProp;
}

/**
  * Set the property for cell \key to a PropertyQuantity with \a value and \a unit.
  * If the Property exists, but of wrong type, the previous Property is destroyed and recreated as the correct type.
  *
  * @param key   The address of the cell we want to create a Property for
  * @param value The value we want to assign to the Property.
  * @param unit  The associated unit for \a value.
  *
  */

Property * Sheet::setQuantityProperty(CellAddress key, double value, const Base::Unit & unit)
{
    Property * prop = props.getPropertyByName(toAddress(key).c_str());
    PropertyQuantity * quantityProp;

    if (!prop || prop->getTypeId() != PropertyQuantity::getClassTypeId()) {
        if (prop) {
            props.removeDynamicProperty(toAddress(key).c_str());
            propAddress.erase(prop);

            std::map<CellAddress, App::Property*>::iterator i = aliasProp.find(key);
            if (i != aliasProp.end()) {
                props.removeDynamicProperty(props.getPropertyName(i->second));
                aliasProp.erase(i);
            }
        }
        Property * p = props.addDynamicProperty("App::PropertyQuantity", toAddress(key).c_str(), 0, 0, Prop_ReadOnly | Prop_Transient, true, true);
        quantityProp = freecad_dynamic_cast<PropertyQuantity>(p);
        quantityProp->StatusBits.set(3);
    }
    else
       quantityProp = static_cast<PropertyQuantity*>(prop);

    propAddress[quantityProp] = key;
    quantityProp->setValue(value);
    quantityProp->setUnit(unit);

    cells.setComputedUnit(key, unit);

    return quantityProp;
}

/**
  * Set the property for cell \key to a PropertyString with \a value.
  * If the Property exists, but of wrong type, the previous Property is destroyed and recreated as the correct type.
  *
  * @param key   The address of the cell we want to create a Property for
  * @param value The value we want to assign to the Property.
  *
  */

Property * Sheet::setStringProperty(CellAddress key, const std::string & value)
{
    Property * prop = props.getPropertyByName(toAddress(key).c_str());
    PropertyString * stringProp = freecad_dynamic_cast<PropertyString>(prop);

    if (!stringProp) {
        if (prop) {
            props.removeDynamicProperty(toAddress(key).c_str());
            propAddress.erase(prop);

            std::map<CellAddress, App::Property*>::iterator i = aliasProp.find(key);
            if (i != aliasProp.end()) {
                props.removeDynamicProperty(props.getPropertyName(i->second));
                aliasProp.erase(i);
            }
        }
        stringProp = freecad_dynamic_cast<PropertyString>(props.addDynamicProperty("App::PropertyString", toAddress(key).c_str(), 0, 0, Prop_ReadOnly | Prop_Transient, true, true));
        stringProp->StatusBits.set(3);
    }

    propAddress[stringProp] = key;
    stringProp->setValue(value.c_str());

    return stringProp;
}

/**
  * Update the Propery given by \a key. This will also eventually trigger recomputations of cells depending on \a key.
  *
  * @param key The address of the cell we want to recompute.
  *
  */

void Sheet::updateProperty(CellAddress key)
{
    const Property * prop;
    const char * aliasType = 0;

    Cell * cell = getCell(key);
    std::string alias;

    if (cell != 0) {
        Expression * output;
        const Expression * input = cell->getExpression();

        if (input) {
            output = input->eval();
        }
        else {
            std::string s;

            if (cell->getStringContent(s))
                output = new StringExpression(this, s);
            else
                output = new StringExpression(this, "");
        }

        /* Eval returns either NumberExpression or StringExpression objects */
        if (freecad_dynamic_cast<NumberExpression>(output)) {
            NumberExpression * number = static_cast<NumberExpression*>(output);
            if (number->getUnit().isEmpty()) {
                prop = setFloatProperty(key, number->getValue());
                aliasType = "App::PropertyFloat";
            }
            else {
                prop = setQuantityProperty(key, number->getValue(), number->getUnit());
                aliasType = "App::PropertyQuantity";
            }
        }
        else {
            prop = setStringProperty(key, freecad_dynamic_cast<StringExpression>(output)->getText().c_str());
            aliasType = "App::PropertyString";
        }

        delete output;
    }
    else
        clear(key);

    if (cell && cell->getAlias(alias)) {
        /* Update or create alias? */
        if (aliasProp.find(key) == aliasProp.end())
            aliasProp[key] = props.addDynamicProperty(aliasType, alias.c_str(), 0, 0, Prop_ReadOnly | Prop_Transient, true, true);
        aliasProp[key]->Paste(*prop);
    }
    else {
        /* Remove alias? */
        std::map<CellAddress, App::Property*>::iterator i = aliasProp.find(key);
        if (i != aliasProp.end()) {
            props.removeDynamicProperty(props.getPropertyName(i->second));
            aliasProp.erase(i);
        }
    }

    cellUpdated(key);
}

/**
  * Retrieve a specifc Property given by \a name.
  * This function might throw an exception if something fails, but might also
  * return 0 in case the property is not found.
  *
  * @returns The property, or 0 if not found.
  *
  */

Property *Sheet::getPropertyByName(const char* name) const
{
    Property * prop = getProperty(name);

    if (prop)
        return prop;
    else
        return DocumentObject::getPropertyByName(name);
}

const char *Sheet::getPropertyName(const Property *prop) const
{
    const char * name = props.getPropertyName(prop);

    if (name)
        return name;
    else
        return PropertyContainer::getPropertyName(prop);
}

void Sheet::recomputeCell(CellAddress p)
{
    Cell * cell = cells.getValue(p);
    std::string docName = getDocument()->Label.getValue();
    std::string docObjName = std::string(getNameInDocument());
    std::string name = docName + "#" + docObjName + "." + toAddress(p);

    try {
        if (cell) {
            cell->clearException();
            cell->clearResolveException();
        }
        updateProperty(p);
        cells.clearDirty(p);
        cellErrors.erase(p);
    }
    catch (const Base::Exception & e) {
        QString msg = QString::fromUtf8("ERR: %1").arg(QString::fromUtf8(e.what()));

        setStringProperty(p, msg.toStdString());
        if (cell)
            cell->setException(e.what());

        // Mark as erronous
        cellErrors.insert(p);
    }

    if (!cell || cell->spansChanged())
        cellSpanChanged(p);
}

/**
  * Update the document properties.
  *
  */

App::DocumentObjectExecReturn *Sheet::execute(void)
{
    // Get dirty cells that we have to recompute
    std::set<CellAddress> dirtyCells = cells.getDirty();

    // Always recompute cells that have failed
    for (std::set<CellAddress>::const_iterator i = cellErrors.begin(); i != cellErrors.end(); ++i) {
         cells.recomputeDependencies(*i);
         dirtyCells.insert(*i);
    }

    // Push dirty cells onto queue
    for (std::set<CellAddress>::const_iterator i = dirtyCells.begin(); i != dirtyCells.end(); ++i) {
        // Create queue and a graph structure to compute order of evaluation
        std::deque<CellAddress> workQueue;
        DependencyList graph;
        std::map<CellAddress, Vertex> VertexList;
        std::map<Vertex, CellAddress> VertexIndexList;

        workQueue.push_back(*i);

        while (workQueue.size() > 0) {
            CellAddress currPos = workQueue.front();
            std::set<CellAddress> s;

            // Get other cells that depends on the current cell (currPos)
            providesTo(currPos, s);
            workQueue.pop_front();

            // Insert into map of CellPos -> Index, if it doesn't exist already
            if (VertexList.find(currPos) == VertexList.end()) {
                VertexList[currPos] = add_vertex(graph);
                VertexIndexList[VertexList[currPos]] = currPos;
            }

            // Process cells that depend on the current cell
            std::set<CellAddress>::const_iterator i = s.begin();
            while (i != s.end()) {
                // Insert into map of CellPos -> Index, if it doesn't exist already
                if (VertexList.find(*i) == VertexList.end()) {
                    VertexList[*i] = add_vertex(graph);
                    VertexIndexList[VertexList[*i]] = *i;
                    workQueue.push_back(*i);
                }
                // Add edge to graph to signal dependency
                add_edge(VertexList[currPos], VertexList[*i], graph);
                ++i;
            }
        }

        // Compute cells
        std::list<Vertex> make_order;

        // Sort graph topologically to find evaluation order
        try {
            boost::topological_sort(graph, std::front_inserter(make_order));

            // Recompute cells
            std::list<Vertex>::const_iterator i = make_order.begin();
            while (i != make_order.end()) {
                recomputeCell(VertexIndexList[*i]);
                ++i;
            }
        }
        catch (std::exception) {
            // Cycle detected; flag all with errors

            std::map<CellAddress, Vertex>::const_iterator i = VertexList.begin();
            while (i != VertexList.end()) {
                Cell * cell = cells.getValue(i->first);

                // Mark as erronous
                cellErrors.insert(i->first);

                if (cell)
                    cell->setException("Circular dependency.");
                updateProperty(i->first);
                ++i;
            }
        }

    }

    // Signal update of column widths
    const std::set<int> & dirtyColumns = columnWidths.getDirty();

    for (std::set<int>::const_iterator i = dirtyColumns.begin(); i != dirtyColumns.end(); ++i)
        columnWidthChanged(*i, columnWidths.getValue(*i));

    // Signal update of row heights
    const std::set<int> & dirtyRows = rowHeights.getDirty();

    for (std::set<int>::const_iterator i = dirtyRows.begin(); i != dirtyRows.end(); ++i)
        rowHeightChanged(*i, rowHeights.getValue(*i));

    //cells.clearDirty();
    rowHeights.clearDirty();
    columnWidths.clearDirty();

    positionChanged(CellAddress(currRow.getValue(), currColumn.getValue()));

    currRow.purgeTouched();
    currColumn.purgeTouched();

    std::set<App::DocumentObject*> ds(cells.getDocDeps());
    std::vector<App::DocumentObject*> dv(ds.begin(), ds.end());
    docDeps.setValues(dv);

    purgeTouched();

    if (cellErrors.size() == 0)
        return App::DocumentObject::StdReturn;
    else
        return new DocumentObjectExecReturn("One or more cells failed contains errors.", this);
}

/**
  * Unimplemented.
  *
  */

short Sheet::mustExecute(void) const
{
    if (cellErrors.size() > 0 || cells.isTouched() || columnWidths.isTouched() || rowHeights.isTouched())
        return 1;
    else if (cells.getDocDeps().size() == 0)
        return 0;
    else
        return -1;
}



/**
  * Clear the cell at \a address. If \a all is false, only the text or expression
  * contents are cleared. If \a all is true everything in the cell
  * is cleared, including color, style, etc.
  *
  * @param address Address of cell to clear
  * @param all     Whether we should clear all or not.
  *
  */

void Sheet::clear(CellAddress address, bool all)
{
    std::string addr = toAddress(address);
    Property * prop = props.getDynamicPropertyByName(addr.c_str());

    cells.clear(address);

    propAddress.erase(prop);
    props.removeDynamicProperty(addr.c_str());
}

/**
  * Get row an column span for the cell at (row, col).
  *
  * @param row  Row address of cell
  * @param col  Column address of cell
  * @param rows The number of unit cells this cell spans
  * @param cols The number of unit rows this cell spans
  *
  */

void Sheet::getSpans(CellAddress address, int &rows, int &cols) const
{
    return cells.getSpans(address, rows, cols);
}

/**
  * Determine whether this cell is merged with another or not.
  *
  * @param row
  * @param col
  *
  * @returns True if cell is merged, false if not.
  *
  */

bool Sheet::isMergedCell(CellAddress address) const
{
    return cells.isMergedCell(address);
}

void Sheet::setColumnWidth(int col, int width)
{
    columnWidths.setValue(col, width);
}

int Sheet::getColumnWidth(int col) const
{
    return columnWidths.getValue(col);
}

void Sheet::setRowHeight(int row, int height)
{
    rowHeights.setValue(row, height);
}

int Sheet::getRowHeight(int row) const
{
    return rowHeights.getValue(row);
}

/**
  * Get a vector of strings with addresses of all used cells.
  *
  * @returns vector of strings.
  *
  */

std::vector<std::string> Sheet::getUsedCells() const
{
    std::vector<std::string> usedCells;

    // Insert int usedSet
    std::set<CellAddress> usedSet = cells.getUsedCells();

    for (std::set<CellAddress>::const_iterator i = usedSet.begin(); i != usedSet.end(); ++i)
        usedCells.push_back(toAddress(*i));

    return usedCells;
}

/**
  * Insert \a count columns at before column \a col in the spreadsheet.
  *
  * @param col   Address of column where the columns are inserted.
  * @param count Number of columns to insert
  *
  */

void Sheet::insertColumns(int col, int count)
{

    cells.insertColumns(col, count);
}

/**
  * Remove \a count columns at column \a col.
  *
  * @param col   Address of first column to remove.
  * @param count Number of columns to remove.
  *
  */

void Sheet::removeColumns(int col, int count)
{
    cells.removeColumns(col, count);
}

/**
  * Inser \a count rows at row \a row.
  *
  * @param row   Address of row where the rows are inserted.
  * @param count Number of rows to insert.
  *
  */

void Sheet::insertRows(int row, int count)
{
    cells.insertRows(row, count);
}

/**
  * Remove \a count rows starting at \a row from the spreadsheet.
  *
  * @param row   Address of first row to remove.
  * @param count Number of rows to remove.
  *
  */

void Sheet::removeRows(int row, int count)
{
    cells.removeRows(row, count);
}

void Sheet::setContent(CellAddress address, const char *value)
{
    cells.setContent(address, value);
}

void Sheet::setAlignment(CellAddress address, int _alignment)
{
    cells.setAlignment(address, _alignment);
}

void Sheet::setStyle(CellAddress address, const std::set<std::string> &_style)
{
    cells.setStyle(address, _style);
}

void Sheet::setForeground(CellAddress address, const Color &color)
{
    cells.setForeground(address, color);
}

void Sheet::setBackground(CellAddress address, const Color &color)
{
    cells.setBackground(address, color);
}

void Sheet::setDisplayUnit(CellAddress address, const std::string &unit)
{
    cells.setDisplayUnit(address, unit);
}

void Sheet::setComputedUnit(CellAddress address, const Base::Unit &unit)
{
    cells.setComputedUnit(address, unit);
}

void Sheet::setAlias(CellAddress address, const std::string &alias)
{
    cells.setAlias(address, alias);
}

void Sheet::setSpans(CellAddress address, int rows, int columns)
{
    cells.setSpans(address, rows, columns);
}

/**
  * Move a cell from \a currPos to \a newPos. If the cell at new position
  * contains data, it is overwritten by the move.
  *
  */

void Sheet::moveCell(CellAddress currPos, CellAddress newPos)
{
    cells.moveCell(currPos, newPos);
}

void Sheet::renamedDocumentObject(const App::DocumentObject * docObj)
{
    cells.renamedDocumentObject(docObj);
    cells.touch();
}

std::set<std::string> Sheet::dependsOn(CellAddress address) const
{
    return cells.getDeps(address);
}

void Sheet::providesTo(CellAddress address, std::set<std::string> & result) const
{
    const char * docName = getDocument()->Label.getValue();
    const char * docObjName = getNameInDocument();
    std::string fullName = std::string(docName) + "#" + std::string(docObjName) + "." + toAddress(address);
    std::set<CellAddress> tmpResult = cells.getDeps(fullName);

    for (std::set<CellAddress>::const_iterator i = tmpResult.begin(); i != tmpResult.end(); ++i)
        result.insert(std::string(docName) + "#" + std::string(docObjName) + "." + toAddress(*i));
}

void Sheet::providesTo(CellAddress address, std::set<CellAddress> & result) const
{
    const char * docName = getDocument()->Label.getValue();
    const char * docObjName = getNameInDocument();
    std::string fullName = std::string(docName) + "#" + std::string(docObjName) + "." + toAddress(address);
    result = cells.getDeps(fullName);
}

void Sheet::onDocumentRestored()
{
    cells.resolveAll();
    execute();
}

void Sheet::onRelabledDocument(const Document &document)
{
    cells.renamedDocument(&document);
    cells.purgeTouched();
}

void Sheet::onRenamedDocument(const Document &document)
{
}

void Sheet::observeDocument(Document * document)
{
    ObserverMap::const_iterator it = observers.find(document->getName());

    if (it != observers.end()) {
        // An observer already exists, increase reference counter for it
        it->second->ref();
    }
    else {
        // Create a new observer
        SheetObserver * observer = new SheetObserver(document, &cells);

        observers[document->getName()] = observer;
    }
}
