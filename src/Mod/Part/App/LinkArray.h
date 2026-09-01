// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2026 Boyer Pierre-Louis <pierrelouis.boyer@gmail.com>    *
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

#include <list>
#include <vector>

#include <App/Link.h>
#include <Base/Placement.h>
#include <Mod/Part/PartGlobal.h>

class gp_Trsf;

namespace Part
{

class PartExport LinkArray: public App::Link
{
    PROPERTY_HEADER_WITH_EXTENSIONS(Part::LinkArray);
    using inherited = App::Link;

public:
    LinkArray();

    const char* getViewProviderName() const override
    {
        return "PartGui::ViewProviderLinkArray";
    }

    App::DocumentObjectExecReturn* execute() override;
    void onDocumentRestored() override;

    Base::Placement getPlacementOf(
        const std::string& sub,
        App::DocumentObject* targetObj = nullptr
    ) override;

    bool isLink() const override;
    bool isLinkGroup() const override;

protected:
    void onChanged(const App::Property* prop) override;
    virtual std::vector<Base::Placement> getElementPlacements();

    void syncGeneratedElementPlacements(const std::vector<Base::Placement>& placements);
    void syncGeneratedElementLinkPlacements(const std::vector<Base::Placement>& placements);
    void enforceLinkArrayPropertyStatus();

    static Base::Placement placementFromTransform(const gp_Trsf& transform);
    static std::vector<Base::Placement> placementsFromTransforms(
        const std::list<gp_Trsf>& transformations
    );
};

}  // namespace Part
