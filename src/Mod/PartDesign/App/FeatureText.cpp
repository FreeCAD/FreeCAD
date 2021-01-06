/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
 *                 2020 David Österberg                                    *
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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <BRep_Builder.hxx>
# include <BRepBndLib.hxx>
# include <BRepPrimAPI_MakeRevol.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Wire.hxx>
# include <TopExp_Explorer.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepAlgoAPI_Common.hxx>
# include <Precision.hxx>
# include <gp_Lin.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <Law_Function.hxx>
# include <BRepOffsetAPI_MakePipeShell.hxx>
# include <BRepBuilderAPI_MakeSolid.hxx>
# include <BRepBuilderAPI_Sewing.hxx>
# include <BRepClass3d_SolidClassifier.hxx>
# include <ShapeAnalysis.hxx>
# include <gp_Ax1.hxx>
# include <gp_Ax3.hxx>

#endif

# include <string>
# include <Standard_Version.hxx>
# include <Base/Axis.h>
# include <Base/Console.h>
# include <Base/Exception.h>
# include <Base/Placement.h>
# include <Base/Tools.h>
#include "boost/filesystem.hpp"

# include <Mod/Part/App/TopoShape.h>
# include <Mod/Part/App/TopoShapePy.h>
# include <Mod/Part/App/FaceMakerCheese.h>

# include "FeatureText.h"


using namespace PartDesign;


PROPERTY_SOURCE(PartDesign::Text, PartDesign::ProfileBased)

Text::Text()
{

    ADD_PROPERTY_TYPE(ReferenceAxis,(0),"Helix", App::Prop_None, "Reference axis of revolution");
    ADD_PROPERTY_TYPE(Base,(Base::Vector3d(0.0,0.0,0.0)),"Text", App::Prop_ReadOnly, "Base");
    ADD_PROPERTY_TYPE(Axis,(Base::Vector3d(0.0,1.0,0.0)),"Text", App::Prop_ReadOnly, "Axis");
    ADD_PROPERTY_TYPE(TextString,("FreeCAD rocks!"),"Text", App::Prop_None, "Text");
    ADD_PROPERTY_TYPE(Font,("/usr/share/fonts/truetype/ttf-bitstream-vera/VeraMono.ttf"),"Text", App::Prop_None, "Font");
    ADD_PROPERTY_TYPE(Size,(20.0),"Text", App::Prop_None, "Size");
    ADD_PROPERTY_TYPE(Height,(30.0),"Text", App::Prop_None, "Height");
    ADD_PROPERTY_TYPE(Offset, (0.0), "Text", App::Prop_None, "Offset from face in which text will end");
    static const App::PropertyQuantityConstraint::Constraints signedLengthConstraint = {-DBL_MAX, DBL_MAX, 1.0};
    Offset.setConstraints(&signedLengthConstraint);
}



short Text::mustExecute() const
{
    if (Placement.isTouched() ||
        ReferenceAxis.isTouched() ||
        Axis.isTouched() ||
        Base.isTouched() ||
        Height.isTouched())
        return 1;
    return ProfileBased::mustExecute();
}

