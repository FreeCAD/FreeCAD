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
#include <App/Document.h>
#include <App/DocumentObjectPy.h>
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

bool Sheet::clearAll()
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

bool Sheet::mergeCells(const std::string &from, const std::string &to)
{
    CellPos fromPos = addressToCellPos(from.c_str());
    CellPos toPos =  addressToCellPos(to.c_str());
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

Sheet::CellContent *Sheet::getCell(int row, int col) const
{
    assert(row >= 0 && row < MAX_ROWS &&
           col >= 0 && row < MAX_COLUMNS);

    return getCell( encodePos(row, col) );
}

Sheet::CellContent *Sheet::getCell(const std::string &cell) const
{
    int row, col;

    addressToRowCol(cell.c_str(), row, col);

    return getCell( row, col );
}

void Sheet::addressToRowCol(const char * address, int & row, int &col)
{
    int i = 0;
    static const boost::regex e("\\${0,1}([A-Za-z]+)\\${0,1}([0-9]+)");
    boost::cmatch cm;

    if (boost::regex_match(address, cm, e)) {
        const boost::sub_match<const char *> colstr = cm[1];
        const boost::sub_match<const char *> rowstr = cm[2];

        if (colstr.length() == 1) {
            std::string s = colstr.str();
            if ((s[0] >= 'A' && s[0] <= 'Z'))
                col = s[0] - 'A';
            else
                col = s[0] - 'a';
        }
        else {
            std::string s = colstr.str();

            col = 0;
            for (std::string::const_reverse_iterator i = s.rbegin(); i != s.rend(); ++i) {
                int v;

                if ((*i >= 'A' && *i <= 'Z'))
                    v = *i - 'A';
                else
                    v = *i - 'a';

                col = col * 26 + v;
            }
            col += 26;
        }

        row = strtol(rowstr.str().c_str(), 0, 10) - 1;
    }
    else
        throw Base::Exception("Invalid cell specifier.");
}

Sheet::CellPos Sheet::addressToCellPos(const char *address)
{
    int row, col;

    addressToRowCol(address, row, col);

    return encodePos(row, col);
}

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

std::string Sheet::toAddress(CellPos key)
{
    int row, col;

    decodePos(key, row, col);

    return toAddress(row, col);
}

void Sheet::setCell(const char * address, const char * contents)
{
    int row, col;

    assert(address != 0 &&  contents != 0);

    addressToRowCol(address, row, col);

    setCell(row, col, contents);
}

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

PyObject *Sheet::getPyObject(void)
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new SheetPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

Property * Sheet::getProperty(CellPos key) const
{
    return props.getDynamicPropertyByName(toAddress(key).c_str());
}

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

void Sheet::getCellAddress(const Property *prop, int &row, int &col)
{
    std::map<const App::Property*, CellPos >::const_iterator i = propAddress.find(prop);

    if (i != propAddress.end()) {
        decodePos((*i).second, row, col);
    }
    else
        throw Base::Exception("Property is not a cell");
}

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

App::DocumentObjectExecReturn *Sheet::execute(void)
{
    return App::DocumentObject::StdReturn;
}

DocumentObjectExecReturn *Sheet::recompute(void)
{
    return App::DocumentObject::StdReturn;
}

short Sheet::mustExecute(void) const
{
    return 0;
}

SheetObserver::SheetObserver(App::Document * document, Sheet *_sheet)
    : DocumentObserver(document)
    , sheet(_sheet)
    , refCount(1)
{
}

void SheetObserver::slotCreatedDocument(const App::Document &Doc)
{
}

void SheetObserver::slotDeletedDocument(const App::Document &Doc)
{
}

void SheetObserver::slotCreatedObject(const DocumentObject &Obj)
{
}

void SheetObserver::slotDeletedObject(const DocumentObject &Obj)
{
    // FIXME: We should recompute any cells referencing this object. How?
}

void SheetObserver::slotChangedObject(const DocumentObject &Obj, const Property &Prop)
{
    sheet->recomputeDependants(&Prop);
}

void SheetObserver::ref()
{
    refCount++;
}

bool SheetObserver::unref()
{
    refCount--;
    return refCount;
}

bool Sheet::clear(const char * address, bool all)
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

bool Sheet::isMergedCell(int row, int col) const
{
    CellPos pos = encodePos(row, col);

    return mergedCells.find(pos) != mergedCells.end();
}

void Sheet::setColumnWidth(int col, int width)
{
    if (width >= 0) {
        int oldValue = columnWidths[col];

        columnWidths[col] = width;

        if (oldValue != width)
            columnWidthChanged(col, width);
    }
}

int Sheet::getColumnWidth(int col) const
{
    if (columnWidths.find(col) != columnWidths.end())
        return columnWidths.at(col);
    else
        return 0;
}

void Sheet::setRowHeight(int row, int height)
{
    if (height >= 0) {
        int oldValue = rowHeights[row];

        rowHeights[row] = height;

        if (oldValue != height)
            rowHeightChanged(row, height);
    }
}

int Sheet::getRowHeight(int row) const
{
    if (rowHeights.find(row) != rowHeights.end())
        return rowHeights.at(row);
    else
        return 0;
}

std::vector<std::string> Sheet::getUsedCells() const
{
    std::set<CellPos> usedSet;
    std::vector<std::string> usedCells;

    // Insert int usedSet
    for (std::map<CellPos, CellContent*>::const_iterator i = cells.begin(); i != cells.end(); ++i)
        usedSet.insert(i->first);

    for (std::set<CellPos>::const_iterator i = usedSet.begin(); i != usedSet.end(); ++i)
        usedCells.push_back(toAddress(*i));

    return usedCells;
}

std::string Property::encodeAttribute(const std::string& str) const
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

int Sheet::decodeRow(const char * row)
{
    char * end;
    int i = strtol(row, &end, 10 || !*end);

    if (i <0 || i >= MAX_ROWS)
        throw Base::Exception("Invalid row specification.");

    return i;
}

int Sheet::decodeColumn(const char * col)
{
    char * end;
    int i = strtol(col, &end, 10);

    if (i <0 || i >= MAX_COLUMNS || !*end)
        throw Base::Exception("Invalid row specification.");

    return i;
}

void Sheet::moveCell(Sheet::CellPos currPos, Sheet::CellPos newPos)
{
}

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
            if (cell->getSpans(rows, cols) && (rows > 1 || cols > 1))
                mergeCells(toAddress(row, col), toAddress(row + rows - 1, col + cols - 1));
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

std::string Spreadsheet::Sheet::rowName(int row)
{
    std::stringstream s;

    s << (row + 1);

    return s.str();
}

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

/* Alignment */
const int Sheet::CellContent::ALIGNMENT_LEFT    = 0x1;
const int Sheet::CellContent::ALIGNMENT_HCENTER = 0x2;
const int Sheet::CellContent::ALIGNMENT_RIGHT   = 0x4;
const int Sheet::CellContent::ALIGNMENT_TOP     = 0x10;
const int Sheet::CellContent::ALIGNMENT_VCENTER = 0x20;
const int Sheet::CellContent::ALIGNMENT_BOTTOM  = 0x40;

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

Sheet::CellContent::~CellContent()
{
    if (expression)
        delete expression;
}

void Sheet::CellContent::setExpression(const Expression *expr)
{
    if (expression)
        delete expression;
    expression = expr;
    setUsed(EXPRESSION_SET);
}

const Expression *Sheet::CellContent::getExpression() const
{
    return expression;
}

void Sheet::CellContent::setStringContent(const std::string &content)
{
    if (content != stringContent) {
        stringContent = content;
        setUsed(STRING_CONTENT_SET);
        owner->cellUpdated(row, col);
    }
}

bool Sheet::CellContent::getStringContent(std::string & s) const
{
    s = stringContent;
    return isUsed(STRING_CONTENT_SET);
}

void Sheet::CellContent::setAlignment(int _alignment)
{
    if (_alignment != alignment) {
        alignment = _alignment;
        setUsed(ALIGNMENT_SET);
        owner->cellUpdated(row, col);
    }
}

bool Sheet::CellContent::getAlignment(int & _alignment) const
{
    _alignment = alignment;
    return isUsed(ALIGNMENT_SET);
}

void Sheet::CellContent::setStyle(const std::set<std::string> & _style)
{
    if (_style != style) {
        style = _style;
        setUsed(STYLE_SET);
        owner->cellUpdated(row, col);
    }
}

bool Sheet::CellContent::getStyle(std::set<std::string> & _style) const
{
    _style = style;
    return isUsed(STYLE_SET);
}

void Sheet::CellContent::setForeground(const Color &color)
{
    if (color != foregroundColor) {
        foregroundColor = color;
        setUsed(FOREGROUND_COLOR_SET);
        owner->cellUpdated(row, col);
    }
}

bool Sheet::CellContent::getForeground(Color &color) const
{
    color = foregroundColor;
    return isUsed(FOREGROUND_COLOR_SET);
}

void Sheet::CellContent::setBackground(const Color &color)
{
    if (color != backgroundColor) {
        backgroundColor = color;
        setUsed(BACKGROUND_COLOR_SET);
        owner->cellUpdated(row, col);
    }
}

bool Sheet::CellContent::getBackground(Color &color) const
{
    color = backgroundColor;
    return isUsed(BACKGROUND_COLOR_SET);
}

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

bool Sheet::CellContent::getDisplayUnit(Sheet::DisplayUnit &unit) const
{
    unit = displayUnit;
    return isUsed(DISPLAY_UNIT_SET);
}

bool Sheet::CellContent::setComputedUnit(const Base::Unit &unit)
{
    computedUnit = unit;
    setUsed(COMPUTED_UNIT_SET);
    owner->cellUpdated(row, col);
}

bool Sheet::CellContent::getComputedUnit(Base::Unit & unit) const
{
    unit = computedUnit;
    return isUsed(COMPUTED_UNIT_SET);
}

void Sheet::CellContent::setSpans(int rows, int columns)
{
    if (rows != rowSpan || columns != columns) {
        rowSpan = rows;
        colSpan = columns;
        setUsed(SPANS_SET);
        owner->cellSpanChanged(row, col);
    }
}

bool Sheet::CellContent::getSpans(int &rows, int &columns) const
{
    rows = rowSpan;
    columns = colSpan;
    return isUsed(SPANS_SET);
}

void Sheet::CellContent::move(int deltaRow, int deltaCol)
{
    row += deltaRow;
    col += deltaCol;
}

void Sheet::CellContent::moveAbsolute(int _row, int _col)
{
    row = _row;
    col = _col;
}

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
}

void Sheet::CellContent::save(Base::Writer &writer) const
{
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

void Sheet::CellContent::setUsed(int mask)
{
    used |= mask;
}

bool Sheet::CellContent::isUsed(int mask) const {
    return (used & mask) == mask;
}


