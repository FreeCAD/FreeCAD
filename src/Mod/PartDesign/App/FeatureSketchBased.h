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


#ifndef PARTDESIGN_SketchBased_H
#define PARTDESIGN_SketchBased_H

#include <App/PropertyStandard.h>
#include <Mod/Part/App/Part2DObject.h>
#include "Feature.h"

class TopoDS_Shape;
class TopoDS_Face;
class TopoDS_Wire;
class gp_Dir;
class gp_Lin;

namespace PartDesign
{

class PartDesignExport SketchBased : public PartDesign::Feature
{
    PROPERTY_HEADER(PartDesign::SketchBased);

public:
    SketchBased();

    /// Common properties for all sketch based features
    App::PropertyLink   Sketch;
    /// Reverse extrusion direction
    App::PropertyBool       Reversed;
    /// Make extrusion symmetric to sketch plane
    App::PropertyBool       Midplane;

    short mustExecute() const;

    /** calculates and updates the Placement property based on the Sketch
     *  or its support if it has one
      */
    void positionBySketch(void);
    /** applies a transform on the Placement of the Sketch or its
     *  support if it has one
      */
    virtual void transformPlacement(const Base::Placement &transform);

    /// Verifies the linked Sketch object
    Part::Part2DObject* getVerifiedSketch() const;
    /// Returns the wires the sketch is composed of
    std::vector<TopoDS_Wire> getSketchWires() const;
    /// Returns the face of the sketch support (if any)
    const TopoDS_Face getSupportFace() const;
    /// Returns the sketch support feature or NULL
    Part::Feature* getSupport() const;
    /// Returns the sketch support shape (if any)
    const TopoDS_Shape& getSupportShape() const;

    /// retrieves the number of axes in the linked sketch (defined as construction lines)
    int getSketchAxisCount(void) const;

protected:
    void onChanged(const App::Property* prop);
    TopoDS_Face validateFace(const TopoDS_Face&) const;
    TopoDS_Shape makeFace(const std::vector<TopoDS_Wire>&) const;
    TopoDS_Shape makeFace(std::list<TopoDS_Wire>&) const; // for internal use only
    bool isInside(const TopoDS_Wire&, const TopoDS_Wire&) const;
    bool isParallelPlane(const TopoDS_Shape&, const TopoDS_Shape&) const;
    bool isEqualGeometry(const TopoDS_Shape&, const TopoDS_Shape&) const;
    bool isQuasiEqual(const TopoDS_Shape&, const TopoDS_Shape&) const;
    void remapSupportShape(const TopoDS_Shape&);
    TopoDS_Shape refineShapeIfActive(const TopoDS_Shape&) const;

    /// Extract a face from a given LinkSub
    static void getUpToFaceFromLinkSub(TopoDS_Face& upToFace,
                                       const App::PropertyLinkSub& refFace);

    /// Find a valid face to extrude up to
    static void getUpToFace(TopoDS_Face& upToFace,
                            const TopoDS_Shape& support,
                            const TopoDS_Face& supportface,
                            const TopoDS_Shape& sketchshape,
                            const std::string& method,
                            const gp_Dir& dir);
    /**
      * Generate a linear prism
      * It will be a stand-alone solid created with BRepPrimAPI_MakePrism
      */
    static void generatePrism(TopoDS_Shape& prism,
                              const TopoDS_Shape& sketchshape,
                              const std::string& method,
                              const gp_Dir& direction,
                              const double L,
                              const double L2,
                              const bool midplane,
                              const bool reversed);

    /// Check whether the wire after projection on the face is inside the face
    static const bool checkWireInsideFace(const TopoDS_Wire& wire,
                                          const TopoDS_Face& face,
                                          const gp_Dir& dir);

    /// Check whether the line crosses the face (line and face must be on the same plane)
    static const bool checkLineCrossesFace(const gp_Lin& line, const TopoDS_Face& face);

private:
    class Wire_Compare;
};

} //namespace PartDesign


#endif // PARTDESIGN_SketchBased_H
