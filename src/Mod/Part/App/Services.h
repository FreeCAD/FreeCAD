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

#pragma once

#include <Attacher.h>
#include <App/Services.h>

class AttacherSubObjectPlacement final: public App::SubObjectPlacementProvider
{
public:
    AttacherSubObjectPlacement();

    Base::Placement calculate(App::SubObjectT object, Base::Placement basePlacement) const override;

private:
    std::unique_ptr<Attacher::AttachEngine3D> attacher;
};

class PartCenterOfMass final: public App::CenterOfMassProvider
{
public:
    std::optional<Base::Vector3d> ofDocumentObject(App::DocumentObject* object) const override;
    bool supports(App::DocumentObject* object) const override;
};
