// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once


#include <vector>
#include <tuple>
#include <functional>
#include <string>
#include <Python.h>
#include <App/DocumentObject.h>
#include <Base/Vector3D.h>
#include <App/DocumentObserver.h>
#include <Base/Interpreter.h>

#include <FCGlobal.h>

namespace App
{

// Add your class methods and member variables here
enum class MeasureElementType
{
    INVALID,
    POINT,
    LINE,
    LINESEGMENT,
    CIRCLE,
    ARC,
    CURVE,  // Has a length but no radius or axis
    PLANE,
    CYLINDER,
    VOLUME,
    SURFACE,
};


struct MeasureSelectionItem
{
    App::SubObjectT object;
    Base::Vector3d pickedPoint;
};


using MeasureSelection = std::vector<MeasureSelectionItem>;
using MeasureValidateMethod = std::function<bool(const MeasureSelection&)>;
using MeasurePrioritizeMethod = std::function<bool(const MeasureSelection&)>;
using MeasureTypeMethod = std::function<MeasureElementType(App::DocumentObject*, const char*)>;

struct MeasureType
{
    std::string identifier;
    std::string label;
    std::string measureObject;

    // Checks if the measurement works with a given selection
    MeasureValidateMethod validatorCb;

    // Allows to prioritize this over other measurement types when the measurement type is picked
    // implicitly from the selection. Gets called only when validatorCb returned true for the given
    // selection
    MeasurePrioritizeMethod prioritizeCb;

    bool isPython;
    PyObject* pythonClass;
};

struct MeasureHandler
{
    std::string module;
    MeasureTypeMethod typeCb;
};


class AppExport MeasureManager
{
public:
    MeasureManager();

    static void addMeasureHandler(const char* module, MeasureTypeMethod typeCb);
    static bool hasMeasureHandler(const char* module);
    static MeasureHandler getMeasureHandler(const char* module);
    static MeasureHandler getMeasureHandler(const App::MeasureSelectionItem& selectionItem);
    static MeasureElementType getMeasureElementType(const App::MeasureSelectionItem& selectionItem);
    static void addMeasureType(MeasureType* measureType);
    static void addMeasureType(std::string id,
                               std::string label,
                               std::string measureObj,
                               MeasureValidateMethod validatorCb,
                               MeasurePrioritizeMethod prioritizeCb);
    static void addMeasureType(const char* id,
                               const char* label,
                               const char* measureObj,
                               MeasureValidateMethod validatorCb,
                               MeasurePrioritizeMethod prioritizeCb);
    static const std::vector<MeasureType*> getMeasureTypes();
    static Py::Tuple getSelectionPy(const App::MeasureSelection& selection);
    static std::vector<MeasureType*> getValidMeasureTypes(App::MeasureSelection selection,
                                                          std::string mode);


private:
    static std::vector<MeasureHandler> _mMeasureHandlers;
    static std::vector<MeasureType*> _mMeasureTypes;
};


}  // namespace App