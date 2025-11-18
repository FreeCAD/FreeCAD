// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2023 Ondsel <development@ondsel.com>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <cmath>
#include <vector>


#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <App/FeaturePythonPyImp.h>
#include <App/Link.h>
#include <App/PropertyPythonObject.h>
#include <App/Range.h>
#include <Base/Console.h>
#include <Base/Placement.h>
#include <Base/Rotation.h>
#include <Base/Tools.h>
#include <Base/Interpreter.h>
#include <QObject>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/Spreadsheet/App/Cell.h>


#include "AssemblyObject.h"
#include "AssemblyLink.h"
#include "BomObject.h"
#include "BomObjectPy.h"


using namespace Assembly;

// ================================ Assembly Object ============================

PROPERTY_SOURCE(Assembly::BomObject, Spreadsheet::Sheet)

BomObject::BomObject()
    : Spreadsheet::Sheet()
{
    ADD_PROPERTY_TYPE(
        columnsNames,
        ("Index"),
        "Bom",
        (App::PropertyType)(App::Prop_None),
        "List of the columns of the Bill of Materials."
    );

    ADD_PROPERTY_TYPE(
        detailSubAssemblies,
        (true),
        "Bom",
        (App::PropertyType)(App::Prop_None),
        "Detail sub-assemblies components."
    );

    ADD_PROPERTY_TYPE(
        detailParts,
        (true),
        "Bom",
        (App::PropertyType)(App::Prop_None),
        "Detail Parts sub-components."
    );

    ADD_PROPERTY_TYPE(
        onlyParts,
        (false),
        "Bom",
        (App::PropertyType)(App::Prop_None),
        "Only Part containers will be added. Solids like PartDesign Bodies will be ignored."
    );
}
BomObject::~BomObject() = default;

PyObject* BomObject::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new BomObjectPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

App::DocumentObjectExecReturn* BomObject::execute()
{
    generateBOM();

    return Spreadsheet::Sheet::execute();
}

void BomObject::saveCustomColumnData()
{
    // This function saves data that is not automatically generated.
    dataElements.clear();
    std::tuple<App::CellAddress, App::CellAddress> res = getUsedRange();
    int maxRow = std::get<1>(res).row();
    int nameColIndex = getColumnIndex("Name");

    for (int row = 1; row <= maxRow; ++row) {

        size_t col = 0;
        // Here we do not iterate columnName : columnsNames.getValues() because they may have
        // changed
        for (size_t i = 0; i < columnsNames.getValues().size(); ++i) {
            std::string columnName = getText(0, i);
            if (columnName != "Index" && columnName != "Name" && columnName != "Quantity"
                && columnName != "File Name") {
                // Base::Console().warning("row col %d %d\n", row, col);
                //  save custom data if any.
                std::string text = getText(row, col);
                if (text != "") {
                    std::string objName = getText(row, nameColIndex);
                    BomDataElement el(objName, columnName, text);
                    dataElements.push_back(el);
                }
            }

            ++col;
        }
    }
}

void BomObject::generateBOM()
{
    saveCustomColumnData();
    clearAll();
    obj_list.clear();
    size_t row = 0;
    size_t col = 0;

    // Populate headers
    for (auto& columnName : columnsNames.getValues()) {
        setCell(App::CellAddress(row, col), columnName.c_str());
        ++col;
    }
    ++row;

    auto* assembly = getAssembly();
    if (assembly) {
        addObjectChildrenToBom(assembly->getOutList(), row, "");
    }
    else {
        addObjectChildrenToBom(getDocument()->getRootObjectsIgnoreLinks(), row, "");
    }
}

void BomObject::addObjectChildrenToBom(
    std::vector<App::DocumentObject*> objs,
    size_t& row,
    std::string index
)
{
    int quantityColIndex = getColumnIndex("Quantity");
    bool hasQuantityCol = hasQuantityColumn();

    size_t siblingsInitialRow = row;

    if (index != "") {
        index = index + ".";
    }

    size_t sub_i = 1;

    for (auto* child : objs) {
        if (!child) {
            continue;
        }
        if (auto* asmLink = freecad_cast<AssemblyLink*>(child)) {
            child = asmLink->getLinkedAssembly();
            if (!child) {
                continue;
            }
        }
        else if (child->isDerivedFrom<App::Link>()) {
            child = static_cast<App::Link*>(child)->getLinkedObject();
            if (!child) {
                continue;
            }
        }

        if (!child->isDerivedFrom<AssemblyObject>() && !child->isDerivedFrom<App::Part>()
            && !(child->isDerivedFrom<Part::Feature>() && !onlyParts.getValue())) {
            continue;
        }

        if (hasQuantityCol && row != siblingsInitialRow) {
            // Check if the object is not already in (case of links). And if so just increment.
            // Note: an object can be used in several parts. In which case we do no want to blindly
            // increment.
            bool found = false;
            for (size_t i = siblingsInitialRow; i <= row; ++i) {
                size_t idInList = i - 1;  // -1 for the header
                if (idInList < obj_list.size() && child == obj_list[idInList]) {

                    int qty = std::stoi(getText(i, quantityColIndex)) + 1;
                    setCell(App::CellAddress(i, quantityColIndex), std::to_string(qty).c_str());
                    found = true;
                    break;
                }
            }
            if (found) {
                continue;
            }
        }

        std::string sub_index = index + std::to_string(sub_i);
        ++sub_i;

        addObjectToBom(child, row, sub_index);
        ++row;

        if ((child->isDerivedFrom<AssemblyObject>() && detailSubAssemblies.getValue())
            || (!child->isDerivedFrom<AssemblyObject>() && child->isDerivedFrom<App::Part>()
                && detailParts.getValue())) {
            addObjectChildrenToBom(child->getOutList(), row, sub_index);
        }
    }
}

