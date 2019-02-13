/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
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


#ifndef PARTDESIGN_FeatureMultiTransform_H
#define PARTDESIGN_FeatureMultiTransform_H

#include <App/PropertyStandard.h>
#include "FeatureTransformed.h"


namespace PartDesign
{

class PartDesignExport MultiTransform : public PartDesign::Transformed
{
    PROPERTY_HEADER(PartDesign::MultiTransform);

public:
    MultiTransform();

    App::PropertyLinkList Transformations;

   /** @name methods override feature */
    //@{
    short mustExecute() const;

    /// returns the type name of the view provider
    const char* getViewProviderName(void) const {
        return "PartDesignGui::ViewProviderMultiTransform";
    }
    //@}

    /** Create transformations
      * Returns a list containing the product of all transformations of the subfeatures given
      * by the Transformations property. Subfeatures can be Mirrored, LinearPattern, PolarPattern and
      * Scaled.
      */
    virtual std::list<gp_Trsf> getTransformations(const std::vector<Part::TopoShape> &) override;

protected:
    virtual void positionBySupport(void);
};

} //namespace PartDesign


#endif // PARTDESIGN_FeatureMultiTransform_H
