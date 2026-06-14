// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Ondsel (PL Boyer) <development@ondsel.com>         *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#pragma once

#include <Gui/ViewProviderLine.h>
#include <Gui/ViewProviderPlane.h>
#include <Gui/ViewProviderPoint.h>
#include <Gui/ViewProviderCoordinateSystem.h>
#include <QCoreApplication>

#include <Mod/Part/PartGlobal.h>

#include <Mod/Part/Gui/ViewProviderAttachExtension.h>

namespace PartGui
{

class PartGuiExport ViewProviderLine: public Gui::ViewProviderLine,
                                      PartGui::ViewProviderAttachExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(PartGui::ViewProviderLine);

public:
    ViewProviderLine();
    ~ViewProviderLine() override = default;

    bool doubleClicked() override;
};

class PartGuiExport ViewProviderPlane: public Gui::ViewProviderPlane,
                                       PartGui::ViewProviderAttachExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(PartGui::ViewProviderPlane);

public:
    ViewProviderPlane();
    ~ViewProviderPlane() override = default;

    bool doubleClicked() override;
};

class PartGuiExport ViewProviderPoint: public Gui::ViewProviderPoint,
                                       PartGui::ViewProviderAttachExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(PartGui::ViewProviderPoint);

public:
    ViewProviderPoint();
    ~ViewProviderPoint() override = default;

    bool doubleClicked() override;
};

class PartGuiExport ViewProviderLCS: public Gui::ViewProviderCoordinateSystem,
                                     PartGui::ViewProviderAttachExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(PartGui::ViewProviderLCS);

public:
    ViewProviderLCS();
    ~ViewProviderLCS() override = default;

    bool doubleClicked() override;
};

}  // namespace PartGui
