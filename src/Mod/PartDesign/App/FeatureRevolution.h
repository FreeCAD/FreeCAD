/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#ifndef PARTDESIGN_Revolution_H
#define PARTDESIGN_Revolution_H

#include <App/PropertyUnits.h>
#include "FeatureSketchBased.h"

namespace PartDesign
{

class PartDesignExport Revolution : public ProfileBased
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Revolution);

public:
    Revolution();

    App::PropertyVector Base;
    App::PropertyVector Axis;
    App::PropertyAngle  Angle;

    /** if this property is set to a valid link, both Axis and Base properties
     *  are calculated according to the linked line
    */
    App::PropertyLinkSub ReferenceAxis;

    /** @name methods override feature */
    //@{
    /** Recalculate the feature
      * Revolves the Sketch around the given Axis (with basepoint Base)
      * The angle of the revolution is given by Angle.
      * If Midplane is true, then the revolution will extend for half of Angle on both sides of the sketch plane.
      * If Reversed is true then the direction of revolution will be reversed.
      * The created material will be fused with the sketch support (if there is one)
      */
    App::DocumentObjectExecReturn *execute() override;
    short mustExecute() const override;
    /// returns the type name of the view provider
    const char* getViewProviderName() const override {
        return "PartDesignGui::ViewProviderRevolution";
    }
    //@}

    /// suggests a value for Reversed flag so that material is always added to the support
    bool suggestReversed();
protected:
    /// updates Axis from ReferenceAxis
    void updateAxis();

    static const App::PropertyAngle::Constraints floatAngle;
};

} //namespace PartDesign


#endif // PART_Revolution_H
