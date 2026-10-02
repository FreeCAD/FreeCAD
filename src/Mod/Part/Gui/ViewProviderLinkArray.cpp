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

#include "ViewProviderLinkArray.h"

#include <App/DocumentObject.h>
#include <Gui/BitmapFactory.h>
#include <Mod/Part/App/LinkArrayCircular.h>
#include <Mod/Part/App/LinkArrayPath.h>
#include <Mod/Part/App/LinkArrayPoint.h>
#include <Mod/Part/App/LinkArrayPolar.h>

#include "TaskLinkArrayParameters.h"

using namespace PartGui;

PROPERTY_SOURCE(PartGui::ViewProviderLinkArray, Gui::ViewProviderLink)

ViewProviderLinkArray::ViewProviderLinkArray() = default;

ViewProviderLinkArray::~ViewProviderLinkArray() = default;

bool ViewProviderLinkArray::doubleClicked()
{
    return doubleClickedObject(getDefaultEditReference());
}

std::optional<bool> ViewProviderLinkArray::doubleClickedOccurrence(const App::SubObjectT& reference)
{
    PartGui::showLinkArrayTask(getObject(), reference);
    return true;
}

QIcon ViewProviderLinkArray::getIcon() const
{
    if (getObject()->isDerivedFrom<Part::LinkArrayCircular>()) {
        return Gui::BitmapFactory().pixmap("Part_CircularLinkArray");
    }
    if (getObject()->isDerivedFrom<Part::LinkArrayPath>()) {
        return Gui::BitmapFactory().pixmap("Part_PathLinkArray");
    }
    if (getObject()->isDerivedFrom<Part::LinkArrayPoint>()) {
        return Gui::BitmapFactory().pixmap("Part_PointLinkArray");
    }
    if (getObject()->isDerivedFrom<Part::LinkArrayPolar>()) {
        return Gui::BitmapFactory().pixmap("Part_PolarLinkArray");
    }
    return Gui::BitmapFactory().pixmap("LinkArray");
}
