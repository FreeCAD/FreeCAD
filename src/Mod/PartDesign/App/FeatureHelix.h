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


#ifndef PARTDESIGN_Helix_H
#define PARTDESIGN_Helix_H

#include <App/PropertyUnits.h>
#include "FeatureSketchBased.h"
#include <TopoDS_Shape.hxx>

namespace PartDesign
{

enum class HelixMode {
    pitch_height_angle,
    pitch_turns_angle,
    height_turns_angle,
    height_turns_growth
};

class PartDesignExport Helix : public ProfileBased
{
    PROPERTY_HEADER(PartDesign::Helix);

public:
    Helix();

    App::PropertyVector      Base;
    App::PropertyVector      Axis;
    App::PropertyLength      Pitch;
    App::PropertyLength      Height;
    App::PropertyFloatConstraint   Turns;
    App::PropertyBool        LeftHanded;
    App::PropertyAngle       Angle;
    App::PropertyDistance    Growth;
    App::PropertyEnumeration Mode;
    App::PropertyBool        Outside;
    App::PropertyBool        HasBeenEdited;

    /** if this property is set to a valid link, both Axis and Base properties
     *  are calculated according to the linked line
    */
    App::PropertyLinkSub ReferenceAxis;

    /** @name methods override feature */
    //@{
    App::DocumentObjectExecReturn* execute(void);
    short mustExecute() const;
    /// returns the type name of the view provider
    const char* getViewProviderName(void) const {
        return "PartDesignGui::ViewProviderHelix";
    }
    //@}

    void proposeParameters(bool force = false);
    double safePitch(void);

protected:
    /// updates Axis from ReferenceAxis
    void updateAxis(void);

    /// generate helix and move it to the right location.
    TopoDS_Shape generateHelixPath(double startOffset0 = 0.0);

    // project shape on plane. Used for detecting self intersection.
    TopoDS_Shape projectShape(const TopoDS_Shape& input, const gp_Ax2& plane);

    // center of profile bounding box
    Base::Vector3d getProfileCenterPoint();

    // handle changed property types for backward compatibility
    virtual void handleChangedPropertyType(Base::XMLReader& reader, const char* TypeName, App::Property* prop);

    void onChanged(const App::Property* prop);

    static const App::PropertyFloatConstraint::Constraints floatTurns;
    static const App::PropertyAngle::Constraints floatAngle;

private:
    static const char* ModeEnums[];

    // Sets the read-only status bit for properties depending on the input mode.
    void setReadWriteStatusForMode(HelixMode inputMode);
};


class PartDesignExport AdditiveHelix : public Helix {

    PROPERTY_HEADER(PartDesign::AdditiveHelix);
public:
    AdditiveHelix();
};


class PartDesignExport SubtractiveHelix : public Helix {

    PROPERTY_HEADER(PartDesign::SubtractiveHelix);
public:
    SubtractiveHelix();
};

} //namespace PartDesign


#endif // PART_Helix_H
