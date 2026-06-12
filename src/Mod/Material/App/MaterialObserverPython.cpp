// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <functional>
#include <algorithm>

#include <Base/Interpreter.h>

#include "MaterialManager.h"
#include "MaterialObserverPython.h"
#include "MaterialPy.h"

using namespace Materials;
namespace sp = std::placeholders;

std::vector<std::unique_ptr<MaterialObserverPython>> MaterialObserverPython::_instances;

void MaterialObserverPython::addObserver(const Py::Object& obj)
{
    _instances.push_back(std::make_unique<MaterialObserverPython>(obj));
}

void MaterialObserverPython::removeObserver(const Py::Object& obj)
{
    auto it = std::find_if(_instances.begin(),
                           _instances.end(),
                           [&obj](const auto& observer) { return observer->inst == obj; });
    if (it != _instances.end()) {
        _instances.erase(it);
    }
}

void MaterialObserverPython::cleanup()
{
    Base::PyGILStateLocker lock;
    _instances.clear();
}

MaterialObserverPython::MaterialObserverPython(const Py::Object& obj)
    : inst(obj)
{
#define FC_PY_ELEMENT_ARG1(_name1, _signal)                                                       \
    do {                                                                                           \
        FC_PY_GetCallable(obj.ptr(), "slot" #_name1, py##_name1.py);                               \
        if (!py##_name1.py.isNone())                                                               \
            py##_name1.slot = MaterialManager::getManager().signal##_signal.connect(               \
                std::bind(&MaterialObserverPython::slot##_name1, this, sp::_1));                   \
    } while (0)

    FC_PY_ELEMENT_ARG1(CreatedMaterial, CreatedMaterial);
    FC_PY_ELEMENT_ARG1(ChangedMaterial, ChangedMaterial);
    FC_PY_ELEMENT_ARG1(DeletedMaterial, DeletedMaterial);

#undef FC_PY_ELEMENT_ARG1
}

MaterialObserverPython::~MaterialObserverPython() = default;

void MaterialObserverPython::slotCreatedMaterial(const Material& material)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::Object(new MaterialPy(new Material(material)), true));
        Base::pyCall(pyCreatedMaterial.ptr(), args.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e;
        e.reportException();
    }
}

void MaterialObserverPython::slotChangedMaterial(const Material& material)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::Object(new MaterialPy(new Material(material)), true));
        Base::pyCall(pyChangedMaterial.ptr(), args.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e;
        e.reportException();
    }
}

void MaterialObserverPython::slotDeletedMaterial(const Material& material)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::Object(new MaterialPy(new Material(material)), true));
        Base::pyCall(pyDeletedMaterial.ptr(), args.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e;
        e.reportException();
    }
}
