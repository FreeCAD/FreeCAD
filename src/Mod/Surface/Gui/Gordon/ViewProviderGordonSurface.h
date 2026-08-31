// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 Andrew Shkolik <shkolik@gmail.com>                  *
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

#ifndef SURFACEGUI_VIEWPROVIDERGORDONSURFACE_H
#define SURFACEGUI_VIEWPROVIDERGORDONSURFACE_H

#include <App/PropertyLinks.h>
#include <Mod/Part/Gui/ViewProviderSpline.h>

class QMenu;
class QObject;

namespace SurfaceGui
{

class ViewProviderGordonSurface: public PartGui::ViewProviderSpline
{
    PROPERTY_HEADER_WITH_OVERRIDE(SurfaceGui::ViewProviderGordonSurface);
    using References = std::vector<App::PropertyLinkSubList::SubSet>;

public:
    void setupContextMenu(QMenu*, QObject*, const char*) override;
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;
    QIcon getIcon() const override;
    void highlightReferences(const References& profiles, const References& guides, bool on);
};

}  // namespace SurfaceGui

#endif  // SURFACEGUI_VIEWPROVIDERGORDONSURFACE_H
