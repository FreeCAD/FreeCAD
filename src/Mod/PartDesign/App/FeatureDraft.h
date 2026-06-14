// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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

#include <gp_Pln.hxx>
#include <gp_Dir.hxx>

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <App/PropertyLinks.h>
#include "FeatureDressUp.h"

namespace PartDesign
{
struct PartDesignExport DraftComputeProps
{
    gp_Dir pullDirection;
    gp_Pln neutralPlane;
};

class PartDesignExport Draft: public DressUp
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Draft);

public:
    Draft();

    App::PropertyAngle Angle;
    App::PropertyLinkSub NeutralPlane;
    App::PropertyLinkSub PullDirection;
    App::PropertyBool Reversed;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    /// returns the type name of the view provider
    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderDraft";
    }
    //@}

    /**
     * @brief getLastComputedProps: Returns the Pull Direction and Neutral Plane
     * computed during the last call of the execute method.
     * Note: The returned values might be in the default initialized state if
     * they were not computed or computation failed
     */
    DraftComputeProps getLastComputedProps() const
    {
        return computeProps;
    }

private:
    void handleChangedPropertyType(
        Base::XMLReader& reader,
        const char* TypeName,
        App::Property* prop
    ) override;
    static const App::PropertyAngle::Constraints floatAngle;

    DraftComputeProps computeProps;
};

}  // namespace PartDesign
