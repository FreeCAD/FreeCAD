/***************************************************************************
 *   Copyright (c) 2009 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#ifndef PARTDESIGN_Pocket_H
#define PARTDESIGN_Pocket_H

#include <App/PropertyUnits.h>
#include "FeatureSketchBased.h"

namespace PartDesign
{

class PartDesignExport Pocket : public ProfileBased
{
    PROPERTY_HEADER(PartDesign::Pocket);

public:
    Pocket();

    App::PropertyEnumeration    Type;
    App::PropertyLength         Length;
    App::PropertyLength         Length2;
    App::PropertyLength         Offset;

    /** @name methods override feature */
    //@{
    /** Recalculate the feature
      * Extrudes the Sketch in the direction of the sketch face normal
      * If Type is "Length" then Length gives the extrusion length, the direction will be into the support
      * If Type is "ThroughAll" then the extrusion length will be infinite
      * If Type is "UpToFirst" then extrusion will stop at the first face of the support that is cut
      *   by a line through the centre of gravite of the sketch
      * If Type is "UpToFace" then the extrusion will stop at FaceName in the support
      * If Midplane is true, then the extrusion will extend for half of the length on both sides of the sketch plane
      * The created material will be cut out of the sketch support
      */
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    /// returns the type name of the view provider
    const char* getViewProviderName(void) const {
        return "PartDesignGui::ViewProviderPocket";
    }
    //@}
private:
    static const char* TypeEnums[];

};

} //namespace PartDesign


#endif // PART_Pocket_H
