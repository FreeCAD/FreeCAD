// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2019 Manuel Apeltauer, direkt cnc-systeme GmbH          *
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

#include "PartFeature.h"
#include <App/PropertyLinks.h>
#include <App/PropertyUnits.h>

#include <Mod/Part/PartGlobal.h>

namespace Part
{

class PartExport ProjectOnSurface: public Part::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::ProjectOnSurface);

public:
    ProjectOnSurface();

    App::PropertyEnumeration Mode;
    App::PropertyLength Height;
    App::PropertyDistance Offset;
    App::PropertyDirection Direction;
    App::PropertyLinkSub SupportFace;
    App::PropertyLinkSubList Projection;

    static constexpr const char* AllMode = "All";
    static constexpr const char* FacesMode = "Faces";
    static constexpr const char* EdgesMode = "Edges";

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn* execute() override;
    const char* getViewProviderName() const override;
    //@}

private:
    void tryExecute();
    TopoDS_Face getSupportFace() const;
    std::vector<TopoDS_Shape> getProjectionShapes() const;
    std::vector<TopoDS_Shape> createProjectedWire(
        const TopoDS_Shape& shape,
        const TopoDS_Face& supportFace,
        const gp_Dir& dir
    );
    TopoDS_Face createFaceFromWire(
        const std::vector<TopoDS_Shape>& wires,
        const TopoDS_Face& supportFace
    ) const;
    TopoDS_Face createFaceFromParametricWire(
        const std::vector<TopoDS_Wire>& wires,
        const TopoDS_Face& supportFace
    ) const;
    TopoDS_Shape createSolidIfHeight(const TopoDS_Face& face) const;
    std::vector<TopoDS_Wire> createWiresFromWires(
        const std::vector<TopoDS_Shape>& wires,
        const TopoDS_Face& supportFace
    ) const;
    std::vector<TopoDS_Wire> getWires(const TopoDS_Face& face) const;
    std::vector<TopoDS_Shape> projectFace(
        const TopoDS_Face& face,
        const TopoDS_Face& supportFace,
        const gp_Dir& dir
    );
    std::vector<TopoDS_Shape> projectWire(
        const TopoDS_Shape& wire,
        const TopoDS_Face& supportFace,
        const gp_Dir& dir
    );
    TopoDS_Wire fixWire(const TopoDS_Shape& shape, const TopoDS_Face& supportFace) const;
    TopoDS_Wire fixWire(const std::vector<TopoDS_Edge>& edges, const TopoDS_Face& supportFace) const;
    std::vector<TopoDS_Shape> filterShapes(const std::vector<TopoDS_Shape>& shapes) const;
    TopoDS_Shape createCompound(const std::vector<TopoDS_Shape>& shapes);
    TopLoc_Location getOffsetPlacement() const;
};

}  // namespace Part
