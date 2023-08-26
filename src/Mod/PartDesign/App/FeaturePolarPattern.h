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


#ifndef PARTDESIGN_FeaturePolarPattern_H
#define PARTDESIGN_FeaturePolarPattern_H

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include "FeatureTransformed.h"


namespace PartDesign
{
enum class PolarPatternMode {
    angle,
    offset
};

class PartDesignExport PolarPattern : public PartDesign::Transformed
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::PolarPattern);

public:
    PolarPattern();

    App::PropertyLinkSub     Axis;
    App::PropertyBool        Reversed;
    App::PropertyEnumeration Mode;
    App::PropertyAngle       Angle;
    App::PropertyAngle       Offset;
    App::PropertyIntegerConstraint Occurrences;

   /** @name methods override feature */
    //@{
    short mustExecute() const override;

    /// returns the type name of the view provider
    const char* getViewProviderName() const override {
        return "PartDesignGui::ViewProviderPolarPattern";
    }
    //@}

    /** Create transformations
     * 
      * Returns a list of (Occurrences - 1) transformations since the first, untransformed instance
      * is not counted. Each transformation will rotate the shape it is applied to by the supplied angle.
      * 
      * Depending on Mode selection list will be constructed differently:
      * 1. For "angle" mode each feature will be rotated by (Angle / (Occurrences - 1)) so 
      * that the transformations will cover the total Angle. The only exception is Angle = 360 degrees in 
      * which case the transformation angle will be (Angle / Occurrences) so that the last transformed shape 
      * is not identical with the original shape. 
      * 2. For "offset" mode each feature will be rotated using exact angle from Offset parameter. It can 
      * potentially result in transformation that extends beyond full rotation or results in overlapping shapes.
      * This situations are considered as potential user errors and should be solved by user.
      * 
      * If Axis contains a feature and an edge name, then the transformation axis will be
      * the given edge, which must be linear.
      * 
      * If Reversed is true, the direction of rotation will be opposite.
      */
    const std::list<gp_Trsf> getTransformations(const std::vector<App::DocumentObject*>) override;

protected:
    void handleChangedPropertyType(Base::XMLReader& reader, const char* TypeName, App::Property* prop) override;
    void onChanged(const App::Property* prop) override;

    static const App::PropertyIntegerConstraint::Constraints intOccurrences;
    static const App::PropertyAngle::Constraints floatAngle;

private:
    static const char* ModeEnums[];

    void setReadWriteStatusForMode(PolarPatternMode mode);
};

} //namespace PartDesign


#endif // PARTDESIGN_FeaturePolarPattern_H
