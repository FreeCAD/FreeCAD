// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef MESHPART_FEATUREPATCH_H
#define MESHPART_FEATUREPATCH_H

#include <Mod/Part/App/PartFeature.h>
#include <Mod/MeshPart/MeshPartGlobal.h>

namespace MeshPart
{

class MeshPartExport Patch: public Part::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(MeshPart::Patch);  // NOLINT

public:
    Patch();

    App::PropertyLink Mesh;
    App::PropertyVector P1;
    App::PropertyVector P2;
    App::PropertyVector P3;
    App::PropertyVector P4;
    App::PropertyVector ViewDirection;
    App::PropertyFloat Tolerance;

    App::PropertyIntegerConstraint MaxDegree;
    App::PropertyIntegerConstraint Profiles;
    App::PropertyIntegerConstraint Guides;

    App::DocumentObjectExecReturn* execute() override;
};

}  // namespace MeshPart

#endif  // MESHPART_FEATUREPATCH_H
