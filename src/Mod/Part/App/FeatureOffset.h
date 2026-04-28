// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2016 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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

#include <App/PropertyStandard.h>

#include <Mod/Part/PartGlobal.h>

#include "PartFeature.h"


namespace Part
{

class PartExport Offset: public Part::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Offset);

public:
    Offset();
    ~Offset() override;

    App::PropertyLink Source;
    App::PropertyFloat Value;
    App::PropertyEnumeration Mode;
    App::PropertyEnumeration Join;
    App::PropertyBool Intersection;
    App::PropertyBool SelfIntersection;
    App::PropertyBool Fill;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    const char* getViewProviderName() const override
    {
        return "PartGui::ViewProviderOffset";
    }
    //@}

private:
    static const char* ModeEnums[];
    static const char* JoinEnums[];
};

class PartExport Offset2D: public Offset
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Offset2D);

public:
    Offset2D();
    ~Offset2D() override;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    const char* getViewProviderName() const override
    {
        return "PartGui::ViewProviderOffset2D";
    }
    //@}
};

}  // namespace Part