void BomObject::addObjectToBom(App::DocumentObject* obj, size_t row, std::string index)
{
    obj_list.push_back(obj);
    size_t col = 0;
    for (auto& columnName : columnsNames.getValues()) {
        if (columnName == "Index") {
            setCell(App::CellAddress(row, col), (std::string("'") + index).c_str());
        }
        else if (columnName == "Name") {
            setCell(App::CellAddress(row, col), obj->Label.getValue());
        }
        else if (columnName == "File Name") {
            setCell(App::CellAddress(row, col), obj->getDocument()->getFileName());
        }
        else if (columnName == "Quantity") {
            setCell(App::CellAddress(row, col), std::to_string(1).c_str());
        }
        else if (columnName.starts_with(".")) {
            // Column names that start with a dot are considered property names
            // Extract the property name
            std::string baseName = columnName.substr(1);

            auto propertyValue = getBomPropertyValue(obj, baseName);
            if (!propertyValue.empty()) {
                setCell(App::CellAddress(row, col), propertyValue.c_str());
            }
        }
        else {
            // load custom data if any.
            for (auto& el : dataElements) {
                if (el.objName == obj->Label.getValue() && el.columnName == columnName) {
                    setCell(App::CellAddress(row, col), el.value.c_str());
                    break;
                }
            }
        }
        ++col;
    }
}

std::string BomObject::getBomPropertyValue(App::DocumentObject* obj, const std::string& baseName)
{
    App::Property* prop = obj->getPropertyByName(baseName.c_str());

    if (!prop) {
        Base::Console().warning("Property not found: %s\n", baseName.c_str());
        return QObject::tr("N/A").toStdString();
    }

    // Only support a subset of property types for BOM
    if (auto propStr = freecad_cast<App::PropertyString*>(prop)) {
        return propStr->getValue();
    }
    else if (auto propQuantity = freecad_cast<App::PropertyQuantity*>(prop)) {
        return propQuantity->getQuantityValue().getUserString();
    }
    else if (auto propEnum = freecad_cast<App::PropertyEnumeration*>(prop)) {
        return propEnum->getValueAsString();
    }
    else if (auto propFloat = freecad_cast<App::PropertyFloat*>(prop)) {
        return std::to_string(propFloat->getValue());
    }
    else if (auto propInt = freecad_cast<App::PropertyInteger*>(prop)) {
        return std::to_string(propInt->getValue());
    }
    else if (auto propBool = freecad_cast<App::PropertyBool*>(prop)) {
        return propBool->getValue() ? "True" : "False";
    }

    Base::Console().warning("Property type not supported for: %s\n", prop->getName());
    return QObject::tr("Not supported").toStdString();
}

AssemblyObject* BomObject::getAssembly()
{
    for (auto& obj : getInList()) {
        if (obj->isDerivedFrom<AssemblyObject>()) {
            return static_cast<AssemblyObject*>(obj);
        }
    }
    return nullptr;
}

bool BomObject::hasQuantityColumn()
{
    for (auto& columnName : columnsNames.getValues()) {
        if (columnName == "Quantity") {
            return true;
        }
    }
    return false;
}

std::string Assembly::BomObject::getText(size_t row, size_t col)
{
    Spreadsheet::Cell* cell = getCell(App::CellAddress(row, col));
    std::string cellName;
    if (cell) {
        cell->getStringContent(cellName);

        // getStringContent is adding a ' before the string for whatever reason.
        if (!cellName.empty() && cellName.front() == '\'') {
            cellName.erase(0, 1);  // Remove the first character if it's a '
        }
    }

    return cellName;
}

int BomObject::getColumnIndex(std::string name)
{
    int col = 0;
    for (auto& columnName : columnsNames.getValues()) {
        if (columnName == name) {
            return col;
        }
        ++col;
    }
    return -1;
}
