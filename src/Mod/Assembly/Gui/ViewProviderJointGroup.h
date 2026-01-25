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

#pragma once

#include <Mod/Assembly/AssemblyGlobal.h>

#include <Gui/ViewProviderDocumentObjectGroup.h>


namespace AssemblyGui
{

class AssemblyGuiExport ViewProviderJointGroup: public Gui::ViewProviderDocumentObjectGroup
{
    PROPERTY_HEADER_WITH_OVERRIDE(AssemblyGui::ViewProviderJointGroup);

public:
    ViewProviderJointGroup();
    ~ViewProviderJointGroup() override;

    /// deliver the icon shown in the tree view. Override from ViewProvider.h
    QIcon getIcon() const override;

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

    // Make the joint group impossible to delete.
    bool onDelete(const std::vector<std::string>&) override
    {
        return false;
    };

    // protected:
    /// get called by the container whenever a property has been changed
    // void onChanged(const App::Property* prop) override;
};

}  // namespace AssemblyGui
