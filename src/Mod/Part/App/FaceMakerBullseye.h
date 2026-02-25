// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2016 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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

#pragma once

#include "FaceMaker.h"

#include <Geom_Surface.hxx>
#include <gp_Pln.hxx>
#include <Bnd_Box.hxx>

#include <Mod/Part/PartGlobal.h>

namespace Part
{

class WireJoiner;

/**
 * @brief The FaceMakerBullseye class is a tool to make planar faces with holes,
 * where there can be additional faces inside holes and they can have holes too
 * and so on.
 *
 * Strengths: makes faces with holes with islands
 *
 * Weaknesses: faces of one compound must be on same plane. TBD
 */
class PartExport FaceMakerBullseye: public FaceMakerPublic
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    FaceMakerBullseye()
        : planeSupplied(false)
    {}
    /**
     * @brief setPlane: sets the plane to use when making faces. This is
     * optional. If the plane was set, it is not tested that the wires are
     * planar or on the supplied plane, potentially speeding things up.
     * @param plane FIXME: the plane is not propagated if processing compounds.
     */
    virtual void setPlane(const gp_Pln& plane) override;

    std::string getUserFriendlyName() const override;
    std::string getBriefExplanation() const override;

protected:
    void Build_Essence() override;

protected:
    gp_Pln myPlane;  // externally supplied plane (if any)
    bool planeSupplied {false};
    bool reuseInnerWire {false};

    struct WireInfo
    {
        TopoShape wire;
        Bnd_Box bound;
        double extent;
        WireInfo(const TopoShape& s, const Bnd_Box& b)
            : wire(s)
            , bound(b)
        {
            extent = bound.SquareExtent();
        }
        bool operator<(const WireInfo& other) const;
    };

    /**
     * @brief The FaceDriller class is similar to BRepBuilderAPI_MakeFace,
     * except that it is tolerant to wire orientation (wires are oriented as
     * needed automatically).
     */
    class FaceDriller
    {
    public:
        FaceDriller(const gp_Pln& plane, TopoDS_Wire outerWire);

        /// Hit test result
        enum class HitTest
        {
            /// Not hitting
            HitNone = 0,
            /// Hit inside face
            Hit = 1,
            /// Hit inside the out wire bound
            HitOuter = 2,
        };

        /**
         * @brief hitTest: returns True if a shape is on the face
         * @param shape
         */
        HitTest hitTest(const TopoShape& shape) const;

        void addHole(TopoDS_Wire w);
        void addHole(const WireInfo& info, std::vector<TopoShape>& sources);
        void copyFaceBound(TopoDS_Face& f, TopoShape& tf, const TopoShape& source);

        const TopoDS_Face& Face() const
        {
            return myFace;
        }

    public:
        /**
         * @brief wireDirection: determines direction of wire with respect to
         * myPlane.
         * @param w
         * @return  1 = CCW (suits as outer wire), -1 = CW (suits as hole)
         */
        static int getWireDirection(const gp_Pln& plane, const TopoDS_Wire& w);

    private:
        gp_Pln myPlane;
        TopoDS_Face myFace;
        TopoDS_Face myFaceBound;
        TopoShape myTopoFace;
        TopoShape myTopoFaceBound;
        std::vector<WireInfo> myHoles;
        Handle(Geom_Surface) myHPlane;
        std::unique_ptr<WireJoiner> myJoiner;
    };
};

/**
 * The FaceMakerRing is a tool to make faces with holes.
 *
 * The tool assumes the wires are closed and do not intersect with each other.
 * Each wire will be used to as an outer wire to make a face, with inner wire as
 * holes. Only the first level inner wire will be used for holes. Nested inner
 * wires are ignored. Note that each inner wire will also be used as outer wire
 * for making face, with the second level inner wires as holes, so on and so
 * forth.
 */
class PartExport FaceMakerRing: public FaceMakerBullseye
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    FaceMakerRing();

    virtual std::string getUserFriendlyName() const override;
    virtual std::string getBriefExplanation() const override;
};

}  // namespace Part
