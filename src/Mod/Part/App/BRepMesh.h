// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Mod/Part/PartGlobal.h>
#include <App/ComplexGeoData.h>

namespace Part
{

class PartExport BRepMesh
{
public:
    using Facet = Data::ComplexGeoData::Facet;
    using Domain = Data::ComplexGeoData::Domain;
    using Segment = std::vector<std::size_t>;

    void getFacesFromDomains(
        const std::vector<Domain>& domains,
        std::vector<Base::Vector3d>& points,
        std::vector<Facet>& faces
    );
    std::vector<Segment> createSegments() const;

private:
    std::vector<std::size_t> domainSizes;
};

}  // namespace Part
