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

#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/assign.hpp>
#include <boost/graph/topological_sort.hpp>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DynamicProperty.h>
#include <App/FeaturePythonPyImp.h>
#include <App/ExpressionParser.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Placement.h>
#include <Base/Reader.h>
#include <Base/Stream.h>
#include <Base/Writer.h>
#include <Base/Tools.h>
#include <Base/Console.h>
#include "Sheet.h"
#include "SheetObserver.h"
#include "Utils.h"
#include "SheetPy.h"
#include <ostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <boost/regex.hpp>
#include <boost/bind.hpp>
#include <deque>

FC_LOG_LEVEL_INIT("Spreadsheet",true,true);

using namespace Base;
using namespace App;
using namespace Spreadsheet;

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
    : DocumentObject()
    , props(this)
    , cells(this)
{
    ADD_PROPERTY_TYPE(cells, (), "Spreadsheet", (PropertyType)(Prop_Hidden), "Cell contents");
    ADD_PROPERTY_TYPE(columnWidths, (), "Spreadsheet", (PropertyType)(Prop_ReadOnly|Prop_Hidden|Prop_Output), "Column widths");
    ADD_PROPERTY_TYPE(rowHeights, (), "Spreadsheet", (PropertyType)(Prop_ReadOnly|Prop_Hidden|Prop_Output), "Row heights");
    ADD_PROPERTY_TYPE(hiddenRows, (), "Spreadsheet", (PropertyType)(Prop_Hidden|Prop_Output), "Hidden rows");
    ADD_PROPERTY_TYPE(hiddenColumns, (), "Spreadsheet", (PropertyType)(Prop_Hidden|Prop_Output), "Hidden columns");
    ADD_PROPERTY_TYPE(PythonMode, (false), "Spreadsheet", Prop_None, "Set default expression syntax mode");
}

/**
 * @brief Sheet::~Sheet
 *
 * The destructor; clear properties to release all memory.
 *
 */

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
        this->removeDynamicProperty((*i).c_str());

    propAddress.clear();
    cellErrors.clear();
    columnWidths.clear();
    rowHeights.clear();
    removedAliases.clear();

    for (ObserverMap::iterator i = observers.begin(); i != observers.end(); ++i)
        delete i->second;
    observers.clear();
}

/**
  * Import a file into the spreadsheet object.
  *
  * @param filename   Name of file to import
  * @param delimiter  The field delimiter character used.
  * @param quoteChar  Quote character, if any (set to '\0' to disable).
  * @param escapeChar The escape character used, if any (set to '0' to disable).
  *
  * @returns True if successful, false if something failed.
  */

bool Sheet::importFromFile(const std::string &filename, char delimiter, char quoteChar, char escapeChar)
{
    Base::FileInfo fi(filename);
    Base::ifstream file(fi, std::ios::in);
    int row = 0;

    PropertySheet::AtomicPropertyChange signaller(cells);

    clearAll();

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
                signaller.tryInvoke();
                return false;
            }

            ++row;
        }
        file.close();
        signaller.tryInvoke();
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
        else if (prop->isDerivedFrom((PropertyInteger::getClassTypeId())))
            field << static_cast<PropertyInteger*>(prop)->getValue();
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
  * Merge a rectangle specified by \a range into one logical cell.
  * Data in all but the upper right cell are cleared when the cells are merged.
  *
  * @param range Range to merge.
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
  * Get contents of the cell specified by \a address, or 0 if it is not defined
  *
  * @returns A CellContent object or 0.
  */

Cell *Sheet::getCell(CellAddress address)
{
    return cells.getValue(address);
}

/**
  * Get cell contents specified by \a address.
  *
  * @param address
  */

