// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2022 Matteo Grellier
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <Mod/Part/Gui/ViewProviderSpline.h>

namespace SurfaceGui
{

class ViewProviderBlendCurve: public PartGui::ViewProviderSpline
{
    PROPERTY_HEADER_WITH_OVERRIDE(SurfaceGui::ViewProviderBlendCurve);

public:
    QIcon getIcon() const override;
    void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;

protected:
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;
};

}  // namespace SurfaceGui
