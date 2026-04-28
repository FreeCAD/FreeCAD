// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 Kacper Donat <kacper@kadet.net>                     *
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

#include <Base/Interpreter.h>
#include <Base/Vector3D.h>

#include "Services.h"

AttacherSubObjectPlacement::AttacherSubObjectPlacement()
    : attacher(std::make_unique<Attacher::AttachEngine3D>())
{
    attacher->setUp({}, Attacher::mmMidpoint);
}

Base::Placement AttacherSubObjectPlacement::calculate(
    App::SubObjectT object,
    Base::Placement basePlacement
) const
{
    attacher->setReferences({object});

    auto calculatedAttachment = attacher->calculateAttachedPlacement(basePlacement);

    return basePlacement.inverse() * calculatedAttachment;
}

std::optional<Base::Vector3d> PartCenterOfMass::ofDocumentObject(App::DocumentObject* object) const
{
    if (const auto* feature = freecad_cast<Part::Feature*>(object)) {
        const auto shape = feature->Shape.getShape();

        if (const auto cog = shape.centerOfGravity()) {
            const Base::Placement comPlacement {*cog, Base::Rotation {}};

            return (feature->Placement.getValue().inverse() * comPlacement).getPosition();
        }
    }

    return {};
}

bool PartCenterOfMass::supports(App::DocumentObject* object) const
{
    return object->isDerivedFrom<Part::Feature>();
}

std::optional<PyObject*> ShapeAttributeProvider::getAttribute(
    App::DocumentObject* object,
    const char* attr
) const
{
    if (Base::streq(attr, "Shape")) {
        Base::PyGILStateLocker lock;
        // Special treatment of Shape property
        static PyObject* getShape = nullptr;
        if (!getShape) {
            getShape = Py_None;
            PyObject* mod = PyImport_ImportModule("Part");
            if (!mod) {
                PyErr_Clear();
                return {};
            }

            Py::Object pyMod = Py::asObject(mod);
            if (pyMod.hasAttr("getShape")) {
                getShape = Py::new_reference_to(pyMod.getAttr("getShape"));
            }
        }
        if (getShape != Py_None) {
            Py::Tuple args(1);
            args.setItem(0, Py::asObject(object->getPyObject()));
            auto res = PyObject_CallObject(getShape, args.ptr());
            if (!res) {
                PyErr_Clear();
                return {};
            }

            Py::Object pyres(res, true);
            if (pyres.hasAttr("isNull")) {
                Py::Callable func(pyres.getAttr("isNull"));
                if (!func.apply().isTrue()) {
                    return Py::new_reference_to(res);
                }
            }
        }
    }

    return {};
}

Py::Object PartPseudoShapeProvider::getElement(
    const Py::Object& module,
    const Py::Object& object,
    const std::string& subname
) const
{
    Py::Callable func(module.getAttr("getShape"));
    Py::Tuple tuple(1);
    tuple.setItem(0, object);
    if (subname.empty()) {
        return func.apply(tuple);
    }

    Py::Dict dict;
    dict.setItem("subname", Py::String(subname));
    dict.setItem("needSubElement", Py::True());
    return func.apply(tuple, dict);
}
