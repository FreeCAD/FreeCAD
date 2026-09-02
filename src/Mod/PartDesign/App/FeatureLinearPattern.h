// SPDX-License-Identifier: LGPL-2.1-or-later

/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#pragma once

#include <App/PropertyUnits.h>
#include <Mod/Part/App/LinearPatternExtension.h>
#include "FeatureTransformed.h"

class gp_Vec;

namespace PartDesign
{

class PartDesignExport LinearPattern: public PartDesign::Transformed,
                                      public Part::LinearPatternExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(PartDesign::LinearPattern);

public:
    LinearPattern();

    // Virtual override to support PartDesign datum objects
    gp_Dir getDirectionFromProperty(const App::PropertyLinkSub& dirProp) const override;

    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderLinearPattern";
    }
    //@}

    /** Create transformations
     * Returns a list of (Occurrences - 1) transformations since the first, untransformed instance
     * is not counted.
     *
     * Depending on Mode selection list will be constructed differently:
     * 1. For "Overall Length" each transformation will move the shape it is applied to by the
     * distance (Length / (Occurrences - 1)) so that the transformations will cover the total
     * Length.
     * 2. For "Spacing" each transformation will move the shape by the distance explicitly given in
     *    the Offset parameter.
     *
     * If Direction contains a feature and a face name, then the transformation direction will be
     *   the normal of the given face, which must be planar. If it contains an edge name, then the
     *   transformation direction will be parallel to the given edge, which must be linear
     *
     * If Reversed is true, the direction of transformation will be opposite
     */
    const std::list<gp_Trsf> getTransformations(const std::vector<App::DocumentObject*>) override;

protected:
    void handleChangedPropertyType(
        Base::XMLReader& reader,
        const char* TypeName,
        App::Property* prop
    ) override;
};

}  // namespace PartDesign
