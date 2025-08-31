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

#include "PreCompiled.h"

#include <Base/Vector3D.h>

#include "Services.h"

AttacherSubObjectPlacement::AttacherSubObjectPlacement()
    : attacher(std::make_unique<Attacher::AttachEngine3D>())
{
    attacher->setUp({}, Attacher::mmMidpoint);
}

Base::Placement AttacherSubObjectPlacement::calculate(App::SubObjectT object,
                                                         Base::Placement basePlacement) const
{
    attacher->setReferences({object});

    auto calculatedAttachment = attacher->calculateAttachedPlacement(basePlacement);

    return basePlacement.inverse() * calculatedAttachment;
}

std::optional<Base::Vector3d> PartCenterOfMass::ofDocumentObject(App::DocumentObject* object) const
{
    if (const auto feature = dynamic_cast<Part::Feature*>(object)) {
        const auto shape = feature->Shape.getShape();

        if (const auto cog = shape.centerOfGravity()) {
            const Base::Placement comPlacement { *cog, Base::Rotation { } };

            return (feature->Placement.getValue().inverse() * comPlacement).getPosition();
        }
    }

    return {};
}