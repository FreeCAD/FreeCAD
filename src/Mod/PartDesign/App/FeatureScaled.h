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


#ifndef PARTDESIGN_FeatureScaled_H
#define PARTDESIGN_FeatureScaled_H

#include <App/PropertyStandard.h>
#include "FeatureTransformed.h"


namespace PartDesign
{

class PartDesignExport Scaled: public PartDesign::Transformed
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Scaled);

public:
    Scaled();

    App::PropertyFloat Factor;
    App::PropertyInteger Occurrences;

    /** @name methods override feature */
    //@{
    short mustExecute() const override;

    /// returns the type name of the view provider
    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderScaled";
    }
    //@}

    /** Apply scale transformation
     * Returns a list containing the same number of shapes as given.
     * Each shape will be scaled by an additional factor of (Factor / (Occurrences - 1)).
     * If there are less shapes than Occurrences, not all factors will be present.
     * If there are more shapes than Occurrences, the factors will repeat.
     * The centre point of the scaling is the centre of mass of each shape.
     */
    std::vector<TopoDS_Shape> applyTransformation(std::vector<TopoDS_Shape> shapes) const override;
};

}  // namespace PartDesign


#endif  // PARTDESIGN_FeatureScaled_H
