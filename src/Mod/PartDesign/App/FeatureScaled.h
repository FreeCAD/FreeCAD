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

class PartDesignExport Scaled : public PartDesign::Transformed
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Scaled);

public:
    Scaled();

    App::PropertyFloat   Factor;
    App::PropertyInteger Occurrences;

   /** @name methods override feature */
    //@{
    short mustExecute() const override;

    /// returns the type name of the view provider
    const char* getViewProviderName() const override {
        return "PartDesignGui::ViewProviderScaled";
    }
    //@}

    /** Create transformations
      * Returns a list containing (Occurrences-1) transformation since the first, untransformed instance
      * is not counted. Each transformation will scale the shape it is applied to by the factor
      * (Factor / (Occurrences - 1))
      * The centre point of the scaling (currently) is the centre of gravity of the first original shape
      * for this reason we need to pass the list of originals into the feature
      */
    // Note: We can't just use the Originals property because this will fail if the Scaled feature
    // is being used inside a MultiTransform feature
    const std::list<gp_Trsf> getTransformations(const std::vector<App::DocumentObject*> originals) override;
};

} //namespace PartDesign


#endif // PARTDESIGN_FeatureScaled_H