App::DocumentObjectExecReturn *Text::execute(void)
{

    std::string text = std::string(TextString.getValue());
    std::string fontpath = std::string(Font.getValue());
    double size = Size.getValue();

    Base::Console().Warning("Text: %s\n", text.c_str());
    Base::Console().Warning("Font: %s\n", fontpath.c_str());
    Base::Console().Warning("Size: %f\n", size);

    // update Axis from ReferenceAxis
    try {
        updateAxis();
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // get revolve axis
    Base::Vector3d b = Base.getValue();
    gp_Pnt pnt(b.x,b.y,b.z);
    Base::Vector3d v = Axis.getValue();
    gp_Dir dir(v.x,v.y,v.z);


    // Validate parameters
    double L = Height.getValue();
    double L2 = 0.0;
    bool imprint = false;
    if (L > Precision::Confusion())
        L2 = 0.0;
    else if (L < - Precision::Confusion()) {
        L2 = L;
        L = 0.0;
    } else {
        imprint = true;
        L = 0.0;
        L2 = 0.0;
    }


    Part::Feature* obj = 0;
    TopoDS_Shape sketchshape, textshape;


    try {
        obj = getVerifiedObject();
        sketchshape = getVerifiedFace();
        // (const std::string& text, const std::string& fontpath, double height, double track)
        textshape = textToShape(text, fontpath, size, 0.0);

    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // if the Base property has a valid shape, fuse the prism into it
    TopoDS_Shape base;
    try {
        base = getBaseShape();
    } catch (const Base::Exception&) {
        base = TopoDS_Shape();
    }

    // get the Sketch plane
    Base::Placement SketchPos = obj->Placement.getValue();
    // get the normal vector of the sketch
    Base::Vector3d SketchVector = getProfileNormal();

    Base::Vector3d paddingDirection = SketchVector;

    try {
        this->positionByPrevious();
        TopLoc_Location invObjLoc = this->getLocation().Inverted();

        base.Move(invObjLoc);


        //Py::Object charList = makeWireString(const Py::Tuple& args);

        // create vector in padding direction with length 1
        gp_Dir dir(paddingDirection.x, paddingDirection.y, paddingDirection.z);

        // The length of a gp_Dir is 1 so the resulting pad would have
        // the length L in the direction of dir. But we want to have its height in the
        // direction of the normal vector.
        // Therefore we must multiply L by the factor that is necessary
        // to make dir as long that its projection to the SketchVector
        // equals the SketchVector.
        // This is the scalar product of both vectors.
        // Since the pad length cannot be negative, the factor must not be negative.

        double factor = fabs(dir * gp_Dir(SketchVector.x, SketchVector.y, SketchVector.z));

        // factor would be zero if vectors are orthogonal
        if (factor < Precision::Confusion())
            return new App::DocumentObjectExecReturn("Pad: Creation failed because direction is orthogonal to sketch's normal vector");

        // perform the length correction
        L = L / factor;
        L2 = L2 / factor;

        dir.Transform(invObjLoc.Transformation());

        if (textshape.IsNull())
            return new App::DocumentObjectExecReturn("Pad: Creating a face from sketch failed");
        textshape.Move(invObjLoc);

        TopoDS_Shape prism;
        std::string method("Length");

        generatePrism(prism, textshape, method, dir, L, L2,
                          Midplane.getValue(), Reversed.getValue());

        if (prism.IsNull())
            return new App::DocumentObjectExecReturn("Pad: Resulting shape is empty");

        // set the additive shape property for later usage in e.g. pattern
        prism = refineShapeIfActive(prism);
        this->AddSubShape.setValue(prism);

        if (!base.IsNull()) {
//             auto obj = getDocument()->addObject("Part::Feature", "prism");
//             static_cast<Part::Feature*>(obj)->Shape.setValue(getSolid(prism));
            // Let's call algorithm computing a fuse operation:
            BRepAlgoAPI_Fuse mkFuse(base, prism);
            // Let's check if the fusion has been successful
            if (!mkFuse.IsDone())
                return new App::DocumentObjectExecReturn("Pad: Fusion with base feature failed");
            TopoDS_Shape result = mkFuse.Shape();

            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape solRes = this->getSolid(result);
            // lets check if the result is a solid
            if (solRes.IsNull())
                return new App::DocumentObjectExecReturn("Pad: Resulting shape is not a solid");

            int solidCount = countSolids(result);
            if (solidCount > 1) {
                return new App::DocumentObjectExecReturn("Pad: Result has multiple solids. This is not supported at this time.");
            }

            solRes = refineShapeIfActive(solRes);
            this->Shape.setValue(getSolid(solRes));
        } else {
            int solidCount = countSolids(prism);
            if (solidCount > 1) {
                return new App::DocumentObjectExecReturn("Pad: Result has multiple solids. This is not supported at this time.");
            }

           this->Shape.setValue(getSolid(prism));
        }

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {

        if (std::string(e.GetMessageString()) == "TopoDS::Face")
            return new App::DocumentObjectExecReturn("Could not create face from sketch.\n"
                "Intersecting sketch entities or multiple faces in a sketch are not allowed.");
        else
            return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
}



void Text::updateAxis(void)
{
    App::DocumentObject *pcReferenceAxis = ReferenceAxis.getValue();
    const std::vector<std::string> &subReferenceAxis = ReferenceAxis.getSubValues();
    Base::Vector3d base;
    Base::Vector3d dir;
    getAxis(pcReferenceAxis, subReferenceAxis, base, dir);

    Base.setValue(base.x,base.y,base.z);
    Axis.setValue(dir.x,dir.y,dir.z);
}


TopoDS_Shape Text::textToShape(const std::string& text, const std::string& fontpath, double height, double track = 0.0)
{
    Base::Console().Warning("textToShape was called\n");

    Py::Module mod(PyImport_ImportModule("Part"), true);
    if (mod.isNull())
        throw Py::Exception();

    Base::Console().Warning("PyImport_ImportModule was succsessfull\n");

    Py::Callable method(mod.getAttr(std::string("makeWireString")));

    Py::Tuple args(4);

    args.setItem(0, Py::String(text));
    args.setItem(1, Py::String(fontpath));
    args.setItem(2, Py::Float(height));
    args.setItem(3, Py::Float(track));

    Py::List charList(method.apply(args));

    TopoDS_Compound comp;
    BRep_Builder builder;
    builder.MakeCompound(comp);


    for (Py::List::iterator it = charList.begin(); it != charList.end(); ++it) {
        Py::List glyph(*it);
        for (Py::List::iterator jt = glyph.begin(); jt != glyph.end(); ++jt) {
            if (PyObject_TypeCheck((*jt).ptr(), &Part::TopoShapePy::Type)) {
                TopoShape* ptr = static_cast<Part::TopoShapePy*>((*jt).ptr())->getTopoShapePtr();
                if (!ptr->getShape().IsNull()) {
                    builder.Add(comp, ptr->getShape());
                }
            }
        }
    }


    // make faces from the wires (simplified from getVerifiedFace)
    const char* err = nullptr;
    TopoShape shape;
    try {
        shape = TopoShape(comp);
        if(shape.isNull())
            err = "Linked shape object is empty";
        else {
            auto faces = shape.getSubTopoShapes(TopAbs_FACE);
            if(faces.empty()) {
                if(!shape.hasSubShape(TopAbs_WIRE))
                    shape = shape.makEWires();
                if(shape.hasSubShape(TopAbs_WIRE))
                    shape = shape.makEFace(0,"Part::FaceMakerCheese");
                else
                    err = "Cannot make face from profile";
            } else if (faces.size() == 1)
                shape = faces.front();
            else
                shape = TopoShape().makECompound(faces);
        }
    } catch (Standard_Failure &e) {
        throw Base::RuntimeError (err);;
    }
    Base::Console().Warning("textToShape if about to return\n");
    return shape.getShape();

}
