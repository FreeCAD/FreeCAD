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
#include "FeatureAddSub.h"

class TopoDS_Shape;
class TopoDS_Face;
class TopoDS_Wire;
class gp_Dir;
class gp_Lin;

namespace PartDesign
{

class PartDesignExport ProfileBased : public PartDesign::FeatureAddSub
{
    PROPERTY_HEADER(PartDesign::SketchBased);

public:
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
    /// Force claim linked profile as children
    App::PropertyBool    ClaimChildren;

    short mustExecute() const;

    /** calculates and updates the Placement property based on the features
     * this one is made from: either from Base, if there is one, or from sketch,
     * if there is no base.
     */
    void positionByPrevious(void);

    /** applies a transform on the Placement of the Sketch or its
     *  support if it has one
      */
    virtual void transformPlacement(const Base::Placement &transform);

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
     * @param silent if profirle property is malformed and the parameter is true
     *               silently returns nullptr, otherwise throw a Base::Exception.
     *               Default is false.
     */
    TopoShape getVerifiedFace(bool silent = false) const;
    TopoDS_Shape getVerifiedFaceOld(bool silent = false) const;
    
    /// Returns the wires the sketch is composed of
    std::vector<TopoShape> getProfileWires() const;
    std::vector<TopoDS_Wire> getProfileWiresOld() const;

    TopoShape getProfileShape() const;
    
    /// Returns the face of the sketch support (if any)
    const TopoDS_Face getSupportFace() const;
    
    Base::Vector3d getProfileNormal() const;

    /// retrieves the number of axes in the linked sketch (defined as construction lines)
    int getSketchAxisCount(void) const;    

    virtual Part::Feature* getBaseObject(bool silent=false) const;
    
    //backwards compatibility: profile property was renamed and has different type now
    virtual void handleChangedPropertyName(
        Base::XMLReader &reader, const char * TypeName, const char *PropName) override;
    
protected:
    void remapSupportShape(const TopoDS_Shape&);
    TopoShape refineShapeIfActive(const TopoShape&) const;

    /// Extract a face from a given LinkSub
    static void getUpToFaceFromLinkSub(TopoDS_Face& upToFace,
                                       const App::PropertyLinkSub& refFace);

    /// Find a valid face to extrude up to
    static void getUpToFace(TopoDS_Face& upToFace,
                            const TopoDS_Shape& support,
                            const TopoDS_Face& supportface,
                            const TopoDS_Shape& sketchshape,
                            const std::string& method,
                            const gp_Dir& dir,
                            const double offset);
    /**
      * Generate a linear prism
      * It will be a stand-alone solid created with BRepPrimAPI_MakePrism
      */
    static void generatePrism(TopoShape& prism,
                              TopoShape sketchshape,
                              const std::string& method,
                              const gp_Dir& direction,
                              const double L,
                              const double L2,
                              const bool midplane,
                              const bool reversed);

    /// Check whether the wire after projection on the face is inside the face
    static bool checkWireInsideFace(const TopoDS_Wire& wire,
                                    const TopoDS_Face& face,
                                    const gp_Dir& dir);

    /// Check whether the line crosses the face (line and face must be on the same plane)
    static bool checkLineCrossesFace(const gp_Lin& line, const TopoDS_Face& face);


    /// Used to suggest a value for Reversed flag so that material is always removed (Groove) or added (Revolution) from the support
    double getReversedAngle(const Base::Vector3d& b, const Base::Vector3d& v);
    /// get Axis from ReferenceAxis
    void getAxis(const App::DocumentObject* pcReferenceAxis, const std::vector<std::string>& subReferenceAxis,
                 Base::Vector3d& base, Base::Vector3d& dir);
        
    void onChanged(const App::Property* prop);
private:
    bool isParallelPlane(const TopoDS_Shape&, const TopoDS_Shape&) const;
    bool isEqualGeometry(const TopoDS_Shape&, const TopoDS_Shape&) const;
    bool isQuasiEqual(const TopoDS_Shape&, const TopoDS_Shape&) const;
};

} //namespace PartDesign


#endif // PARTDESIGN_SketchBased_H
