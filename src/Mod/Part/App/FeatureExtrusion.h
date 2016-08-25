/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef PART_FEATUREEXTRUSION_H
#define PART_FEATUREEXTRUSION_H

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include "PartFeature.h"
#include <TopoDS_Face.hxx>

namespace Part
{

class PartExport Extrusion : public Part::Feature
{
    PROPERTY_HEADER(Part::Extrusion);

public:
    Extrusion();

    App::PropertyLink Base;
    App::PropertyVector Dir;
    App::PropertyEnumeration DirMode;
    App::PropertyLinkSub DirLink;
    App::PropertyDistance LengthFwd;
    App::PropertyDistance LengthRev;
    App::PropertyBool Solid;
    App::PropertyBool Reversed;
    App::PropertyBool Symmetric;
    App::PropertyAngle TaperAngle;
    App::PropertyAngle TaperAngleRev;


    /**
     * @brief The ExtrusionParameters struct is supposed to be filled with final
     * extrusion parameters, after resolving links, applying mode logic,
     * reversing, etc., and be passed to extrudeShape.
     */
    struct ExtrusionParameters {
        gp_Dir dir;
        double lengthFwd;
        double lengthRev;
        bool solid;
        double taperAngleFwd; //in radians
        double taperAngleRev;
        ExtrusionParameters(): lengthFwd(0), lengthRev(0), solid(false), taperAngleFwd(0), taperAngleRev(0) {}// constructor to keep garbage out
    };

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    /// returns the type name of the view provider
    const char* getViewProviderName(void) const {
        return "PartGui::ViewProviderExtrusion";
    }
    //@}

    /**
     * @brief extrudeShape powers the extrusion feature.
     * @param source: the shape to be extruded
     * @param params: extrusion parameters
     * @return result of extrusion
     */
    static TopoShape extrudeShape(TopoShape source, ExtrusionParameters params);

    /**
     * @brief fetchAxisLink: read AxisLink to obtain the direction and
     * length. Note: this routine is re-used in Extrude dialog, hence it
     * is static.
     * @param axisLink (input): the link
     * @param basepoint (output): starting point of edge. Not used by extrude as of now.
     * @param dir (output): direction of axis, with magnitude (length)
     * @return true if link was fetched. false if link was empty. Throws if the
     * link is wrong.
     */
    static bool fetchAxisLink(const App::PropertyLinkSub& axisLink,
                              Base::Vector3d &basepoint,
                              Base::Vector3d &dir);

    /**
     * @brief computeFinalParameters: applies mode logic and fetches links, to
     * compute the actual parameters of extrusion. Dir property is updated in
     * the process, hence the function is non-const.
     */
    ExtrusionParameters computeFinalParameters();

    static Base::Vector3d calculateShapeNormal(const App::PropertyLink &shapeLink);

public: //mode enumerations
    enum eDirMode{
        dmCustom,
        dmEdge,
        dmNormal
    };
    static const char* eDirModeStrings[];

private:
    static bool isInside(const TopoDS_Wire&, const TopoDS_Wire&);
    static TopoDS_Face validateFace(const TopoDS_Face&);
    static TopoDS_Shape makeFace(const std::vector<TopoDS_Wire>&);
    static TopoDS_Shape makeFace(std::list<TopoDS_Wire>&); // for internal use only
    static void makeDraft(ExtrusionParameters params, const TopoDS_Shape&, std::list<TopoDS_Shape>&);

private:
    class Wire_Compare;
};

} //namespace Part


#endif // PART_FEATUREEXTRUSION_H
