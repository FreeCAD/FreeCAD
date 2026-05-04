// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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

#include "FeatureSketchBased.h"

namespace PartDesign
{

class PartDesignExport Loft: public ProfileBased
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Loft);

public:
    Loft();

    App::PropertyLinkSubList Sections;
    App::PropertyBool Ruled;
    App::PropertyBool Closed;

    /** @name methods override feature */
    //@{
    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    /// returns the type name of the view provider
    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderLoft";
    }
    //@}

    static std::vector<Part::TopoShape> getSectionShape(
        const char* name,
        App::DocumentObject* obj,
        const std::vector<std::string>& subname,
        size_t expected_size = 0
    );

protected:
    // handle changed property
    void handleChangedPropertyType(
        Base::XMLReader& reader,
        const char* TypeName,
        App::Property* prop
    ) override;

private:
    // static const char* TypeEnums[];
    // static const char* SideEnums[];
};

class PartDesignExport AdditiveLoft: public Loft
{

    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::AdditiveLoft);

public:
    AdditiveLoft();
};

class PartDesignExport SubtractiveLoft: public Loft
{

    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::SubtractiveLoft);

public:
    SubtractiveLoft();
};

}  // namespace PartDesign