Cell *Sheet::getNewCell(CellAddress address)
{
     Cell * cell = getCell(address);

    if (cell == 0)
        cell = cells.createCell(address);

    return cell;
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
  * Set cell at \a address to \a value. If a merged cell is specified, the upper right corner of the
  * merged cell must be specified.
  *
  * @param address    Row position of cell.
  * @param value      String value of expression.
  *
  */

void Sheet::setCell(CellAddress address, const char * value)
{
    assert(value != 0);


    if (*value == '\0') {
        clear(address, false);
        return;
    }

    setContent(address, value);
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
    return props.getDynamicPropertyByName(key.toString().c_str());
}

/**
 * @brief Get a dynamic property.
 * @param addr Name of dynamic propeerty.
 * @return Pointer to property, or 0 if it does not exist.
 */

Property * Sheet::getProperty(const char * addr) const
{
    return props.getDynamicPropertyByName(addr);
}

/**
  * Get the address as \a address of the Property \a prop. This function
  * throws an exception if the property is not found.
  *
  */

bool Sheet::getCellAddress(const Property *prop, CellAddress & address)
{
    std::map<const Property*, CellAddress >::const_iterator i = propAddress.find(prop);

    if (i != propAddress.end()) {
        address = i->second;
        return true;
    }
    return false;
}

/**
 * @brief Get a map with column indices and widths.
 * @return Map with results.
 */

std::map<int, int> Sheet::getColumnWidths() const
{
    return columnWidths.getValues();
}

/**
 * @brief Get a map with row indices and heights.
 * @return Map with results
 */

std::map<int, int> Sheet::getRowHeights() const
{
    return rowHeights.getValues();
}


/**
 * @brief Remove all aliases.
 *
 */

void Sheet::removeAliases()
{
    std::map<CellAddress, std::string>::iterator i = removedAliases.begin();

    while (i != removedAliases.end()) {
        this->removeDynamicProperty(i->second.c_str());
        ++i;
    }
    removedAliases.clear();
}

/**
 * Update internal structure when document is set for this property.
 */

void Sheet::onSettingDocument()
{
    cells.documentSet();
}

/**
  * Set the property for cell \p key to a PropertyFloat with the value \a value.
  * If the Property exists, but of wrong type, the previous Property is destroyed and recreated as the correct type.
  *
  * @param key   The address of the cell we want to create a Property for
  * @param value The value we want to assign to the Property.
  *
  */

Property * Sheet::setFloatProperty(CellAddress key, double value)
{
    Property * prop = props.getPropertyByName(key.toString().c_str());
    PropertyFloat * floatProp;

    if (!prop || prop->getTypeId() != PropertyFloat::getClassTypeId()) {
        if (prop) {
            this->removeDynamicProperty(key.toString().c_str());
            propAddress.erase(prop);
        }
        floatProp = freecad_dynamic_cast<PropertyFloat>(props.addDynamicProperty("App::PropertyFloat", key.toString().c_str(), 0, 0, Prop_ReadOnly | Prop_Hidden | Prop_Transient));
    }
    else
        floatProp = static_cast<PropertyFloat*>(prop);

    propAddress[floatProp] = key;
    floatProp->setValue(value);

    return floatProp;
}

Property * Sheet::setIntegerProperty(CellAddress key, long value)
{
    Property * prop = props.getPropertyByName(key.toString().c_str());
    PropertyInteger * intProp;

    if (!prop || prop->getTypeId() != PropertyInteger::getClassTypeId()) {
        if (prop) {
            this->removeDynamicProperty(key.toString().c_str());
            propAddress.erase(prop);
        }
        intProp = freecad_dynamic_cast<PropertyInteger>(props.addDynamicProperty(
                    "App::PropertyInteger", key.toString().c_str(), 0, 0, 
                    Prop_ReadOnly | Prop_Hidden | Prop_Transient));
    }
    else
        intProp = static_cast<PropertyInteger*>(prop);

    propAddress[intProp] = key;
    intProp->setValue(value);

    return intProp;
}


/**
  * Set the property for cell \p key to a PropertyQuantity with \a value and \a unit.
  * If the Property exists, but of wrong type, the previous Property is destroyed and recreated as the correct type.
  *
  * @param key   The address of the cell we want to create a Property for
  * @param value The value we want to assign to the Property.
  * @param unit  The associated unit for \a value.
  *
  */

Property * Sheet::setQuantityProperty(CellAddress key, double value, const Base::Unit & unit)
{
    Property * prop = props.getPropertyByName(key.toString().c_str());
    PropertySpreadsheetQuantity * quantityProp;

    if (!prop || prop->getTypeId() != PropertySpreadsheetQuantity::getClassTypeId()) {
        if (prop) {
            this->removeDynamicProperty(key.toString().c_str());
            propAddress.erase(prop);
        }
        Property * p = props.addDynamicProperty("Spreadsheet::PropertySpreadsheetQuantity", key.toString().c_str(), 0, 0, Prop_ReadOnly | Prop_Hidden | Prop_Transient);
        quantityProp = freecad_dynamic_cast<PropertySpreadsheetQuantity>(p);
    }
    else
       quantityProp = static_cast<PropertySpreadsheetQuantity*>(prop);

    propAddress[quantityProp] = key;
    quantityProp->setValue(value);
    quantityProp->setUnit(unit);

    cells.setComputedUnit(key, unit);

    return quantityProp;
}

/**
  * Set the property for cell \p key to a PropertyString with \a value.
  * If the Property exists, but of wrong type, the previous Property is destroyed and recreated as the correct type.
  *
  * @param key   The address of the cell we want to create a Property for
  * @param value The value we want to assign to the Property.
  *
  */

Property * Sheet::setStringProperty(CellAddress key, const std::string & value)
{
    Property * prop = props.getPropertyByName(key.toString().c_str());
    PropertyString * stringProp = freecad_dynamic_cast<PropertyString>(prop);

    if (!stringProp) {
        if (prop) {
            this->removeDynamicProperty(key.toString().c_str());
            propAddress.erase(prop);
        }
        stringProp = freecad_dynamic_cast<PropertyString>(props.addDynamicProperty("App::PropertyString", key.toString().c_str(), 0, 0, Prop_ReadOnly | Prop_Hidden | Prop_Transient));
    }

    propAddress[stringProp] = key;
    stringProp->setValue(value.c_str());

    return stringProp;
}

Property * Sheet::setObjectProperty(CellAddress key, Py::Object object)
{
    Property * prop = props.getPropertyByName(key.toString().c_str());
    PropertyPythonObject * pyProp = freecad_dynamic_cast<PropertyPythonObject>(prop);

    if (!pyProp) {
        if (prop) {
            this->removeDynamicProperty(key.toString().c_str());
            propAddress.erase(prop);
        }
        pyProp = freecad_dynamic_cast<PropertyPythonObject>(props.addDynamicProperty("App::PropertyPythonObject", key.toString().c_str(), 0, 0, Prop_ReadOnly | Prop_Hidden | Prop_Transient));
    }

    propAddress[pyProp] = key;
    pyProp->setValue(object);

    return pyProp;
}

/**
 * @brief Update the alias for the cell at \a key.
 * @param key Cell to update.
 */

void Sheet::updateAlias(CellAddress key)
{
    std::string alias;
    Property * prop = props.getDynamicPropertyByName(key.toString().c_str());

    if (!prop)
        return;

    Cell * cell = getCell(key);

    if (cell && cell->getAlias(alias)) {
        Property * aliasProp = props.getDynamicPropertyByName(alias.c_str());

        /* Update or create alias? */
        if (aliasProp) {
            // Type of alias and property must always be the same
            if (aliasProp->getTypeId() != prop->getTypeId()) {
                this->removeDynamicProperty(alias.c_str());
                aliasProp = 0;
            }
        }

        if (!aliasProp) {
            aliasProp = props.addDynamicProperty(prop->getTypeId().getName(), alias.c_str(), 0, 0, Prop_ReadOnly | Prop_Transient);
            aliasProp->setStatus(App::Property::Hidden,true);
        }

        aliasProp->Paste(*prop);
    }
}

struct CurrentAddressLock {
    CurrentAddressLock(int &r, int &c, const CellAddress &addr)
        :row(r),col(c)
    {
        row = addr.row();
        col = addr.col();
    }
    ~CurrentAddressLock() {
        row = -1;
        col = -1;
    }

    int &row;
    int &col;
};

/**
  * Update the Property given by \a key. This will also eventually trigger recomputations of cells depending on \a key.
  *
  * @param key The address of the cell we want to recompute.
  *
  */

void Sheet::updateProperty(CellAddress key)
{
    Cell * cell = getCell(key);

    if (cell != 0) {
        std::unique_ptr<Expression> output;
        const Expression * input = cell->getExpression();

        if (input) {
            CurrentAddressLock lock(currentRow,currentCol,key);
            output = cells.eval(input);
        }
        else {
            std::string s;

            if (cell->getStringContent(s))
                output = StringExpression::create(this, std::move(s));
            else
                output = StringExpression::create(this, "");
        }

        /* Eval returns either NumberExpression or StringExpression, or
         * PyObjectExpression objects */
        auto number = freecad_dynamic_cast<NumberExpression>(output.get());
        if(number) {
            long l;
            if (!number->getUnit().isEmpty())
                setQuantityProperty(key, number->getValue(), number->getUnit());
            else if(number->isInteger(&l))
                setIntegerProperty(key,l);
            else
                setFloatProperty(key, number->getValue());
        }else{
            auto str_expr = freecad_dynamic_cast<StringExpression>(output.get());
            if(str_expr) 
                setStringProperty(key, str_expr->getText().c_str());
            else {
                Base::PyGILStateLocker lock;
                auto py_expr = freecad_dynamic_cast<PyObjectExpression>(output.get());
                if(py_expr) 
                    setObjectProperty(key, py_expr->getPyObject());
                else
                    setObjectProperty(key, Py::Object());
            }
        }
    }
    else
        clear(key);

    cellUpdated(key);
}

/**
  * Retrieve a specific Property given by \a name.
  * This function might throw an exception if something fails, but might also
  * return 0 in case the property is not found.
  *
  * @returns The property, or 0 if not found.
  *
  */

Property *Sheet::getPropertyByName(const char* name) const
{
    std::string _name;
    CellAddress addr;
    if(addr.parseAbsoluteAddress(name)) {
        _name = addr.toString(true);
        name = _name.c_str();
    }
    Property * prop = getProperty(name);

    if (prop)
        return prop;
    else
        return DocumentObject::getPropertyByName(name);
}

/**
 * @brief Get name of a property, given a pointer to it.
 * @param prop Pointer to property.
 * @return Pointer to string.
 */

const char *Sheet::getPropertyName(const Property *prop) const
{
    const char * name = props.getPropertyName(prop);

    if (name)
        return name;
    else
        return PropertyContainer::getPropertyName(prop);
}

void Sheet::touchCells(Range range) {
    do {
        cells.setDirty(*range);
    }while(range.next());
}

/**
 * @brief Recompute cell at address \a p.
 * @param p Address of cell.
 */

void Sheet::recomputeCell(CellAddress p)
{
    Cell * cell = cells.getValue(p);

    try {
        if (cell && cell->hasException()) {
            std::string content;
            cell->getStringContent(content);
            cell->setContent(content.c_str());
        }

        updateProperty(p);

        if(!cell || !cell->hasException()) {
            cells.clearDirty(p);
            cellErrors.erase(p);
        }
    }
    catch (const Base::Exception & e) {
        QString msg = QString::fromUtf8("ERR: %1").arg(QString::fromUtf8(e.what()));

        setStringProperty(p, Base::Tools::toStdString(msg));
        if (cell)
            cell->setException(e.what());
        else
            e.ReportException();

        // Mark as erroneous
        cellErrors.insert(p);
        cellUpdated(p);

        if(e.isDerivedFrom(Base::AbortException::getClassTypeId()))
            throw;
    }

    updateAlias(p);

    if (!cell || cell->spansChanged())
        cellSpanChanged(p);
}

/**
  * Update the document properties.
  *
  */

DocumentObjectExecReturn *Sheet::execute(void)
{
    // Remove all aliases first
    removeAliases();

    // Get dirty cells that we have to recompute
    std::set<CellAddress> dirtyCells = cells.getDirty();

    // Always recompute cells that have failed
    for (std::set<CellAddress>::const_iterator i = cellErrors.begin(); i != cellErrors.end(); ++i) {
         cells.recomputeDependencies(*i);
         dirtyCells.insert(*i);
    }

    DependencyList graph;
    std::map<CellAddress, Vertex> VertexList;
    std::map<Vertex, CellAddress> VertexIndexList;
    std::deque<CellAddress> workQueue(dirtyCells.begin(),dirtyCells.end());
    while(workQueue.size()) {
        CellAddress currPos = workQueue.front();
        workQueue.pop_front();

        // Insert into map of CellPos -> Index, if it doesn't exist already
        auto res = VertexList.emplace(currPos,Vertex());
        if(res.second) {
            res.first->second = add_vertex(graph);
            VertexIndexList[res.first->second] = currPos;
        }

        // Process cells that depend on the current cell
        for(auto &dep : providesTo(currPos)) {
            auto resDep = VertexList.emplace(dep,Vertex());
            if(resDep.second) {
                resDep.first->second = add_vertex(graph);
                VertexIndexList[resDep.first->second] = dep;
                if(dirtyCells.insert(dep).second)
                    workQueue.push_back(dep);
            }
            // Add edge to graph to signal dependency
            add_edge(res.first->second, resDep.first->second, graph);
        }
    }
    // Compute cells
    std::list<Vertex> make_order;
    // Sort graph topologically to find evaluation order
    try {
        boost::topological_sort(graph, std::front_inserter(make_order));
        // Recompute cells
        FC_LOG("recomputing " << getFullName());
        for(auto &pos : make_order) {
            const auto &addr = VertexIndexList[pos];
            FC_LOG(addr.toString());
            recomputeCell(addr);
        }
    } catch (std::exception&) {
        for(auto &v : VertexList) {
            Cell * cell = cells.getValue(v.first);
            // Mark as erroneous
            cellErrors.insert(v.first);
            if (cell)
                cell->setException("Pending computation due to cyclic dependency");
            updateProperty(v.first);
            updateAlias(v.first);
        }

        // Try to be more user friendly by finding individual loops
        while(dirtyCells.size()) {

            std::deque<CellAddress> workQueue;
            DependencyList graph;
            std::map<CellAddress, Vertex> VertexList;
            std::map<Vertex, CellAddress> VertexIndexList;

            CellAddress currentAddr = *dirtyCells.begin();
            workQueue.push_back(currentAddr);
            dirtyCells.erase(dirtyCells.begin());

            while (workQueue.size() > 0) {
                CellAddress currPos = workQueue.front();
                workQueue.pop_front();

                // Insert into map of CellPos -> Index, if it doesn't exist already
                auto res = VertexList.emplace(currPos,Vertex());
                if(res.second) {
                    res.first->second = add_vertex(graph);
                    VertexIndexList[res.first->second] = currPos;
                }

                // Process cells that depend on the current cell
                for(auto &dep : providesTo(currPos)) {
                    auto resDep = VertexList.emplace(dep,Vertex());
                    if(resDep.second) {
                        resDep.first->second = add_vertex(graph);
                        VertexIndexList[resDep.first->second] = dep;
                        workQueue.push_back(dep);
                        dirtyCells.erase(dep);
                    }
                    // Add edge to graph to signal dependency
                    add_edge(res.first->second, resDep.first->second, graph);
                }
            }

            std::list<Vertex> make_order;
            try {
                boost::topological_sort(graph, std::front_inserter(make_order));
            } catch (std::exception&) {
                // Cycle detected; flag all with errors
                std::ostringstream ss;
                ss << "Cyclic dependency" << std::endl;
                int count = 0;
                for(auto &v : VertexList) {
                    if(count==20)
                        ss << std::endl;
                    else
                        ss << ", ";
                    ss << v.first.toString();
                }
                std::string msg = ss.str();
                for(auto &v : VertexList) {
                    Cell * cell = cells.getValue(v.first);
                    if (cell)
                        cell->setException(msg.c_str());
                }
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

    if (cellErrors.size() == 0)
        return DocumentObject::StdReturn;
    else
        return new DocumentObjectExecReturn("One or more cells failed contains errors.", this);
}

/**
  * Determine whether this object needs to be executed to update internal structures.
  *
  */

short Sheet::mustExecute(void) const
{
    if (cellErrors.size() > 0 || cells.isDirty())
        return 1;
    return DocumentObject::mustExecute();
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

void Sheet::clear(CellAddress address, bool /*all*/)
{
    Cell * cell = getCell(address);
    std::string addr = address.toString();
    Property * prop = props.getDynamicPropertyByName(addr.c_str());

    // Remove alias, if defined
    std::string aliasStr;
    if (cell && cell->getAlias(aliasStr))
        this->removeDynamicProperty(aliasStr.c_str());

    cells.clear(address);

    propAddress.erase(prop);
    this->removeDynamicProperty(addr.c_str());
}

/**
  * Get row an column span for the cell at (row, col).
  *
  * @param address Address of cell
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
  * @param address
  *
  * @returns True if cell is merged, false if not.
  *
  */

bool Sheet::isMergedCell(CellAddress address) const
{
    return cells.isMergedCell(address);
}

/**
 * @brief Set column with of column \a col to \a width-
 * @param col   Index of column.
 * @param width New width of column.
 */

void Sheet::setColumnWidth(int col, int width)
{
    columnWidths.setValue(col, width);
}

/**
 * @brief Get column with of column at index \a col.
 * @param col
 * @return
 */

int Sheet::getColumnWidth(int col) const
{
    return columnWidths.getValue(col);
}

/**
 * @brief Set row height of row given by index in \p row to \a height.
 * @param row Row index.
 * @param height New height of row.
 */

void Sheet::setRowHeight(int row, int height)
{
    rowHeights.setValue(row, height);
}

/**
 * @brief Get height of row at index \a row.
 * @param row Index of row.
 * @return Height
 */

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
        usedCells.push_back(i->toString());

    return usedCells;
}

void Sheet::updateColumnsOrRows(bool horizontal, int section, int count) 
{
    auto &hiddenProp = horizontal?hiddenColumns:hiddenRows;
    const auto &hidden = hiddenProp.getValues();
    auto it = hidden.lower_bound(section);
    if(it!=hidden.end()) {
        std::set<long> newHidden(hidden.begin(),it);
        if(count>0) {
            for(;it!=hidden.end();++it)
                newHidden.insert(*it + count);
        } else {
            it = hidden.lower_bound(section-count);
            if(it!=hidden.end()) {
                for(;it!=hidden.end();++it)
                    newHidden.insert(*it+count);
            }
        }
        hiddenProp.setValues(newHidden);
    }

    const auto &sizes = horizontal?columnWidths.getValues():rowHeights.getValues();
    auto iter = sizes.lower_bound(section);
    if(iter!=sizes.end()) {
        std::map<int,int> newsizes(sizes.begin(),iter);
        if(count>0) {
            for(;iter!=sizes.end();++iter)
                newsizes.emplace(iter->first + count, iter->second);
        } else {
            iter = sizes.lower_bound(section-count);
            if(iter!=sizes.end()) {
                for(;iter!=sizes.end();++iter)
                    newsizes.emplace(iter->first+count, iter->second);
            }
        }
        if(horizontal) {
            columnWidths.setValues(newsizes);
        } else {
            rowHeights.setValues(newsizes);
        }
    }
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
    updateColumnsOrRows(true,col,count);
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
    updateColumnsOrRows(true,col,-count);
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
    updateColumnsOrRows(false,row,count);
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
    updateColumnsOrRows(false,row,-count);
}

/**
 * @brief Set content of cell at \a address to \a value.
 * @param address Address of cell
 * @param value New value
 */

void Sheet::setContent(CellAddress address, const char *value)
{
    cells.setContent(address, value);
}

/**
 * @brief Set alignment of content in cell at \a address to \a alignment.
 * @param address Address of cell
 * @param alignment New alignment
 */

void Sheet::setAlignment(CellAddress address, int alignment)
{
    cells.setAlignment(address, alignment);
}

/**
 * @brief Set style of cell at \a address to \a style.
 * @param address Address of cell
 * @param style New style
 */

void Sheet::setStyle(CellAddress address, const std::set<std::string> &style)
{
    cells.setStyle(address, style);
}

/**
 * @brief Set foreground (text color) of cell at address \a address to \a color.
 * @param address Address of cell
 * @param color New color
 */

void Sheet::setForeground(CellAddress address, const Color &color)
{
    cells.setForeground(address, color);
}

/**
 * @brief Set background color of cell at address \a address to \a color.
 * @param address Address of cell
 * @param color New color
 */

void Sheet::setBackground(CellAddress address, const Color &color)
{
    cells.setBackground(address, color);
}

/**
 * @brief Set display unit of cell at address \a address to \a unit.
 * @param address Address of cell
 * @param unit New unit
 */

void Sheet::setDisplayUnit(CellAddress address, const std::string &unit)
{
    cells.setDisplayUnit(address, unit);
}

/**
 * @brief Set computed unit for cell at address \a address to \a unit.
 * @param address Address of cell
 * @param unit New unit.
 */

void Sheet::setComputedUnit(CellAddress address, const Base::Unit &unit)
{
    cells.setComputedUnit(address, unit);
}

/**
 * @brief Set alias for cell at address \a address to \a alias. If the alias
 * is an empty string, the existing alias is removed.
 * @param address Address of cell
 * @param alias New alias.
 */

void Sheet::setAlias(CellAddress address, const std::string &alias)
{
    std::string existingAlias = getAddressFromAlias(alias);

    if (existingAlias.size() > 0) {
        if (existingAlias == address.toString()) // Same as old?
            return;
        else
            throw Base::ValueError("Alias already defined");
    }
    else if (alias.size() == 0) // Empty?
        cells.setAlias(address, "");
    else if (isValidAlias(alias)) // Valid?
        cells.setAlias(address, alias);
    else
        throw Base::ValueError("Invalid alias");
}

/**
 * @brief Get cell given an alias string
 * @param alias Alias for cell
 *
 * @returns Name of cell, or empty string if not defined
 */

std::string Sheet::getAddressFromAlias(const std::string &alias) const
{
    const Cell * cell = cells.getValueFromAlias(alias);

    if (cell)
        return cell->getAddress().toString();
    else
        return std::string();
}

/**
 * @brief Determine whether a given alias candidate is valid or not.
 *
 * A candidate is valid is the string is syntactically correct,
 * and the alias does not conflict with an existing property.
 *
 */

bool Sheet::isValidAlias(const std::string & candidate)
{
    // Valid syntactically?
    if (!cells.isValidAlias(candidate))
        return false;

    // Existing alias? Then it's ok
    if (getAddressFromAlias(candidate).size() > 0 )
        return true;

    // Check to see that is does not crash with any other property in the Sheet object.
    if (getPropertyByName(candidate.c_str()))
        return false;
    else
        return true;
}

/**
 * @brief Set row and column span for the cell at address \a address to \a rows and \a columns.
 * @param address Address to upper right corner of cell
 * @param rows Rows to span
 * @param columns Columns to span
 */

void Sheet::setSpans(CellAddress address, int rows, int columns)
{
    cells.setSpans(address, rows, columns);
}

/**
 * @brief Called when alias \a alias at \a address is removed.
 * @param address Address of alias.
 * @param alias Removed alias.
 */

void Sheet::aliasRemoved(CellAddress address, const std::string & alias)
{
    removedAliases[address] = alias;
}

/**
 * @brief Return a set of dependencies links for cell at \a address.
 * @param address Address of cell
 * @return Set of dependencies.
 */

std::set<std::string> Sheet::dependsOn(CellAddress address) const
{
    return cells.getDeps(address);
}

/**
 * @brief Compute links to cells that cell at \a address provides input to.
 * @param address Address of cell
 * @param result Set of links.
 */

void Sheet::providesTo(CellAddress address, std::set<std::string> & result) const
{
    std::string fullName = getFullName() + ".";
    std::set<CellAddress> tmpResult = cells.getDeps(fullName + address.toString());

    for (std::set<CellAddress>::const_iterator i = tmpResult.begin(); i != tmpResult.end(); ++i)
        result.insert(fullName + i->toString());
}

/**
 * @brief Compute links to cells that cell at \a address provides input to.
 * @param address Address of cell
 * @param result Set of links.
 */

std::set<CellAddress>  Sheet::providesTo(CellAddress address) const
{
    return cells.getDeps(getFullName()+"."+address.toString());
}

void Sheet::onDocumentRestored()
{
    auto ret = execute();
    if(ret!=DocumentObject::StdReturn) {
        FC_ERR("Failed to restore " << getFullName() << ": " << ret->Why);
        delete ret;
    }
}

/**
 * @brief Create a document observer for this sheet. Used to track changes.
 * @param document document to observer.
 */

void Sheet::observeDocument(Document * document)
{
    // observer is no longer required as PropertySheet is now derived from
    // PropertyLinkBase and will handle all the link related behavior
#if 1
    (void)document;
#else
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
#endif
}

void Sheet::renameObjectIdentifiers(const std::map<ObjectIdentifier, ObjectIdentifier> &paths)
{
    DocumentObject::renameObjectIdentifiers(paths);

    cells.renameObjectIdentifiers(paths);
}

void Sheet::editCell(App::CellAddress address, const char *data) {
    auto cell = getCell(address);
    if(!cell || cell->getEditMode()==Cell::EditNormal) {
        setCell(address,data);
        return;
    }

    if(cell->hasException()) {
        FC_THROWM(Base::ExpressionError,cell->getException());
        return;
    }

    cell->setEditData(data);
}

bool Sheet::hasCell(const std::vector<App::Range> &ranges) const {
    for(auto range : ranges) {
        do {
            if(cells.getValue(*range))
                return true;
        }while(range.next());
    }
    return false;
}

std::string Sheet::getRow(int offset) const {
    if(currentRow < 0)
        throw Base::RuntimeError("No current row");
    int row = currentRow + offset;
    if(row<0 || row>CellAddress::MAX_ROWS)
        throw Base::ValueError("Out of range");
    return std::to_string(row+1);
}

std::string Sheet::getColumn(int offset) const {
    if(currentCol < 0)
        throw Base::RuntimeError("No current column");
    int col = currentCol + offset;
    if(col<0 || col>CellAddress::MAX_COLUMNS)
        throw Base::ValueError("Out of range");
    if (col < 26) {
        char txt[2];
        txt[0] = (char)('A' + col);
        txt[1] = 0;
        return txt;
    }

    col -= 26;
    char txt[3];
    txt[0] = (char)('A' + (col / 26));
    txt[1] = (char)('A' + (col % 26));
    txt[2] = 0;
    return txt;
}

void Sheet::onChanged(const App::Property *prop) {
    if(!isRestoring() && getDocument() && !getDocument()->isPerformingTransaction()) {
        if(prop == &PythonMode) 
            cells.setDirty();
    }
    App::DocumentObject::onChanged(prop);
}

///////////////////////////////////////////////////////////////////////////////

TYPESYSTEM_SOURCE(Spreadsheet::PropertySpreadsheetQuantity, App::PropertyQuantity);

Property *PropertySpreadsheetQuantity::Copy() const
{
    PropertySpreadsheetQuantity * obj = new PropertySpreadsheetQuantity();

    obj->_dValue = _dValue;
    obj->_Unit = _Unit;

    return obj;
}

void PropertySpreadsheetQuantity::Paste(const Property &from)
{
    aboutToSetValue();
    _dValue = static_cast<const PropertySpreadsheetQuantity*>(&from)->_dValue;
    _Unit = static_cast<const PropertySpreadsheetQuantity*>(&from)->_Unit;
    hasSetValue();
}

// Python sheet feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Spreadsheet::SheetPython, Spreadsheet::Sheet)
template<> const char* Spreadsheet::SheetPython::getViewProviderName(void) const {
    return "SpreadsheetGui::ViewProviderSheet";
}
template<> PyObject* Spreadsheet::SheetPython::getPyObject(void) {
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new FeaturePythonPyT<Spreadsheet::SheetPy>(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
/// @endcond

// explicit template instantiation
template class SpreadsheetExport FeaturePythonT<Spreadsheet::Sheet>;
}
