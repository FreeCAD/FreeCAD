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

#include <Mod/Part/App/Part2DObject.h>
#include "FeatureAddSub.h"

class gp_Dir;
class gp_Lin;
class TopoDS_Face;
class TopoDS_Shape;
class TopoDS_Wire;

namespace PartDesign
{

class PartDesignExport ProfileBased : public PartDesign::FeatureAddSub
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::SketchBased);

public:
    enum class ForbiddenAxis {
        NoCheck = 0,
        NotPerpendicularWithNormal = 1,
        NotParallelWithNormal = 2
    };
    ProfileBased();

    // Common properties for all sketch based features
    /// Profile used to create this feature
    App::PropertyLinkSub Profile;
    /// Reverse extrusion direction
    App::PropertyBool    Reversed;
    /// Make extrusion symmetric to sketch plane
    App::PropertyBool    Midplane;
    /// Face to extrude up to
    App::PropertyLinkSub UpToFace;

    App::PropertyBool AllowMultiFace;

    short mustExecute() const override;

    void setupObject() override;

    /** calculates and updates the Placement property based on the features
     * this one is made from: either from Base, if there is one, or from sketch,
     * if there is no base.
     */
    void positionByPrevious();

    /** applies a transform on the Placement of the Sketch or its
     *  support if it has one
      */
    void transformPlacement(const Base::Placement &transform) override;

    /**
     * Verifies the linked Profile and returns it if it is a valid 2D object
     * @param silent if profile property is malformed and the parameter is true
     *               silently returns nullptr, otherwise throw a Base::Exception.
     *               Default is false.
     */
    Part::Part2DObject* getVerifiedSketch(bool silent=false) const;

    /**
     * Verifies the linked Profile and returns it if it is a valid object
     * @param silent if profile property is malformed and the parameter is true
     *               silently returns nullptr, otherwise throw a Base::Exception.
     *               Default is false.
     */
    Part::Feature* getVerifiedObject(bool silent=false) const;

    /**
     * Verifies the linked Object and returns the shape used as profile
     * @param silent if profile property is malformed and the parameter is true
     *               silently returns nullptr, otherwise throw a Base::Exception.
     *               Default is false.
     */
    TopoDS_Shape getVerifiedFace(bool silent = false) const;

    /// Returns the wires the sketch is composed of
    std::vector<TopoDS_Wire> getProfileWires() const;

    /// Returns the face of the sketch support (if any)
    const TopoDS_Face getSupportFace() const;

    Base::Vector3d getProfileNormal() const;

    Part::TopoShape getProfileShape() const;

    /// retrieves the number of axes in the linked sketch (defined as construction lines)
    int getSketchAxisCount() const;

    Part::Feature* getBaseObject(bool silent=false) const override;

    //backwards compatibility: profile property was renamed and has different type now
    void Restore(Base::XMLReader& reader) override;
    void handleChangedPropertyName(Base::XMLReader &reader, const char * TypeName, const char *PropName) override;

    // calculate the through all length
    double getThroughAllLength() const;

protected:
    void remapSupportShape(const TopoDS_Shape&);

    TopoDS_Face getSupportFace(const Part::Part2DObject*) const;
    TopoDS_Face getSupportFace(const App::PropertyLinkSub& link) const;

    /// Extract a face from a given LinkSub
    static void getFaceFromLinkSub(TopoDS_Face& upToFace,
                                   const App::PropertyLinkSub& refFace);

    /// Find a valid face to extrude up to
    static void getUpToFace(TopoDS_Face& upToFace,
                            const TopoDS_Shape& support,
                            const TopoDS_Shape& sketchshape,
                            const std::string& method,
                            const gp_Dir& dir);

    /// Add an offset to the face
    static void addOffsetToFace(TopoDS_Face& upToFace,
                                const gp_Dir& dir,
                                double offset);

    /// Check whether the wire after projection on the face is inside the face
    static bool checkWireInsideFace(const TopoDS_Wire& wire,
                                    const TopoDS_Face& face,
                                    const gp_Dir& dir);

    /// Check whether the line crosses the face (line and face must be on the same plane)
    static bool checkLineCrossesFace(const gp_Lin& line, const TopoDS_Face& face);


    /// Used to suggest a value for Reversed flag so that material is always removed (Groove) or added (Revolution) from the support
    double getReversedAngle(const Base::Vector3d& b, const Base::Vector3d& v) const;
    /// get Axis from ReferenceAxis
    void getAxis(const App::DocumentObject* pcReferenceAxis, const std::vector<std::string>& subReferenceAxis,
                 Base::Vector3d& base, Base::Vector3d& dir, ForbiddenAxis checkAxis) const;

    void onChanged(const App::Property* prop) override;
private:
    bool isParallelPlane(const TopoDS_Shape&, const TopoDS_Shape&) const;
    bool isEqualGeometry(const TopoDS_Shape&, const TopoDS_Shape&) const;
    bool isQuasiEqual(const TopoDS_Shape&, const TopoDS_Shape&) const;
};

} //namespace PartDesign


#endif // PARTDESIGN_SketchBased_H
