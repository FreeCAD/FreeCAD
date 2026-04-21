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

#include <App/Datums.h>

#include <Mod/Part/PartGlobal.h>

#include "AttachExtension.h"

namespace Part
{

class PartExport DatumPlane: public App::Plane, public AttachExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(Part::DatumPlane);

public:
    DatumPlane();
    ~DatumPlane() override = default;
    const char* getViewProviderName() const override
    {
        return "PartGui::ViewProviderPlane";
    }
};

class PartExport DatumLine: public App::Line, public AttachExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(Part::DatumLine);

public:
    DatumLine();
    ~DatumLine() override = default;
    const char* getViewProviderName() const override
    {
        return "PartGui::ViewProviderLine";
    }
};

class PartExport DatumPoint: public App::Point, public AttachExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(Part::DatumPoint);

public:
    DatumPoint();
    ~DatumPoint() override = default;
    const char* getViewProviderName() const override
    {
        return "PartGui::ViewProviderPoint";
    }
};

class PartExport LocalCoordinateSystem: public App::LocalCoordinateSystem, public AttachExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(Part::LocalCoordinateSystem);

public:
    LocalCoordinateSystem();
    ~LocalCoordinateSystem() override = default;
    const char* getViewProviderName() const override
    {
        return "PartGui::ViewProviderLCS";
    }
};

}  // namespace Part
