// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
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

#include <QCoreApplication>

#include <Base/BoundBox.h>
#include <Mod/Assembly/AssemblyGlobal.h>

#include <Gui/ViewProviderPart.h>

class SoBaseColor;
class SoDrawStyle;
class SoSeparator;
class SoSwitch;

namespace Gui
{
class SoFCBoundingBox;
}

namespace AssemblyGui
{

class AssemblyGuiExport ViewProviderAssemblyLink: public Gui::ViewProviderPart
{
    Q_DECLARE_TR_FUNCTIONS(AssemblyGui::ViewProviderAssemblyLink)
    PROPERTY_HEADER_WITH_OVERRIDE(AssemblyGui::ViewProviderAssemblyLink);

public:
    ViewProviderAssemblyLink();
    ~ViewProviderAssemblyLink() override;

    void attach(App::DocumentObject*) override;

    /// deliver the icon shown in the tree view. Override from ViewProvider.h
    QIcon getIcon() const override;

    bool setEdit(int ModNum) override;

    bool doubleClicked() override;

    // When the assembly link is deleted, we delete all its content as well.
    bool onDelete(const std::vector<std::string>& subNames) override;

    // Prevent deletion of the link assembly's content.
    bool canDelete(App::DocumentObject*) const override
    {
        return false;
    };

    // Prevent drag/drop of objects within the assembly link.
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

    void updateData(const App::Property*) override;
    void finishRestoring() override;
    void setupContextMenu(QMenu*, QObject*, const char*) override;
    void setLightweightPlaceholderVisible(bool visible);

private:
    void updateLightweightPlaceholder();
    void syncLightweightPlaceholderVisibility();
    static bool lightweightProxyBounds(const App::DocumentObject&, Base::BoundBox3d&);

private:
    SoSwitch* lightweightPlaceholderSwitch {nullptr};
    SoSeparator* lightweightPlaceholderRoot {nullptr};
    SoBaseColor* lightweightPlaceholderColor {nullptr};
    SoDrawStyle* lightweightPlaceholderStyle {nullptr};
    Gui::SoFCBoundingBox* lightweightPlaceholderBox {nullptr};
    bool lightweightPlaceholderVisible {true};
    bool lightweightPlaceholderRenderable {false};
};

}  // namespace AssemblyGui
