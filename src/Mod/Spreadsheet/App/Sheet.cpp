/***************************************************************************
 *   Copyright (c) 2013 Eivind Kvedalen (eivind@kvedalen.name)             *
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


#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include <boost/tokenizer.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/assign.hpp>
#include <App/Document.h>
#include <App/DynamicProperty.h>
#include <Base/Exception.h>
#include <Base/Placement.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include "Expression.h"
#include "Sheet.h"
#include "SheetPy.h"
#include <ostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <boost/regex.hpp>

using namespace Spreadsheet;
using namespace App;

PROPERTY_SOURCE(Spreadsheet::Sheet, App::DocumentObject)

const int Sheet::MAX_ROWS = 26 * 26 + 26;
const int Sheet::MAX_COLUMNS = 16384;

/**
  * Construct a new Sheet object.
  */

Sheet::Sheet()
    : App::DocumentObject()
    , props(this)
{
    ADD_PROPERTY(docDeps, (0));
    docDeps.setSize(0);
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
    std::map<CellPos, CellContent* >::iterator i = cells.begin();

    /* Clear cells */
    while (i != cells.end()) {
        delete (*i).second;
        ++i;
    }
    cells.clear();

    std::vector<std::string> propNames = props.getDynamicPropertyNames();

    for (std::vector<std::string>::const_iterator i = propNames.begin(); i != propNames.end(); ++i)
        props.removeDynamicProperty((*i).c_str());

    propAddress.clear();
    deps.clear();
    isComputing.clear();

    for (ObserverMap::iterator it = observers.begin(); it != observers.end(); ++it)
        delete (*it).second;
    observers.clear();
    docDeps.setValues(std::vector<DocumentObject*>());

    mergedCells.clear();
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
                        setCell(row, col, (*i).c_str());
                    col++;
                }
            }
            catch (...) {
                return false;
            }

            ++row;
        }
        file.close();
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
    int row, col, prevRow = -1, prevCol = -1;

    file.open(filename.c_str(), std::ios::out | std::ios::ate | std::ios::binary);

    if (!file.is_open())
        return false;

    std::map<CellPos, CellContent*>::const_iterator i = cells.begin();

    while (i != cells.end()) {
        Property * prop = getProperty((*i).first);

        decodePos((*i).first, row, col);

        if (prevRow != -1 && prevRow != row) {
            for (int i = prevRow; i < row; ++i)
                file << std::endl;
            prevCol = 0;
        }
        if (prevCol != -1 && col != prevCol) {
            for (int i = prevCol; i < col; ++i)
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

        prevRow = row;
        prevCol = col;
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

bool Sheet::mergeCells(const Sheet::Range & range)
{
    CellPos fromPos = range.from();
    CellPos toPos =  range.to();
    int fromRow, fromCol;
    int toRow, toCol;

    decodePos(fromPos, fromRow, fromCol);
    decodePos(toPos, toRow, toCol);

    // Check that this merge is not overlapping other merged cells
    for (int r = fromRow; r <= toRow; ++r) {
        for (int c = fromCol; c <= toCol; ++c) {
            if (mergedCells.find(encodePos(r, c)) != mergedCells.end())
                return false;
        }
    }

    // Clear cells that will be hidden by the merge
    for (int r = fromRow; r <= toRow; ++r)
        for (int c = fromCol; c <= toCol; ++c)
            if ( !(r == fromRow && c == fromCol) )
                clear(toAddress(r, c).c_str(), true);

    // Update internal structure to track merged cells
    for (int r = fromRow; r <= toRow; ++r)
        for (int c = fromCol; c <= toCol; ++c)
            mergedCells[encodePos(r, c)] = fromPos;

    CellContent * cell = getCell(fromPos);
    cell->setSpans(toRow - fromRow + 1, toCol - fromCol + 1);

    cellSpanChanged(fromRow, fromCol);

    return true;
}

/**
  * Split a previously merged cell specified by \a address into its individual cells.
  * The address can point to any of the cells that make up the merged cell.
  *
  * @param address Address of cell to split
  *
  */

void Sheet::splitCell(const std::string &address)
{
    CellPos pos = addressToCellPos(address.c_str());
    CellPos anchor;
    int rows, cols;
    int row, col;

    decodePos(pos, row, col);

    if (mergedCells.find(pos) == mergedCells.end())
        return;

    anchor = mergedCells[pos];
    getSpans(row, col, rows, cols);

    // decode anchor pos into row and col
    decodePos(anchor, row, col);

    for (int r = row; r <= row + rows; ++r)
        for (int c = col; c <= col + cols; ++c)
            mergedCells.erase(encodePos(r, c));

    CellContent * cell = getCell(anchor);
    cell->setSpans(1, 1);

    cellSpanChanged(row, col);
}

/**
  * Get contents of the cell specified by \a key.
  * This function always return a CellContent, and will create an empty
  * cell if it is not previously undefined.
  *
  * @returns A CellContent object.
  */

Sheet::CellContent *Sheet::getCell(CellPos key) const
{
    std::map<CellPos, CellContent*>::const_iterator i = cells.find(key);

    if (i == cells.end()) {
        int row, col;

        decodePos(key, row, col);
        return cells[key] = new CellContent(row, col, this);
    }
    else
        return (*i).second;
}

/**
  * Get cell contents specified by (\a row, \a col).
  *
  */

Sheet::CellContent *Sheet::getCell(int row, int col) const
{
    assert(row >= 0 && row < MAX_ROWS &&
           col >= 0 && row < MAX_COLUMNS);

    return getCell( encodePos(row, col) );
}

/**
  * Get cell contents specified by \a cell.
  *
  */

Sheet::CellContent *Sheet::getCell(const std::string &cell) const
{
    int row, col;

    addressToRowCol(cell.c_str(), row, col);

    return getCell( row, col );
}

/**
  * Convert a string address into integer \a row and \a column.
  * row and col are 0-based.
  *
  * This function will throw an exception if the specified \a address is invalid.
  *
  * @param address Address to parse.
  * @param row     Reference to integer where row position is stored.
  * @param col     Reference to integer where col position is stored.
  *
  */

void Sheet::addressToRowCol(const char * address, int & row, int &col)
{
    int i = 0;
    static const boost::regex e("\\${0,1}([A-Za-z]+)\\${0,1}([0-9]+)");
    boost::cmatch cm;

    if (boost::regex_match(address, cm, e)) {
        const boost::sub_match<const char *> colstr = cm[1];
        const boost::sub_match<const char *> rowstr = cm[2];

        col = decodeColumn(colstr.str());
        row = decodeRow(rowstr.str());
    }
    else
        throw Base::Exception("Invalid cell specifier.");
}

/**
  * Convert given \a address to a CellPos type. This function will throw an exception
  * if the specified address is invalid.
  *
  * @returns A CellPos with the address.
  */

Sheet::CellPos Sheet::addressToCellPos(const char *address)
{
    int row, col;

    addressToRowCol(address, row, col);

    return encodePos(row, col);
}

/**
  * Convert given \a row and \a col into a string address.
  *
  * @param row Row address.
  * @param col Column address.
  *
  * @returns Address given as a string.
  */

std::string Sheet::toAddress(int row, int col)
{
    std::stringstream s;

    if (col < 26)
        s << (char)('A' + col);
    else {
        col -= 26;

        s << (char)('A' + (col / 26));
        s << (char)('A' + (col % 26));
    }

    s << (row + 1);

    return s.str();
}

/**
  * Convert given \a key address to string.
  *
  * @param key Address of cell.
  *
  * @returns Address given as a string.
  */

std::string Sheet::toAddress(CellPos key)
{
    int row, col;

    decodePos(key, row, col);

    return toAddress(row, col);
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
    int row, col;

    assert(address != 0 &&  contents != 0);

    addressToRowCol(address, row, col);

    setCell(row, col, contents);
}

/**
  * Set cell at \a row, \a col to given value.
  *
  * @param row   Row position of cell.
  * @param col   Column position of cell.
  * @param value Value to set.
  */

void Sheet::setCell(int row, int col, const char *value)
{
    Expression * e;

    assert(row >= 0 && row < MAX_ROWS &&
           col >= 0 && row < MAX_COLUMNS &&
           value != 0);

    if (*value == '=') {
        try {
            e = ExpressionParser::parse(this, value + 1);
        }
        catch (...) {
            e = new StringExpression(this, "ERR:parse");
        }
    }

    else if (*value == '\'')
        e = new StringExpression(this, value + 1);
    else {
        char * end;

        if (*value == '\0') {
            clear(toAddress(row, col).c_str(), false);
            return;
        }
        else {
            errno = 0;
            double float_value = strtod(value, &end);
            if (!*end && errno == 0)
                e = new NumberExpression(this, float_value);
            else
                e = new StringExpression(this, value);
        }
    }

    setCell(row, col, e, value);
}

/**
  * Update dependencies of \a expression for cell at \a key. This function creates
  * DocumentObserver objects to automatically get changes of the Property objects used by the
  * expressions.
  *
  * @param expression Expression to extract dependencies from
  * @param key        Address of cell containing the expression.
  */

void Sheet::addDependencies(const Expression * expression, CellPos key)
{
    std::set<const App::Property*> expressionDeps;

    // Get dependencies from expression
    expression->getDeps(expressionDeps);

    std::set<const App::Property*>::const_iterator i = expressionDeps.begin();
    while (i != expressionDeps.end()) {
        const App::Property * depProp = *i;
        DocumentObject * docObject = dynamic_cast<DocumentObject*>(depProp->getContainer());

        std::string name = getPropertyName(depProp);
        std::string docName = std::string(docObject->getNameInDocument());
        std::string fullName = docName + "." + name;

        ObserverMap::const_iterator it = observers.find(docName);
        if (it != observers.end()) {
            // An observer already exists, increase reference counter for it
            (*it).second->ref();
        }
        else {
            // Create a new observer
            SheetObserver * observer = new SheetObserver(getDocument(), this);

            observers.insert(std::make_pair<std::string, SheetObserver*>(docName, observer));

            docDeps.setSize(docDeps.getSize() + 1);
            docDeps.set1Value(docDeps.getSize() - 1, docObject);
        }

        deps[fullName].insert(key);
        ++i;
    }
}

/**
  * Remove dependecies given by \a expression for cell at \a key.
  *
  * @param expression Expression to extract dependencies from
  * @param key        Address of cell containing the expression
  *
  */

void Sheet::removeDependencies(const Expression * expression, CellPos key)
{
    assert(expression != 0);

    std::set<const App::Property*> expressionDeps;

    // Get dependencies from expression
    expression->getDeps(expressionDeps);

    std::set<const App::Property*>::const_iterator i = expressionDeps.begin();
    while (i != expressionDeps.end()) {
        const App::Property * depProp = *i;
        DocumentObject * docObject = dynamic_cast<DocumentObject*>(depProp->getContainer());

        std::string name = getPropertyName(depProp);
        std::string docName = std::string(docObject->getNameInDocument());
        std::string fullName = docName + "." + name;

        ObserverMap::iterator it = observers.find(docName);
        if (it != observers.end()) {
            // Observer found, decrease reference, and delete it if it is unused after this dependency is removed
            if (!(*it).second->unref()) {

                // Remove from object dependency list
                std::vector<App::DocumentObject*> v = docDeps.getValues();
                v.erase(find(v.begin(), v.end(), (App::DocumentObject*)it->second));
                docDeps.setValues(v);

                // Delete object
                delete it->second;

                // Remove from map
                observers.erase(it);
            }
        }

        deps[fullName].erase(key);
        ++i;
    }
}

/**
  * Recompute any cells that depend on \a prop.
  *
  * @param prop Property that presumably has changed an triggers updates of other cells.
  *
  */

void Sheet::recomputeDependants(const Property *prop)
{
    DocumentObject * owner = dynamic_cast<DocumentObject*>(prop->getContainer());

    if (owner) {
        // Recompute cells that depend on this cell
        std::string fullName = std::string(owner->getNameInDocument()) + "." + getPropertyName(prop);
        std::set<CellPos>::const_iterator j = deps[fullName].begin();
        std::set<CellPos>::const_iterator end = deps[fullName].end();

        while (j != end) {
            updateProperty((*j));
            ++j;
        }
    }
}

/**
  * Set cell at \a row, \a col to \a expression. The original string given by the user
  * is specified in \a value. If a merged cell is specified, the upper right corner of the
  * merged cell must be specified.
  *
  * @param row        Row position of cell.
  * @param col        Column position of cell.
  * @param expression Expression to set.
  * @param value      String value of original expression.
  *
  */

void Sheet::setCell(int row, int col, const Expression * expression, const char * value)
{
    assert(row >= 0 && row < MAX_ROWS &&
           col >= 0 && row < MAX_COLUMNS);

    // Compute index into map
    CellPos key = encodePos(row, col);

    if (mergedCells.find(key) != mergedCells.end() &&
            mergedCells.at(key) != key) {
        // Trying to set something "hidden" by a merged cell -- disallow this
        throw Base::Exception("Setting this cell is not allowed (hidden by merge");
    }

    // Update expression, delete old first if necessary
    Sheet::CellContent * cell = getCell(row, col);
    if (cell->getExpression()) {
        removeDependencies(cell->getExpression(), key);
        cell->setExpression(0);
    }
    cell->setExpression(expression);
    cell->setStringContent(value ? value : "");

    addDependencies(expression, key);

    // Update property
    updateProperty(key);

    // Emit update signal
    cellUpdated(row, col);

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

Property * Sheet::getProperty(CellPos key) const
{
    return props.getDynamicPropertyByName(toAddress(key).c_str());
}

/**
  * Get the name of the Property object \a prop.
  *
  * @returns The name of the Property.
  *
  */

const std::string Sheet::getPropertyName(const Property * prop) const
{
    if (prop->getContainer() == this) {
        const char * name = props.getName(prop);
        assert(name != 0);
        return name;
    }
    else {
        const char * name = prop->getName();

        assert(name != 0);
        return name;
    }
}

/**
  * Get the address as (\a row, \a col) of the Property \a prop. This function
  * throws an exception if the property is not found.
  *
  */

void Sheet::getCellAddress(const Property *prop, int &row, int &col)
{
    std::map<const App::Property*, CellPos >::const_iterator i = propAddress.find(prop);

    if (i != propAddress.end()) {
        decodePos((*i).second, row, col);
    }
    else
        throw Base::Exception("Property is not a cell");
}

/**
  * Set the property for cell \key to a PropertyFloat with the value \a value.
  * If the Property exists, but of wrong type, the previous Property is destroyed and recreated as the correct type.
  *
  * @param key   The address of the cell we want to create a Property for
  * @param value The value we want to assign to the Property.
  *
  */

void Sheet::setFloatProperty(CellPos key, double value) const
{
    Property * prop = props.getPropertyByName(toAddress(key).c_str());
    PropertyFloat * floatProp = dynamic_cast<PropertyFloat*>(prop);

    if (!floatProp) {
        if (prop) {
            props.removeDynamicProperty(toAddress(key).c_str());
            propAddress.erase(prop);
        }
        floatProp = dynamic_cast<PropertyFloat*>(props.addDynamicProperty("App::PropertyFloat", toAddress(key).c_str(), 0, 0, 0, false, true));
    }
    propAddress[floatProp] = key;
    floatProp->setValue(value);
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

void Sheet::setQuantityProperty(CellPos key, double value, const Base::Unit & unit) const
{
    Property * prop = props.getPropertyByName(toAddress(key).c_str());
    PropertyQuantity * quantityProp = dynamic_cast<PropertyQuantity*>(prop);

    if (!quantityProp) {
        if (prop) {
            props.removeDynamicProperty(toAddress(key).c_str());
            propAddress.erase(prop);
        }
        Property * p = props.addDynamicProperty("App::PropertyQuantity", toAddress(key).c_str(), 0, 0, 0, false, true);
        quantityProp = dynamic_cast<PropertyQuantity*>(p);
    }
    propAddress[quantityProp] = key;
    quantityProp->setValue(value);
    quantityProp->setUnit(unit);
    getCell(key)->setComputedUnit(unit);
}

/**
  * Set the property for cell \key to a PropertyString with \a value.
  * If the Property exists, but of wrong type, the previous Property is destroyed and recreated as the correct type.
  *
  * @param key   The address of the cell we want to create a Property for
  * @param value The value we want to assign to the Property.
  *
  */

void Sheet::setStringProperty(CellPos key, const char * value) const {
    Property * prop = props.getPropertyByName(toAddress(key).c_str());
    PropertyString * stringProp = dynamic_cast<PropertyString*>(prop);

    if (!stringProp) {
        if (prop) {
            props.removeDynamicProperty(toAddress(key).c_str());
            propAddress.erase(prop);
        }
        stringProp = dynamic_cast<PropertyString*>(props.addDynamicProperty("App::PropertyString", toAddress(key).c_str(), 0, 0, 0, false, true));
    }

    propAddress[stringProp] = key;
    stringProp->setValue(value);
}

/**
  * Update the Propery given by \a key. This will also eventually trigger recomputations of cells depending on \a key.
  *
  * @param key The address of the cell we want to recompute.
  *
  */

void Sheet::updateProperty(CellPos key) const
{
    CellContent * cell = getCell(key);

    try {
        Expression * output;

        if (isComputing.find(key) != isComputing.end())
            throw Expression::CircularException();

        // Mark this cell as "is computing"
        isComputing.insert(key);

        if (cell->getExpression())
            output = cell->getExpression()->eval();
        else {
            std::string s;

            if (cell->getStringContent(s))
                output = new StringExpression(this, s);
            else
                output = new StringExpression(this, "");
        }

        /* Eval returns either NumberExpression or StringExpression objects */
        if (dynamic_cast<NumberExpression*>(output)) {
            NumberExpression * number = dynamic_cast<NumberExpression*>(output);
            if (number->getUnit().isEmpty())
                setFloatProperty(key, number->getValue());
            else
                setQuantityProperty(key, number->getValue(), number->getUnit());
        }
        else
            setStringProperty(key, dynamic_cast<StringExpression*>(output)->toString().c_str());

        delete output;
    }
    catch (const Expression::CircularException & e) {
        setStringProperty(key, "ERR:circular");
        isComputing.erase(key);
        return;
    }
    catch (const Expression::Exception & e) {
        setStringProperty(key, "ERR");
        isComputing.erase(key);
        return;
    }
    catch (...) {
        setStringProperty(key, "ERR");
        isComputing.erase(key);
        return;
    }

    // Done computing this cell
    isComputing.erase(key);
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
    try {
        Property * prop;

        // addressToRow wil throw an exception if name is not a valid cell specifier
        CellPos key = addressToCellPos(name);

        prop = getProperty(key);

        if (prop == 0) {
            if (mergedCells.find(key) != mergedCells.end() &&
                    mergedCells.at(key) != key) {
                // Trying to get something "hidden" by a merged cell -- disallow this
                throw Base::Exception("Setting this cell is not allowed (hidden by merge");
            }
            updateProperty(key);
            prop = getProperty(key);
        }
        return prop;
    }
    catch (const Expression::CircularException & e) {
        throw e;
    }
    catch (const Expression::Exception & e) {
        throw e;
    }
    catch (...) {
        return DocumentObject::getPropertyByName(name);
    }
}

/**
  * Unimplemented.
  *
  */

App::DocumentObjectExecReturn *Sheet::execute(void)
{
    return App::DocumentObject::StdReturn;
}

/**
  * Unimplemented.
  *
  */

DocumentObjectExecReturn *Sheet::recompute(void)
{
    return App::DocumentObject::StdReturn;
}

/**
  * Unimplemented.
  *
  */

short Sheet::mustExecute(void) const
{
    return 0;
}

/**
  * The SheetObserver constructor.
  *
  * @param document The Document we are observing
  * @param _sheet   The sheet owning this observer.
  *
  */

SheetObserver::SheetObserver(App::Document * document, Sheet *_sheet)
    : DocumentObserver(document)
    , sheet(_sheet)
    , refCount(1)
{
}

/**
  * Unimplemented.
  *
  */

void SheetObserver::slotCreatedDocument(const App::Document &Doc)
{
}

/**
  * Unimplemented.
  *
  */

void SheetObserver::slotDeletedDocument(const App::Document &Doc)
{
}

/**
  * Unimplemented.
  *
  */

void SheetObserver::slotCreatedObject(const DocumentObject &Obj)
{
}

/**
  * Unimplemented.
  *
  */

void SheetObserver::slotDeletedObject(const DocumentObject &Obj)
{
    // FIXME: We should recompute any cells referencing this object. How?
}

/**
  * Invoke the sheets recomputeDependants when a change to a Property occurs.
  *
  */

void SheetObserver::slotChangedObject(const DocumentObject &Obj, const Property &Prop)
{
    sheet->recomputeDependants(&Prop);
}

/**
  * Increase reference count.
  *
  */

void SheetObserver::ref()
{
    refCount++;
}

/**
  * Decrease reference count.
  *
  */

bool SheetObserver::unref()
{
    refCount--;
    return refCount;
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

void Sheet::clear(const char * address, bool all)
{
    CellPos pos = addressToCellPos(address);

    if (cells.find(pos) != cells.end()) {
        if (cells[pos]->getExpression() != 0)
            removeDependencies(cells[pos]->getExpression(), pos);
        delete cells[pos];
        cells.erase(pos);

        std::string addr = toAddress(pos);
        Property * prop = props.getDynamicPropertyByName(addr.c_str());

        propAddress.erase(prop);
        props.removeDynamicProperty(addr.c_str());
    }

    int row, col;
    decodePos(pos, row, col);
    cellUpdated(row, col);
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

void Sheet::getSpans(int row, int col, int &rows, int &cols) const
{
    CellPos pos = encodePos(row, col);

    if (mergedCells.find(pos) != mergedCells.end()) {
        CellPos anchor = mergedCells.at(pos);
        CellContent * cell = getCell(anchor);

        cell->getSpans(rows, cols);
    }
    else {
        rows = cols = 1;
    }
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

bool Sheet::isMergedCell(int row, int col) const
{
    CellPos pos = encodePos(row, col);

    return mergedCells.find(pos) != mergedCells.end();
}

/**
  * Set the width (in pixels) of column \a col to \a width.
  *
  * @param col   Column to set
  * @param width Width in pixels
  *
  */

void Sheet::setColumnWidth(int col, int width)
{
    if (width >= 0) {
        int oldValue = columnWidths[col];

        columnWidths[col] = width;

        if (oldValue != width)
            columnWidthChanged(col, width);
    }
}

/**
  * Get column width of column \a col.
  *
  * @param col Column to query
  *
  * @returns Width in pixels, or 0 if width is the default and not set eralier.
  *
  */

int Sheet::getColumnWidth(int col) const
{
    if (columnWidths.find(col) != columnWidths.end())
        return columnWidths.at(col);
    else
        return 0;
}

/**
  * Set height of row given by \a row to \a height in pixels.
  *
  * @param row    Address of row
  * @param height Height in pixels
  *
  */

void Sheet::setRowHeight(int row, int height)
{
    if (height >= 0) {
        int oldValue = rowHeights[row];

        rowHeights[row] = height;

        if (oldValue != height)
            rowHeightChanged(row, height);
    }
}

/**
  * Get height of row \a row.
  *
  * @param row
  *
  * @returns Height of row in pixels, or 0 if height is the default and not set eralier.
  *
  */

int Sheet::getRowHeight(int row) const
{
    if (rowHeights.find(row) != rowHeights.end())
        return rowHeights.at(row);
    else
        return 0;
}

/**
  * Get a vector of strings with addresses of all used cells.
  *
  * @returns vector of strings.
  *
  */

std::vector<std::string> Sheet::getUsedCells() const
{
    std::set<CellPos> usedSet;
    std::vector<std::string> usedCells;

    // Insert int usedSet
    for (std::map<CellPos, CellContent*>::const_iterator i = cells.begin(); i != cells.end(); ++i) {
        if (i->second->isUsed())
            usedSet.insert(i->first);
    }

    for (std::set<CellPos>::const_iterator i = usedSet.begin(); i != usedSet.end(); ++i)
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
    std::vector<CellPos> keys;

    /* Copy all keys from cells map */
    boost::copy(cells | boost::adaptors::map_keys, std::back_inserter(keys));

    /* Sort them */
    std::sort(keys.begin(), keys.end());

    for (std::vector<CellPos>::const_reverse_iterator i = keys.rbegin(); i != keys.rend(); ++i) {
        int curr_row, curr_col;

        decodePos(*i, curr_row, curr_col);

        if (curr_col >= MAX_COLUMNS - col)
            clear(toAddress(curr_row, curr_col).c_str());
        else if (curr_col >= col)
            moveCell(encodePos(curr_row, curr_col), encodePos(curr_row, curr_col + count));
    }
}

/**
  * Sort function to sort two cell positions according to their column position.
  *
  */

bool Sheet::colSortFunc(const Sheet::CellPos & a, const Sheet::CellPos & b) {
    int row_a, col_a, row_b, col_b;

    decodePos(a, row_a, col_a);
    decodePos(b, row_b, col_b);

    if (col_a < col_b)
        return true;
    else
        return false;
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
    std::vector<CellPos> keys;

    /* Copy all keys from cells map */
    boost::copy(cells | boost::adaptors::map_keys, std::back_inserter(keys));

    /* Sort them */
    std::sort(keys.begin(), keys.end(), boost::bind(&Sheet::rowSortFunc, this, _1, _2));

    for (std::vector<CellPos>::const_iterator i = keys.begin(); i != keys.end(); ++i) {
        int curr_row, curr_col;

        decodePos(*i, curr_row, curr_col);

        if (curr_col >= col) {
            if (curr_col < col + count)
                clear(toAddress(curr_col, curr_row).c_str());
            else
                moveCell(encodePos(curr_row, curr_col), encodePos(curr_row, curr_col - count));
        }
    }
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
    std::vector<CellPos> keys;

    /* Copy all keys from cells map */
    boost::copy(cells | boost::adaptors::map_keys, std::back_inserter(keys));

    /* Sort them */
    std::sort(keys.begin(), keys.end(), boost::bind(&Sheet::rowSortFunc, this, _1, _2));

    for (std::vector<CellPos>::const_reverse_iterator i = keys.rbegin(); i != keys.rend(); ++i) {
        int curr_row, curr_col;

        decodePos(*i, curr_row, curr_col);

        if (curr_row >= MAX_ROWS - row)
            clear(toAddress(curr_row, curr_col).c_str());
        else if (curr_row >= row)
            moveCell(encodePos(curr_row, curr_col), encodePos(curr_row + count, curr_col));
    }
}

/**
  * Sort function to sort two cell positions according to their row position.
  *
  */

bool Sheet::rowSortFunc(const Sheet::CellPos & a, const Sheet::CellPos & b) {
    int row_a, col_a, row_b, col_b;

    decodePos(a, row_a, col_a);
    decodePos(b, row_b, col_b);

    if (row_a < row_b)
        return true;
    else
        return false;
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
    std::vector<CellPos> keys;

    /* Copy all keys from cells map */
    boost::copy(cells | boost::adaptors::map_keys, std::back_inserter(keys));

    /* Sort them */
    std::sort(keys.begin(), keys.end(), boost::bind(&Sheet::rowSortFunc, this, _1, _2));

    for (std::vector<CellPos>::const_iterator i = keys.begin(); i != keys.end(); ++i) {
        int curr_row, curr_col;

        decodePos(*i, curr_row, curr_col);

        if (curr_row >= row) {
            if (curr_row < row + count)
                clear(toAddress(curr_col, curr_row).c_str());
            else
                moveCell(encodePos(curr_row, curr_col), encodePos(curr_row - count, curr_col));
        }
    }
}

/**
  * Encode \a str using the same encoding as XML attributes.
  *
  * @param str String to encode.
  *
  * @returns Encoded attribute.
  *
  */

std::string Sheet::encodeAttribute(const std::string& str)
{
    std::string tmp;
    for (std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
        if (*it == '<')
            tmp += "&lt;";
        else if (*it == '"')
            tmp += "&quot;";
        else if (*it == '&')
            tmp += "&amp;";
        else if (*it == '>')
            tmp += "&gt;";
        else if (*it == '\n')
            tmp += " ";
        else
            tmp += *it;
    }

    return tmp;
}

/**
  * Encode \a color as a #rrggbbaa string.
  *
  * @param color Color to encode.
  *
  * @returns String with encoded color.
  *
  */

std::string Sheet::encodeColor(const Color & color)
{
    std::stringstream tmp;

    tmp << "#"
        << std::hex << std::setw(2) << int(color.r * 255.0)
        << std::hex << std::setw(2) << int(color.g * 255.0)
        << std::hex << std::setw(2) << int(color.b * 255.0)
        << std::hex << std::setw(2) << int(color.a * 255.0);

    return tmp.str();
}

/**
  * Decode aligment into its internal value.
  *
  * @param itemStr   Alignment as a string
  * @param alignment Current alignment. This is or'ed with the one in \a itemStr.
  *
  * @returns New alignment.
  *
  */

int Sheet::decodeAlignment(const std::string & itemStr, int alignment)
{
    if (itemStr == "left")
        alignment = (alignment & ~0x07) | 1;
    else if (itemStr == "center")
        alignment = (alignment & ~0x07) | 2;
    else if (itemStr == "right")
        alignment = (alignment & ~0x07) | 4;
    else if (itemStr == "top")
        alignment = (alignment & ~0x70) | 0x10;
    else if (itemStr == "vcenter")
        alignment = (alignment & ~0x70) | 0x20;
    else if (itemStr == "bottom")
        alignment = (alignment & ~0x70) | 0x40;
    else
        throw Base::Exception("Invalid alignment.");

    return alignment;
}

/**
  * Encode internal alignment value as a string.
  *
  * @param alignment Alignment as a binary value.
  *
  * @returns Alignment represented as a string.
  *
  */

std::string Sheet::encodeAlignment(int alignment)
{
    std::string s;

    if (alignment & 0x1)
        s += "left";
    if (alignment & 0x2)
        s += "center";
    if (alignment & 0x4)
        s += "right";

    if (alignment & 0xf0)
        s += "|";

    if (alignment & 0x10)
        s += "top";
    if (alignment & 0x20)
        s += "vcenter";
    if (alignment & 0x40)
        s += "bottom";

    return s;
}

/**
  * Encode set of styles as a string.
  *
  * @param style Set of string describing the style.
  *
  * @returns Set encoded as a string.
  *
  */

std::string Sheet::encodeStyle(const std::set<std::string> & style)
{
    std::string s;
    std::set<std::string>::const_iterator j = style.begin();
    std::set<std::string>::const_iterator j_end = style.end();

    while (j != j_end) {
        s += *j;
        ++j;
        if (j != j_end)
            s += "|";
    }

    return s;
}

/**
  * Decode a string of the format #rrggbb or #rrggbbaa into a Color.
  *
  * @param color        The color to decode.
  * @param defaultColor A default color in case the decoding fails.
  *
  * @returns Decoded color.
  *
  */

Color Sheet::decodeColor(const std::string & color, const Color & defaultColor)
{
    if (color.size() == 7 || color.size() == 9) {
        Color c;

        if (color[0] != '#')
            return defaultColor;
        unsigned int value = strtoul(color.c_str() + 1, 0, 16);

        if (color.size() == 7)
            value = (value << 8) | 0xff;

        c.setPackedValue(value);
        return c;
    }
    else
        return defaultColor;
}

/**
  * Decode a row specification into a 0-based integer.
  *
  * @param rowstr Row specified as a string, with "1" being the first row.
  *
  * @returns The row.
  */

int Sheet::decodeRow(const std::string &rowstr)
{
    char * end;
    int i = strtol(rowstr.c_str(), &end, 10);

    if (i <0 || i >= MAX_ROWS || *end)
        throw Base::Exception("Invalid row specification.");

    return i - 1;
}

/**
  * Decode a column specification into a 0-based integer.
  *
  * @param colstr Column specified as a string, with "A" begin the first column.
  *
  * @returns The column.
  *
  */

int Sheet::decodeColumn(const std::string &colstr)
{
    int col = 0;

    if (colstr.length() == 1) {
        if ((colstr[0] >= 'A' && colstr[0] <= 'Z'))
            col = colstr[0] - 'A';
        else
            col = colstr[0] - 'a';
    }
    else {
        col = 0;
        for (std::string::const_reverse_iterator i = colstr.rbegin(); i != colstr.rend(); ++i) {
            int v;

            if ((*i >= 'A' && *i <= 'Z'))
                v = *i - 'A';
            else if ((*i >= 'a' && *i <= 'z'))
                v = *i - 'a';
            else
                throw Base::Exception("Invalid column specification");

            col = col * 26 + v;
        }
        col += 26;
    }
    return col;
}

/**
  * Move a cell from \a currPos to \a newPos. If the cell at new position
  * contains data, it is overwritten by the move.
  *
  */

void Sheet::moveCell(Sheet::CellPos currPos, Sheet::CellPos newPos)
{
    std::map<CellPos, CellContent*>::const_iterator i = cells.find(currPos);
    std::map<CellPos, CellContent*>::const_iterator j = cells.find(newPos);

    if (j != cells.end())
        clear(toAddress(newPos).c_str());

    if (i != cells.end()) {
        int row, col;
        CellContent * cell = cells[currPos];

        // Remove from old
        cells.erase(currPos);
        decodePos(currPos, row, col);
        updateProperty(currPos);
        cellUpdated(row, col);

        // Insert into new spot
        decodePos(newPos, row, col);
        cell->moveAbsolute(row, col);
        cells[newPos] = cell;
        updateProperty(newPos);
        cellUpdated(row, col);
    }
}

/**
  * Save the spreadsheet to \a writer.
  *
  * @param writer XML stream to write to.
  *
  */

void Sheet::Save(Base::Writer &writer) const
{
    DocumentObject::Save(writer);

    // Save cell contents
    writer.incInd(); // indention for 'Cells'
    writer.Stream() << writer.ind() << "<Cells Count=\"" << cells.size() << "\">" << std::endl;

    std::map<CellPos, CellContent*>::const_iterator ci = cells.begin();
    while (ci != cells.end()) {
        ci->second->save(writer);
        ++ci;
    }

    writer.Stream() << writer.ind() << "</Cells>" << std::endl;

    // Save column information
    writer.Stream() << writer.ind() << "<ColumnInfo Count=\"" << columnWidths.size() << "\">" << std::endl;
    writer.incInd(); // indention for 'ColumnInfo'
    std::map<int, int>::const_iterator coli = columnWidths.begin();
    while (coli != columnWidths.end()) {
        writer.Stream() << writer.ind() << "<Column name=\"" << columnName(coli->first) << "\" width=\"" << coli->second << "\" />" << std::endl;
        ++coli;
    }
    writer.decInd(); // indention for 'ColumnInfo'
    writer.Stream() << writer.ind() << "</ColumnInfo>" << std::endl;

    // Save row information
    writer.Stream() << writer.ind() << "<RowInfo Count=\"" << rowHeights.size() << "\">" << std::endl;
    writer.incInd(); // indention for 'RowInfo'

    std::map<int, int>::const_iterator ri = rowHeights.begin();
    while (ri != rowHeights.end()) {
        writer.Stream() << writer.ind() << "<Row name=\"" << rowName(ri->first) << "\"  height=\"" << ri->second << "\" />" << std::endl;
        ++ri;
    }
    writer.decInd(); // indention for 'RowInfo'
    writer.Stream() << writer.ind() << "</RowInfo>" << std::endl;
    writer.decInd(); // indention for 'RowInfo'
}

/**
  * Restore sheet from \a reader.
  *
  * @params reader XML stream to reconstruct sheet object from.
  *
  */

void Sheet::Restore(Base::XMLReader &reader)
{
    int Cnt;

    App::DocumentObject::Restore(reader);

    reader.readElement("Cells");
    Cnt = reader.getAttributeAsInteger("Count");
    for (int i = 0; i < Cnt; i++) {
        reader.readElement("Cell");

        const char* address = reader.hasAttribute("address") ? reader.getAttribute("address") : 0;

        try {
            int row, col;

            addressToRowCol(address, row, col);

            CellContent * cell = getCell(row, col);

            cell->restore(reader);

            int rows, cols;
            if (cell->getSpans(rows, cols) && (rows > 1 || cols > 1)) {
                mergeCells(Range(row, col,
                                 row + rows - 1, col + cols - 1));
            }
        }
        catch (const Base::Exception & e) {
            // Something is wrong, skip this cell
        }
        catch (...) {
        }
    }
    reader.readEndElement("Cells");

    // Column info
    reader.readElement("ColumnInfo");
    Cnt = reader.hasAttribute("Count") ? reader.getAttributeAsInteger("Count") : 0;
    for (int i = 0; i < Cnt; i++) {
        reader.readElement("Column");
        const char* name = reader.hasAttribute("name") ? reader.getAttribute("name") : 0;
        const char * width = reader.hasAttribute("width") ? reader.getAttribute("width") : 0;

        try {
            if (name && width) {
                int col = decodeColumn(name);
                int colWidth = atoi(width);

                setColumnWidth(col, colWidth);
            }
        }
        catch (...) {
            // Something is wrong, skip this column
        }

        reader.readEndElement("Column");
    }
    reader.readEndElement("ColumnInfo");

    // Row info
    reader.readElement("RowInfo");
    Cnt = reader.hasAttribute("Count") ? reader.getAttributeAsInteger("Count") : 0;
    for (int i = 0; i < Cnt; i++) {
        reader.readElement("Row");
        const char* name = reader.hasAttribute("name") ? reader.getAttribute("name") : 0;
        const char * height = reader.hasAttribute("height") ? reader.getAttribute("height") : 0;

        try {
            if (name && height) {
                int row = decodeRow(name);
                int rowHeight = atoi(height);

                setRowHeight(row, rowHeight);
            }
        }
        catch (...) {
            // Something is wrong, skip this row
        }

        reader.readEndElement("Row");
    }
    reader.readEndElement("RowInfo");
}

/**
  * Encode \a row as a string.
  *
  * @param row Row given as a 0-based row position.
  *
  * @returns String with row position, with "1" being the first row.
  *
  */

std::string Spreadsheet::Sheet::rowName(int row)
{
    std::stringstream s;

    s << (row + 1);

    return s.str();
}

/**
  * Encode \a col as a string.
  *
  * @param col Column given as a 0-based column position.
  *
  * @returns String with column position, with "A" being the first column, "B" being the second and so on.
  *
  */

std::string Sheet::columnName(int col)
{
    std::stringstream s;

    if (col < 26)
        s << ((char)('A' + col));
    else
        s << ((char)('A' + (col - 26) / 26 )) << ((char)('A' + (col - 26) % 26));

    return s.str();
}

const int Sheet::CellContent::EXPRESSION_SET       = 1;
const int Sheet::CellContent::STRING_CONTENT_SET   = 2;
const int Sheet::CellContent::ALIGNMENT_SET        = 4;
const int Sheet::CellContent::STYLE_SET            = 8;
const int Sheet::CellContent::BACKGROUND_COLOR_SET = 0x10;
const int Sheet::CellContent::FOREGROUND_COLOR_SET = 0x20;
const int Sheet::CellContent::DISPLAY_UNIT_SET     = 0x40;
const int Sheet::CellContent::COMPUTED_UNIT_SET    = 0x80;
const int Sheet::CellContent::SPANS_SET            = 0x100;
const int Sheet::CellContent::FROZEN_SET           = 0x80000000;

/* Alignment */
const int Sheet::CellContent::ALIGNMENT_LEFT    = 0x1;
const int Sheet::CellContent::ALIGNMENT_HCENTER = 0x2;
const int Sheet::CellContent::ALIGNMENT_RIGHT   = 0x4;
const int Sheet::CellContent::ALIGNMENT_TOP     = 0x10;
const int Sheet::CellContent::ALIGNMENT_VCENTER = 0x20;
const int Sheet::CellContent::ALIGNMENT_BOTTOM  = 0x40;

/**
  * Construct a CellContent object.
  *
  * @param _row   The row of the cell in the spreadsheet that contains is.
  * @param _col   The column of the cell in the spreadsheet that contains is.
  * @param _owner The spreadsheet that owns this cell.
  *
  */

Sheet::CellContent::CellContent(int _row, int _col, const Sheet *_owner)
    : row(_row)
    , col(_col)
    , owner(_owner)
    , used(0)
    , expression(0)
    , stringContent("")
    , alignment(ALIGNMENT_VCENTER)
    , style()
    , foregroundColor(0, 0, 0, 1)
    , backgroundColor(1, 1, 1, 1)
    , displayUnit()
    , computedUnit()
    , rowSpan(0)
    , colSpan(0)
{
    assert(row >=0 && row < Sheet::MAX_ROWS &&
           col >= 0 && col < Sheet::MAX_COLUMNS);
    assert(owner != 0);
}

/**
  * Destroy a CellContent object.
  *
  */

Sheet::CellContent::~CellContent()
{
    if (expression)
        delete expression;
}

/**
  * Set the expression tree to \a expr.
  *
  */

void Sheet::CellContent::setExpression(const Expression *expr)
{
    if (expression)
        delete expression;
    expression = expr;
    setUsed(EXPRESSION_SET);
}

/**
  * Get the expression tree.
  *
  */

const Expression *Sheet::CellContent::getExpression() const
{
    return expression;
}

/**
  * Set the contents to \a content. This is a textual version of the expression tree,
  * usually exactly what was typed by the user.
  */

void Sheet::CellContent::setStringContent(const std::string &content)
{
    if (content != stringContent) {
        stringContent = content;
        setUsed(STRING_CONTENT_SET);
        owner->cellUpdated(row, col);
    }
}

/**
  * Get string content.
  *
  */

bool Sheet::CellContent::getStringContent(std::string & s) const
{
    s = stringContent;
    return isUsed(STRING_CONTENT_SET);
}

/**
  * Set alignment of this cell. Alignment is the or'ed value of
  * vertical and horizontal alignment, given by the constants
  * defined in the class.
  *
  */

void Sheet::CellContent::setAlignment(int _alignment)
{
    if (_alignment != alignment) {
        alignment = _alignment;
        setUsed(ALIGNMENT_SET);
        owner->cellUpdated(row, col);
    }
}

/**
  * Get alignment.
  *
  */

bool Sheet::CellContent::getAlignment(int & _alignment) const
{
    _alignment = alignment;
    return isUsed(ALIGNMENT_SET);
}

/**
  * Set style to the given set \a _style.
  *
  */

void Sheet::CellContent::setStyle(const std::set<std::string> & _style)
{
    if (_style != style) {
        style = _style;
        setUsed(STYLE_SET);
        owner->cellUpdated(row, col);
    }
}

/**
  * Get the style of the cell.
  *
  */

bool Sheet::CellContent::getStyle(std::set<std::string> & _style) const
{
    _style = style;
    return isUsed(STYLE_SET);
}

/**
  * Set foreground (i.e text) color of the cell to \a color.
  *
  */

void Sheet::CellContent::setForeground(const Color &color)
{
    if (color != foregroundColor) {
        foregroundColor = color;
        setUsed(FOREGROUND_COLOR_SET);
        owner->cellUpdated(row, col);
    }
}

/**
  * Get foreground color of the cell.
  *
  */

bool Sheet::CellContent::getForeground(Color &color) const
{
    color = foregroundColor;
    return isUsed(FOREGROUND_COLOR_SET);
}

/**
  * Set background color of the cell to \a color.
  *
  */

void Sheet::CellContent::setBackground(const Color &color)
{
    if (color != backgroundColor) {
        backgroundColor = color;
        setUsed(BACKGROUND_COLOR_SET);
        owner->cellUpdated(row, col);
    }
}

/**
  * Get the background color of the cell into \a color.
  *
  * @returns true if the background color was previously set.
  *
  */

bool Sheet::CellContent::getBackground(Color &color) const
{
    color = backgroundColor;
    return isUsed(BACKGROUND_COLOR_SET);
}

/**
  * Set the display unit for the cell.
  *
  */

void Sheet::CellContent::setDisplayUnit(const std::string &unit)
{
    std::auto_ptr<UnitExpression> e(ExpressionParser::parseUnit(owner, unit.c_str()));
    DisplayUnit newDisplayUnit = DisplayUnit(unit, e->getUnit(), e->getScaler());

    if (newDisplayUnit != displayUnit) {
        displayUnit = newDisplayUnit;
        setUsed(DISPLAY_UNIT_SET);
        owner->cellUpdated(row, col);
    }
}

/**
  * Get the display unit for the cell into unit.
  *
  * @returns true if the display unit was previously set.
  *
  */

bool Sheet::CellContent::getDisplayUnit(Sheet::DisplayUnit &unit) const
{
    unit = displayUnit;
    return isUsed(DISPLAY_UNIT_SET);
}

/**
  * Set the computed unit for the cell to \a unit.
  *
  */

void Sheet::CellContent::setComputedUnit(const Base::Unit &unit)
{
    computedUnit = unit;
    setUsed(COMPUTED_UNIT_SET);
    owner->cellUpdated(row, col);
}

/**
  * Get the computed unit into \a unit.
  *
  * @returns true if the computed unit was previously set.
  *
  */

bool Sheet::CellContent::getComputedUnit(Base::Unit & unit) const
{
    unit = computedUnit;
    return isUsed(COMPUTED_UNIT_SET);
}

/**
  * Set the cell's row and column span to \a rows and \a columns. This
  * is done when cells are merged.
  *
  */

void Sheet::CellContent::setSpans(int rows, int columns)
{
    if (rows != rowSpan || columns != columns) {
        rowSpan = rows;
        colSpan = columns;
        setUsed(SPANS_SET);
        owner->cellSpanChanged(row, col);
    }
}

/**
  * Get the row and column spans for the cell into \a rows and \a columns.
  *
  */

bool Sheet::CellContent::getSpans(int &rows, int &columns) const
{
    rows = rowSpan;
    columns = colSpan;
    return isUsed(SPANS_SET);
}

/**
  * Move the cell to a new position given by \a _row and \a _col.
  *
  */

void Sheet::CellContent::moveAbsolute(int _row, int _col)
{
    row = _row;
    col = _col;
    // FIXME; update expression
}

/**
  * Restore cell contents from \a reader.
  *
  */

void Sheet::CellContent::restore(Base::XMLReader &reader)
{
    const char* style = reader.hasAttribute("style") ? reader.getAttribute("style") : 0;
    const char* alignment = reader.hasAttribute("alignment") ? reader.getAttribute("alignment") : 0;
    const char* content = reader.hasAttribute("content") ? reader.getAttribute("content") : 0;
    const char* foregroundColor = reader.hasAttribute("foregroundColor") ? reader.getAttribute("foregroundColor") : 0;
    const char* backgroundColor = reader.hasAttribute("backgroundColor") ? reader.getAttribute("backgroundColor") : 0;
    const char* displayUnit = reader.hasAttribute("displayUnit") ? reader.getAttribute("displayUnit") : 0;
    const char* rowSpan = reader.hasAttribute("rowSpan") ? reader.getAttribute("rowSpan") : 0;
    const char* colSpan = reader.hasAttribute("colSpan") ? reader.getAttribute("colSpan") : 0;

    // Don't trigger multiple updates below; wait until everything is loaded by calling unfreeze() below.
    freeze();

    if (content)
        setStringContent(content);
    if (style) {
        using namespace boost;
        std::set<std::string> styleSet;

        escaped_list_separator<char> e('\0', '|', '\0');
        std::string line = std::string(style);
        tokenizer<escaped_list_separator<char> > tok(line, e);

        for(tokenizer<escaped_list_separator<char> >::iterator i = tok.begin(); i != tok.end();++i)
            styleSet.insert(*i);
        setStyle(styleSet);
    }
    if (alignment) {
        int alignmentCode = 0;
        using namespace boost;

        escaped_list_separator<char> e('\0', '|', '\0');
        std::string line = std::string(alignment);
        tokenizer<escaped_list_separator<char> > tok(line, e);

        for(tokenizer<escaped_list_separator<char> >::iterator i = tok.begin(); i != tok.end();++i)
            alignmentCode = decodeAlignment(*i, alignmentCode);

        setAlignment(alignmentCode);
    }
    if (foregroundColor) {
        Color color = decodeColor(foregroundColor, Color(0, 0, 0, 1));

        setForeground(color);
    }
    if (backgroundColor) {
        Color color = decodeColor(foregroundColor, Color(1, 1, 1, 1));

        setBackground(color);
    }
    if (displayUnit)
        setDisplayUnit(displayUnit);

    if (rowSpan || colSpan) {
        int rs = rowSpan ? atoi(rowSpan) : 1;
        int cs = colSpan ? atoi(colSpan) : 1;
    }

    unfreeze();
}

/**
  * Save cell contents into \a writer.
  *
  */

void Sheet::CellContent::save(Base::Writer &writer) const
{
    if (!isUsed())
        return;

    writer.incInd(); // indention for 'Cell'
    writer.Stream() << writer.ind() << "<Cell ";

    writer.Stream() << "address=\"" << Sheet::toAddress(row, col) << "\" ";

    if (isUsed(STRING_CONTENT_SET))
        writer.Stream() << "content=\"" << encodeAttribute(stringContent) << "\" ";

    if (isUsed(ALIGNMENT_SET))
        writer.Stream() << "alignment=\"" << encodeAlignment(alignment) << "\" ";

    if (isUsed(STYLE_SET))
        writer.Stream() << "style=\"" << encodeStyle(style) << "\" ";

    if (isUsed(FOREGROUND_COLOR_SET))
        writer.Stream() << "foregroundColor=\"" << encodeColor(foregroundColor) << "\" ";

    if (isUsed(BACKGROUND_COLOR_SET))
        writer.Stream() << "backgroundColor=\"" << encodeColor(backgroundColor) << "\" ";

    if (isUsed(DISPLAY_UNIT_SET))
        writer.Stream() << "displayUnit=\"" << encodeAttribute(displayUnit.stringRep) << "\" ";

    if (isUsed(SPANS_SET)) {
        writer.Stream() << "rowSpan=\"" << rowSpan<< "\" ";
        writer.Stream() << "colSpan=\"" << colSpan << "\" ";
    }

    writer.Stream() << "/>" << std::endl;
    writer.decInd(); // indention for 'Cell'
}

/**
  * Update the \a used member variable with mask (bitwise or'ed).
  *
  */

void Sheet::CellContent::setUsed(int mask)
{
    used |= mask;

    if (!isUsed(FROZEN_SET))
        owner->cellUpdated(row, col);
}

/**
  * Determine whether the bits in \a mask are set in the \a used member variable.
  *
  */

bool Sheet::CellContent::isUsed(int mask) const
{
    return (used & mask) == mask;
}

/**
  * Determine if the any of the contents of the cell is set a non-default value.
  *
  */

bool Sheet::CellContent::isUsed() const
{
    return (used & ~FROZEN_SET) != 0;
}

void Sheet::CellContent::freeze()
{
    setUsed(FROZEN_SET);
}

void Sheet::CellContent::unfreeze()
{
    used &= (~FROZEN_SET);
    owner->cellUpdated(row, col);
}

void Sheet::createRectangles(std::set<std::pair<int, int> > & cells, std::map<std::pair<int, int>, std::pair<int, int> > & rectangles)
{
    while (cells.size() != 0) {
        int row, col;
        int orgRow;
        int rows = 1;
        int cols = 1;

        orgRow = row = (*cells.begin()).first;
        col = (*cells.begin()).second;

        // Expand right first
        while (cells.find(std::make_pair<int,int>(row, col + cols)) != cells.end())
            ++cols;

        // Expand left
        while (cells.find(std::make_pair<int,int>(row, col + cols)) != cells.end()) {
            col--;
            ++cols;
        }

        // Try to expand cell up (the complete row above from [col,col + cols> needs to be in the cells variable)
        bool ok = true;
        while (ok) {
            for (int i = col; i < col + cols; ++i) {
                if ( cells.find(std::make_pair<int,int>(row - 1, i)) == cells.end()) {
                    ok = false;
                    break;
                }
            }
            if (ok) {
                // Complete row
                row--;
                rows++;
            }
            else
                break;
        }

        // Try to expand down (the complete row below from [col,col + cols> needs to be in the cells variable)
        ok = true;
        while (ok) {
            for (int i = col; i < col + cols; ++i) {
                if ( cells.find(std::make_pair<int,int>(orgRow + 1, i)) == cells.end()) {
                   ok = false;
                   break;
                }
            }
            if (ok) {
                // Complete row
                orgRow++;
                rows++;
            }
            else
                break;
        }

        // Remove entries from cell set for this rectangle
        for (int r = row; r < row + rows; ++r)
            for (int c = col; c < col + cols; ++c)
                cells.erase(std::make_pair<int,int>(r, c));

        // Insert into output variable
        rectangles[std::make_pair<int,int>(row, col)] = std::make_pair<int,int>(rows, cols);
    }
}

Sheet::Range::Range(const char * range)
{
    std::string from;
    std::string to;

    assert(range != NULL);

    if (strchr(range, ':') == NULL) {
        from = range;
        to = range;
    }
    else {
        std::string s = range;
        from = s.substr(0, s.find(':'));
        to = s.substr(s.find(':') + 1);
    }

    addressToRowCol(from.c_str(), row_begin, col_begin);
    addressToRowCol(to.c_str(), row_end, col_end);

    row_curr = row_begin;
    col_curr = col_begin;
}

Sheet::Range::Range(int _row_begin, int _col_begin, int _row_end, int _col_end)
    : row_curr(_row_begin)
    , col_curr(_col_begin)
    , row_begin(_row_begin)
    , col_begin(_col_begin)
    , row_end(_row_end)
    , col_end(_col_end)
{
}

bool Sheet::Range::next()
{
    if (row_curr < row_end) {
        row_curr++;

        return true;
    }
    if (col_curr < col_end) {
        if (row_curr == row_end + 1)
            return false;
        row_curr = row_begin;
        ++col_curr;
        return true;
    }
    return false;
}
