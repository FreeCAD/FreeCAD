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

#ifndef PART_FACEMAKER_H
#define PART_FACEMAKER_H

#include <BRepBuilderAPI_MakeShape.hxx>
#include <Standard_Version.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>

#include <memory>
#include <Base/BaseClass.h>
#include <Mod/Part/PartGlobal.h>


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
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    FaceMaker() = default;
    ~FaceMaker() override = default;

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
     * @brief Face: returns the face (result). If result is not a single face,
     * throws Base::TypeError. (hint: use .Shape() instead)
     * @return
     */
    virtual const TopoDS_Face& Face();

#if OCC_VERSION_HEX >= 0x070600
    void Build(const Message_ProgressRange& theRange = Message_ProgressRange()) override;
#else
    void Build() override;
#endif

    //fails to compile, huh!
    //virtual const TopTools_ListOfShape& Generated(const TopoDS_Shape &S) override {throwNotImplemented();}
    //virtual const TopTools_ListOfShape& Modified(const TopoDS_Shape &S) override {throwNotImplemented();}
    //virtual Standard_Boolean IsDeleted(const TopoDS_Shape &S) override {throwNotImplemented();}

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
     * Add new faces (or whatever) to myShapesToReturn. The rest is done by
     * base class's Build(). Please ignore contents of myCompounds in
     * implementation. If special handling of nesting is required, override
     * whole Build().
     */
    virtual void Build_Essence() = 0;

    static void throwNotImplemented();
};

/**
 * @brief The FaceMakerPublic class: derive from it if you want the face maker to be listed in tools that allow choosing one.
 */
class PartExport FaceMakerPublic : public FaceMaker
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    virtual std::string getUserFriendlyName() const = 0;
    virtual std::string getBriefExplanation() const = 0;
};



/**
 * @brief The FaceMakerSimple class: make plane faces from all closed wires
 * supplied, ignoring overlaps.
 *
 * Strengths: can work with non-coplanar sets of wires. Will not make broken
 * faces if wires overlap*.
 *
 * Limitations: can't make faces with holes (will generate overlapping faces
 * instead). Can't make faces from nonplanar wires.
 *
 * * Compound of valid but overlapping faces is created. The compound is invalid
 * for BOPs, but the faces themselves are valid, provided that the source wires
 * are valid.
 */
class PartExport FaceMakerSimple : public FaceMakerPublic
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    std::string getUserFriendlyName() const override;
    std::string getBriefExplanation() const override;
protected:
    void Build_Essence() override;
};


}//namespace Part
#endif // PART_FACEMAKER_H
