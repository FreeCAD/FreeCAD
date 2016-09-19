/***************************************************************************
 *   Copyright (c) 2016 Victor Titov (DeepSOIC)      <vv.titov@gmail.com>  *
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

#ifndef PART_FACEMAKER_H
#define PART_FACEMAKER_H

#include <BRepBuilderAPI_MakeShape.hxx>
#include <Base/BaseClass.h>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Face.hxx>

#include <memory>

namespace Part
{

/**
 * @brief FaceMaker class is the base class for implementing various "smart"
 * face making routines. This was created to address the problem of multiple
 * private implementations of making faces with holes, which are quite complex.
 * The two most important facemaking routines then was: one in Part Extrude,
 * and one in PartDesign (there, it is used in every sketch-based feature).
 * Plus, another one (new) was needed for filling 2D offset.
 */
class PartExport FaceMaker: public BRepBuilderAPI_MakeShape, public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    FaceMaker(const TopoDS_Edge &e);
    FaceMaker(const TopoDS_Wire &w);

    /**
     * @brief FaceMaker: take compound as list of shapes to make face from (like useCompound).
     * @param comp
     */
    FaceMaker(const TopoDS_Compound &comp);

    FaceMaker() {};
    virtual ~FaceMaker() {};

    virtual void addWire(const TopoDS_Wire& w);
    /**
     * @brief addShape: add another wire, edge, or compound. If compound is
     * added, its internals will be treated as isolated from the rest, and the
     * compounding structure of result will follow.
     * @param sh
     */
    virtual void addShape(const TopoDS_Shape& sh);
    /**
     * @brief useCompound: add children of compound to the FaceMaker. Note that
     * this is different from addShape(comp) - structure is lost. The compound
     * is NOT expanded recursively.
     * @param comp
     */
    virtual void useCompound(const TopoDS_Compound &comp);

    /**
     * @brief Face: returns the face (result). If result is not a single face, throws Base::TypeError.
     * @return
     */
    virtual const TopoDS_Face& Face();

    virtual void Build();

    static std::unique_ptr<FaceMaker> ConstructFromType(const char* className);
    static std::unique_ptr<FaceMaker> ConstructFromType(Base::Type type);

protected:
    std::vector<TopoDS_Shape> mySourceShapes; //wire or compound
    std::vector<TopoDS_Wire> myWires; //wires from mySourceShapes
    std::vector<TopoDS_Compound> myCompounds; //compounds, for recursive processing
    std::vector<TopoDS_Shape> myShapesToReturn;

    /**
     * @brief Build_Essence: build routine that can assume there is no nesting.
     *
     * Implementing instructions:
     * Add new faces (or whatever) to myShapesToReturn. The rest is done by base class.
     * Please ignore contents of myCompounds in implementation. If special handling of nesting is required, override whole Build().
     */
    virtual void Build_Essence() = 0;
};

/**
 * @brief The FaceMakerPublic class: derive from it if you want the face maker to be listed in tools that allow choosing one.
 */
class PartExport FaceMakerPublic : public FaceMaker
{
    TYPESYSTEM_HEADER();
public:
    virtual std::string getUserFriendlyName() const = 0;
    virtual std::string getBriefExplanation() const = 0;
};



/**
 * @brief The FaceMakerSimple class: make faces from all closed wires supplied, ignoring self-intersections.
 * Strengths: can work with non-coplanar sets of wires.
 * Limitations: can't make faces with holes (will generate overlapping faces instead). Can't make faces from nonplanar wires.
 */
class PartExport FaceMakerSimple : public FaceMakerPublic
{
    TYPESYSTEM_HEADER();
public:
    virtual std::string getUserFriendlyName() const override;
    virtual std::string getBriefExplanation() const override;
protected:
    virtual void Build_Essence() override;
};


}//namespace Part
#endif // PART_FACEMAKER_H
