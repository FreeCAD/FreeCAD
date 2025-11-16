// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2023 Ondsel <development@ondsel.com>                     *
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

#ifndef ASSEMBLYGUI_VIEWPROVIDER_ViewProviderGroups_H
#define ASSEMBLYGUI_VIEWPROVIDER_ViewProviderGroups_H

#include <Mod/Assembly/AssemblyGlobal.h>

#include <Gui/ViewProviderDocumentObjectGroup.h>


namespace AssemblyGui
{

class AssemblyGuiExport ViewProviderGroupBase: public Gui::ViewProviderDocumentObjectGroup
{
    PROPERTY_HEADER_WITH_OVERRIDE(AssemblyGui::ViewProviderGroupBase);

public:
    ViewProviderGroupBase() = default;
    ~ViewProviderGroupBase() override = default;

    // Prevent dragging of the joints and dropping things inside the joint group.
    bool canDragObjects() const override
    {
        return false;
    };
    bool canDropObjects() const override
    {
        return false;
    };
    bool canDragAndDropObject(App::DocumentObject*) const override
    {
        return false;
    };
};

class AssemblyGuiExport ViewProviderBomGroup: public ViewProviderGroupBase
{
    PROPERTY_HEADER_WITH_OVERRIDE(AssemblyGui::ViewProviderBomGroup);

public:
    ViewProviderBomGroup() = default;
    ~ViewProviderBomGroup() override = default;

    QIcon getIcon() const override;
};

class AssemblyGuiExport ViewProviderJointGroup: public ViewProviderGroupBase
{
    PROPERTY_HEADER_WITH_OVERRIDE(AssemblyGui::ViewProviderJointGroup);

public:
    ViewProviderJointGroup() = default;
    ~ViewProviderJointGroup() override = default;

    QIcon getIcon() const override;

    // Make the joint group impossible to delete.
    bool onDelete(const std::vector<std::string>&) override
    {
        return false;
    };
};

class AssemblyGuiExport ViewProviderSimulationGroup: public ViewProviderGroupBase
{
    PROPERTY_HEADER_WITH_OVERRIDE(AssemblyGui::ViewProviderSimulationGroup);

public:
    ViewProviderSimulationGroup() = default;
    ~ViewProviderSimulationGroup() override = default;

    QIcon getIcon() const override;
};


class AssemblyGuiExport ViewProviderSnapshotGroup: public ViewProviderGroupBase
{
    PROPERTY_HEADER_WITH_OVERRIDE(AssemblyGui::ViewProviderSnapshotGroup);

public:
    ViewProviderSnapshotGroup() = default;
    ~ViewProviderSnapshotGroup() override = default;

    QIcon getIcon() const override;
};

class AssemblyGuiExport ViewProviderViewGroup: public ViewProviderGroupBase
{
    PROPERTY_HEADER_WITH_OVERRIDE(AssemblyGui::ViewProviderViewGroup);

public:
    ViewProviderViewGroup() = default;
    ~ViewProviderViewGroup() override = default;

    QIcon getIcon() const override;
};

}  // namespace AssemblyGui

#endif  // ASSEMBLYGUI_VIEWPROVIDER_ViewProviderGroups_H
