/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david@friedli-be.ch>                *
 *   Copyright (c) 2023 Wandererfan <wandererfan@gmail.com>                *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/
#include "PreCompiled.h"
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/VectorPy.h>
#include <App/Document.h>
#include <App/Link.h>

#include "MeasureManager.h"

namespace App
{

std::vector<MeasureHandler> MeasureManager::_mMeasureHandlers;
std::vector<MeasureType*> MeasureManager::_mMeasureTypes;

MeasureManager::MeasureManager()
{
    // Constructor implementation
}


void MeasureManager::addMeasureHandler(const char* module, MeasureTypeMethod typeCb)
{
    _mMeasureHandlers.emplace_back(MeasureHandler {module, typeCb});
}

bool MeasureManager::hasMeasureHandler(const char* module)
{
    for (MeasureHandler& handler : _mMeasureHandlers) {
        if (strcmp(handler.module.c_str(), module) == 0) {
            return true;
        }
    }
    return false;
}

MeasureHandler MeasureManager::getMeasureHandler(const char* module)
{
    for (MeasureHandler handler : _mMeasureHandlers) {
        if (!strcmp(handler.module.c_str(), module)) {
            return handler;
        }
    }

    MeasureHandler empty;
    return empty;
}

MeasureHandler MeasureManager::getMeasureHandler(const App::MeasureSelectionItem& selectionItem)
{
    auto objT = selectionItem.object;

    // Resolve App::Link
    App::DocumentObject* sub = objT.getSubObject();
    if (sub->isDerivedFrom<App::Link>()) {
        auto link = static_cast<App::Link*>(sub);
        sub = link->getLinkedObject(true);
    }

    const char* className = sub->getTypeId().getName();
    std::string mod = Base::Type::getModuleName(className);

    return getMeasureHandler(mod.c_str());
}

MeasureElementType
MeasureManager::getMeasureElementType(const App::MeasureSelectionItem& selectionItem)
{
    auto handler = getMeasureHandler(selectionItem);
    if (handler.module.empty()) {
        return App::MeasureElementType::INVALID;
    }

    auto objT = selectionItem.object;
    return handler.typeCb(objT.getObject(), objT.getSubName().c_str());
}

void MeasureManager::addMeasureType(MeasureType* measureType)
{
    _mMeasureTypes.push_back(measureType);
}

void MeasureManager::addMeasureType(std::string id,
                                    std::string label,
                                    std::string measureObj,
                                    MeasureValidateMethod validatorCb,
                                    MeasurePrioritizeMethod prioritizeCb)
{
    MeasureType* mType =
        new MeasureType {id, label, measureObj, validatorCb, prioritizeCb, false, nullptr};
    _mMeasureTypes.push_back(mType);
}

void MeasureManager::addMeasureType(const char* id,
                                    const char* label,
                                    const char* measureObj,
                                    MeasureValidateMethod validatorCb,
                                    MeasurePrioritizeMethod prioritizeCb)
{
    addMeasureType(std::string(id),
                   std::string(label),
                   std::string(measureObj),
                   validatorCb,
                   prioritizeCb);
}

const std::vector<MeasureType*> MeasureManager::getMeasureTypes()
{
    return _mMeasureTypes;
}


Py::Tuple MeasureManager::getSelectionPy(const App::MeasureSelection& selection)
{
    // Convert selection to python list
    Py::Tuple selectionPy(selection.size());

    int i = 0;
    for (auto it : selection) {

        Py::Dict sel;
        sel.setItem("object", Py::asObject(it.object.getObject()->getPyObject()));
        sel.setItem("subName", Py::String(it.object.getSubName()));
        sel.setItem("pickedPoint", Py::asObject(new Base::VectorPy(it.pickedPoint)));

        selectionPy.setItem(i, sel);

        i++;
    }
    return selectionPy;
}


std::vector<MeasureType*> MeasureManager::getValidMeasureTypes(App::MeasureSelection selection,
                                                               std::string mode)
{
    Base::PyGILStateLocker lock;

    // Convert selection to python list
    Py::Tuple selectionPy = getSelectionPy(selection);

    // Store valid measure types
    std::vector<MeasureType*> validTypes;
    std::pair<int, MeasureType>();


    // Loop through measure types and check if they work with given selection
    for (App::MeasureType* mType : getMeasureTypes()) {

        if (mode != "" && mType->label != mode) {
            continue;
        }


        if (mType->isPython) {
            // Parse Python measure types
            auto measurePyClass = Py::Object(mType->pythonClass);

            Py::Tuple args(1);
            args.setItem(0, selectionPy);

            Py::Object isValid;
            try {
                isValid = measurePyClass.callMemberFunction(std::string("isValidSelection"), args);
            }
            catch (const Py::Exception&) {
                Base::PyException e;
                e.reportException();
                isValid = Py::False();
            }

            if (isValid.as_bool()) {

                // Check priority
                Py::Object isPriority;
                try {
                    isPriority = measurePyClass.callMemberFunction("isPrioritySelection", args);
                }
                catch (const Py::Exception&) {
                    Base::PyException e;
                    e.reportException();
                    isPriority = Py::False();
                }

                if (isPriority.as_bool()) {
                    validTypes.insert(validTypes.begin(), mType);
                }
                else {
                    validTypes.push_back(mType);
                }
            }
        }
        else {
            // Parse c++ measure types

            if (mType->validatorCb && !mType->validatorCb(selection)) {
                continue;
            }

            // Check if the measurement type prioritizes the given selection
            if (mType->prioritizeCb && mType->prioritizeCb(selection)) {
                validTypes.insert(validTypes.begin(), mType);
            }
            else {
                validTypes.push_back(mType);
            }
        }
    }

    return validTypes;
}


}  // namespace App
