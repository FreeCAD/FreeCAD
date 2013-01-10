/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2008     *
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



#ifndef PART_PART2DOBJECT_H
#define PART_PART2DOBJECT_H

#include <App/PropertyStandard.h>
#include <Base/Axis.h>

#include "PartFeature.h"

class TopoDS_Face;

namespace Part
{
class Geometry;

/** 2D Shape
  * This is a specialiced version of the PartShape for use with
  * flat (2D) geometry. The Z direction has always to be 0.
  * The position and orientation of the Plane this 2D geometry is
  * referenced is defined by the Placement property. It also
  * has a link to a supporting Face which defines the position
  * in space where it is located. If the support is changed the
  * static methode positionBySupport() is used to calculate a
  * new position for the Part2DObject.
  * This object can be used stand alone or for constraint
  * geometry as its descend Sketcher::SketchObject .
  */

class PartExport Part2DObject : public Part::Feature
{
    PROPERTY_HEADER(Part::Part2DObject);

public:
    Part2DObject();

    /// if the 2DObject lies on the Face of an other object this links to it
    App::PropertyLinkSub        Support;

    /** calculate and update the Placement property based on the Support
      * this methode will calculate the position of the
      * 2D shape on a supporting Face. The Normal(Orientation) get
      * extracted from the Face and for the position an educated guess is made,
      * by examining the placement of the support object (not only the face).
      * If the support is changed this methode is called do determine a new
      * postion of the 2D shape on the supporting Face
      */
    void positionBySupport(void);
    /** applies a transform on the Placement of the Sketch or its
     *  support if it has one
      */
    virtual void transformPlacement(const Base::Placement &transform);

    /// returns the number of construction lines (to be used as axes)
    virtual int getAxisCount(void) const;
    /// retrieves an axis iterating through the construction lines of the sketch (indices start at 0)
    virtual Base::Axis getAxis(int axId) const;

    /** calculate the points where a curve with index GeoId should be trimmed
      * with respect to the rest of the curves contained in the list geomlist
      * and a picked point. The outputs intersect1 and intersect2 specify the
      * tightest boundaries for trimming around the picked point and the
      * indexes GeoId1 and GeoId2 specify the corresponding curves that intersect
      * the curve GeoId.
      */
    static bool seekTrimPoints(const std::vector<Geometry *> &geomlist,
                               int GeoId, const Base::Vector3d &point,
                               int &GeoId1, Base::Vector3d &intersect1,
                               int &GeoId2, Base::Vector3d &intersect2);

    static const int H_Axis;
    static const int V_Axis;
    static const int N_Axis;

    /** @name methods overide Feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute(void);

    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const {
        return "PartGui::ViewProvider2DObject";
    }
    //@}

};

typedef App::FeaturePythonT<Part2DObject> Part2DObjectPython;

} //namespace Part


#endif // PART_PART2DOBJECT_H
