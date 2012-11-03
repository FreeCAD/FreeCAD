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


#ifndef PARTDESIGN_Pad_H
#define PARTDESIGN_Pad_H

#include <App/PropertyUnits.h>
#include <App/PropertyStandard.h>
#include "FeatureAdditive.h"

namespace PartDesign
{

class PartDesignExport Pad : public Additive
{
    PROPERTY_HEADER(PartDesign::Pad);

public:
    Pad();

    App::PropertyEnumeration    Type;
    App::PropertyLength         Length;
    App::PropertyLength         Length2;
    App::PropertyLinkSub        UpToFace;

    /** @name methods override feature */
    //@{
    /** Recalculate the feature
      * Extrudes the Sketch in the direction of the sketch face normal
      * If Type is "Length" then Length gives the extrusion length, the direction will be away from the support
      * If Type is "UpToLast" then the extrusion will stop at the last face of the support
      *  that is cut by a line through the centre of gravite of the sketch
      * If Type is "UpToFirst" then extrusion will stop at the first face of the support
      * If Type is "UpToFace" then the extrusion will stop at FaceName in the support
      * If Type is "TwoLengths" then the extrusion will extend Length in the direction away from the support
      *  and Length2 in the opposite direction
      * If Midplane is true, then the extrusion will extend for half of the length on both sides of the sketch plane
      * If Reversed is true then the direction of revolution will be reversed.
      * The created material will be fused with the sketch support (if there is one)
      */
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    /// returns the type name of the view provider
    const char* getViewProviderName(void) const {
        return "PartDesignGui::ViewProviderPad";
    }
    //@}
private:
    static const char* TypeEnums[];
    //static const char* SideEnums[];
};

} //namespace PartDesign


#endif // PART_Pad_H
