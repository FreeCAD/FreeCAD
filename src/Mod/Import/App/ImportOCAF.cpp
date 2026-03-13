// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#if defined(__MINGW32__)
# define WNT  // avoid conflict with GUID
#endif
#include <BRepBndLib.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <Quantity_ColorRGBA.hxx>
#include <Standard_Failure.hxx>
#include <Standard_Version.hxx>
#include <TDF_ChildIterator.hxx>
#include <TDF_Label.hxx>
#include <TDF_LabelSequence.hxx>
#include <TDataStd_Name.hxx>
#include <TDocStd_Document.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS_Iterator.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_Location.hxx>
#include <gp_Pln.hxx>  // for Precision::Confusion()
#include <gp_Trsf.hxx>


#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Mod/Part/App/FeatureCompound.h>
#include <Mod/Part/App/ShapeMapHasher.h>

#include "ImportOCAF.h"
#include "Tools.h"

#include <unordered_map>

#ifdef HAVE_TBB
# include <tbb/blocked_range.h>
# include <tbb/parallel_for.h>
# include <tbb/task_group.h>
#endif

using namespace Import;

#define OCAF_KEEP_PLACEMENT

ImportOCAF::ImportOCAF(Handle(TDocStd_Document) h, App::Document* d, const std::string& name)
    : pDoc(h)
    , doc(d)
    , default_name(name)
{
    aShapeTool = XCAFDoc_DocumentTool::ShapeTool(pDoc->Main());
    aColorTool = XCAFDoc_DocumentTool::ColorTool(pDoc->Main());
}

ImportOCAF::~ImportOCAF() = default;

void ImportOCAF::tryPlacementFromLoc(App::GeoFeature* part, const TopLoc_Location& part_loc)
{
    gp_Trsf trf;
    Base::Matrix4D mtrx;
    if (part_loc.IsIdentity()) {
        trf = part_loc.Transformation();
    }
    else {
        trf = TopLoc_Location(part_loc.FirstDatum()).Transformation();
    }

    Part::TopoShape::convertToMatrix(trf, mtrx);

    try {
        Base::Placement pl;
        pl.fromMatrix(mtrx);
        part->Placement.setValue(pl);
    }
    catch (const Base::ValueError& e) {
        e.reportException();
    }
}

void ImportOCAF::loadShapes()
{
    myRefShapes.clear();
    // Build the mapping from shapes to Labels, used to obtain the colours of the elements of the
    // model. This is necessary because XCAFDoc_ColorTool::GetLabel(TopoDS_Shape) is dismally slow,
    // taking time proportional to the number of Labels in the whole document.
    TDF_ChildIterator labelIterator(pDoc->Main(), true);
    TopoDS_Shape sh;
    while (labelIterator.More()) {
        TDF_Label l(labelIterator.Value());
        if (aShapeTool->GetShape(l, sh)) {
            shapeToLabelMap.emplace(sh, l);
        }
        labelIterator.Next();
    }
    loadShapes(pDoc->Main(), TopLoc_Location(), default_name, "", false);
}

void ImportOCAF::setMerge(bool merge)
{
    this->merge = merge;
}

App::DocumentObject* ImportOCAF::loadShapes(
    const TDF_Label& label,
    const TopLoc_Location& loc,
    const std::string& defaultname,
    const std::string& assembly,
    bool isRef
)
{
    int hash = 0;
#ifdef HAVE_TBB
    using namespace tbb;
    task_group g;
#endif
    TopoDS_Shape aShape;

    if (aShapeTool->GetShape(label, aShape)) {
        hash = Part::ShapeMapHasher {}(aShape);
    }

    Handle(TDataStd_Name) name;
    std::string part_name = defaultname;
    if (label.FindAttribute(TDataStd_Name::GetID(), name)) {
        TCollection_ExtendedString extstr = name->Get();
        char* str = new char[extstr.LengthOfCString() + 1];
        extstr.ToUTF8CString(str);
        part_name = str;
        delete[] str;
        if (part_name.empty()) {
            part_name = defaultname;
        }
        else {
            bool ws = true;
            for (char it : part_name) {
                if (it != ' ') {
                    ws = false;
                    break;
                }
            }
            if (ws) {
                part_name = defaultname;
            }
        }
    }

    TopLoc_Location part_loc = loc;
    Handle(XCAFDoc_Location) hLoc;
    if (label.FindAttribute(XCAFDoc_Location::GetID(), hLoc)) {
        if (isRef) {
            part_loc = part_loc * hLoc->Get();
        }
        else {
            part_loc = hLoc->Get();
        }
    }

#ifdef FC_DEBUG
    Base::Console().log(
        "H:%d, N:%s, T:%d, A:%d, S:%d, C:%d, SS:%d, F:%d, R:%d, C:%d, SS:%d\n",
        hash,
        part_name.c_str(),
        aShapeTool->IsTopLevel(label),
        aShapeTool->IsAssembly(label),
        aShapeTool->IsShape(label),
        aShapeTool->IsCompound(label),
        aShapeTool->IsSimpleShape(label),
        aShapeTool->IsFree(label),
        aShapeTool->IsReference(label),
        aShapeTool->IsComponent(label),
        aShapeTool->IsSubShape(label)
    );
#endif

#if defined(OCAF_KEEP_PLACEMENT)
    std::string asm_name = part_name;
    (void)assembly;
#else
    std::string asm_name = assembly;
    if (aShapeTool->IsAssembly(label)) {
        asm_name = part_name;
    }
#endif

    TDF_Label ref;
    if (aShapeTool->IsReference(label) && aShapeTool->GetReferredShape(label, ref)) {
        return loadShapes(ref, part_loc, part_name, asm_name, true);
    }

    if (isRef || myRefShapes.find(hash) == myRefShapes.end()) {
        TopoDS_Shape aShape;
        if (isRef && aShapeTool->GetShape(label, aShape)) {
            myRefShapes.insert(Part::ShapeMapHasher {}(aShape));
        }

        if (aShapeTool->IsSimpleShape(label)) {
            if (isRef || aShapeTool->IsFree(label)) {
                if (!asm_name.empty()) {
                    part_name = asm_name;
                }

                if (isRef) {
                    return createShape(label, loc, part_name);
                }
                else {
                    // Note: This does not return the created shape.
                    // This is carrying forward behaviour that may have been intentional, but looked
                    // more like a mistake in previous code: Older code, rather than returning the
                    // created DocumentObjects, the groups of methods accepted a vector which the
                    // part was pushed back onto. In this particular instance rather than passing
                    // the collection we received on to createShape, a local collection called
                    // 'localValue' was passed, and its contents subsequently ignored. The effect of
                    // this would be that if an Assembly directly (not through a ref) contains a
                    // Simple Shape and that shape's label is a Free Label, that shape would be at
                    // the top level of the FC model rather than a child of the Part corresponding
                    // to the Assembly. I'm not sure this condition can actually exist.
                    (void)createShape(label, part_loc, part_name);
                }
            }
            // A simple shape that does not have a Free Label and is not reached through a ref.

            // We are not creating a list of Part::Feature in that case but just
            // a single Part::Feature which has as a Shape a Compound of the Subshapes contained
            // within the global shape
            // This is standard behavior of many STEP reader and avoid to register a crazy
            // amount of Shape within the Tree as STEP file do mostly contain large assemblies
        }
        else {
            // This is probably an Assembly. Either way we create its contents, and if it turns out
            // to be an Assembly, we will create a Part and place all the contents in that Part.
            // Otherwise they will be left as top-level document objects.
            std::vector<App::DocumentObject*> localValue;

            for (TDF_ChildIterator it(label); it.More(); it.Next()) {
                App::DocumentObject* createdDocObj
                    = loadShapes(it.Value(), part_loc, part_name, asm_name, false);
                if (createdDocObj != nullptr) {
                    localValue.push_back(createdDocObj);
                }
            }

            if (!localValue.empty() && aShapeTool->IsAssembly(label)) {
                App::Part* pcPart = doc->addObject<App::Part>(asm_name.c_str());
                pcPart->Label.setValue(asm_name);
                pcPart->addObjects(localValue);

                // STEP reader is now a hierarchical reader. Node and leaf must have
                // there local placement updated and relative to the STEP file content
                // standard FreeCAD placement was absolute we are now moving to relative

                tryPlacementFromLoc(pcPart, part_loc);
                return pcPart;
            }
        }
    }
    return nullptr;
}

App::DocumentObject* ImportOCAF::createShape(
    const TDF_Label& label,
    const TopLoc_Location& loc,
    const std::string& name
)
{
    const TopoDS_Shape& aShape = aShapeTool->GetShape(label);
#ifdef HAVE_TBB
    using namespace tbb;
    task_group g;
#endif

    if (aShape.IsNull()) {
        return nullptr;
    }
    if (aShape.ShapeType() == TopAbs_COMPOUND) {
        TopExp_Explorer xp;

        if (this->merge) {
            // Combine all the SOLID, SHELL, EDGE, and VERTEX subshapes into a new compound as a
            // single Part::Feature.
            // It may seem strange to pick apart a compound only to make a new compound but perhaps
            // aShape contains topology other than the solids, hollow shells, edges, and vertices
            // that we transfer to the new compound. (perhaps a Solid from the STEP file also has a
            // Shell that we don't want???)
            BRep_Builder builder;
            TopoDS_Compound comp;
            builder.MakeCompound(comp);

            for (xp.Init(aShape, TopAbs_SOLID); xp.More(); xp.Next()) {
                const TopoDS_Shape& sh = xp.Current();
                if (!sh.IsNull()) {
                    builder.Add(comp, sh);
                }
            }

            for (xp.Init(aShape, TopAbs_SHELL, TopAbs_SOLID); xp.More(); xp.Next()) {
                const TopoDS_Shape& sh = xp.Current();
                if (!sh.IsNull()) {
                    builder.Add(comp, sh);
                }
            }

            for (xp.Init(aShape, TopAbs_EDGE); xp.More(); xp.Next()) {
                const TopoDS_Shape& sh = xp.Current();
                if (!sh.IsNull()) {
                    builder.Add(comp, sh);
                }
            }

            for (xp.Init(aShape, TopAbs_VERTEX); xp.More(); xp.Next()) {
                const TopoDS_Shape& sh = xp.Current();
                if (!sh.IsNull()) {
                    builder.Add(comp, sh);
                }
            }

            // Create the single compound shape, setting its Placement from loc.
            // We pass the Label of the original compound so if it has any colour applied that will
            // be used.
            return comp.IsNull() ? nullptr : createShape(comp, label, loc, true, name);
        }
        else {
            // Create a new Part::Feature for each subshape, and nest them all into an App::Part
            std::vector<App::DocumentObject*> partComponents;
            // We pass empty labels to createShape, making it find the shape's Label itself.
            for (xp.Init(aShape, TopAbs_SOLID); xp.More(); xp.Next()) {
                partComponents.push_back(createShape(xp.Current(), TDF_Label(), loc, false, name));
            }
            for (xp.Init(aShape, TopAbs_SHELL, TopAbs_SOLID); xp.More(); xp.Next()) {
                partComponents.push_back(createShape(xp.Current(), TDF_Label(), loc, false, name));
            }
            if (partComponents.empty()) {
                return nullptr;
            }
            App::Part* pcPart = doc->addObject<App::Part>(name.c_str());
            pcPart->Label.setValue(name);
            pcPart->addObjects(partComponents);

            return pcPart;
        }
    }
    else {
        // aShape is not a Compound Shape, make a single Part::Feature for it
        // By passing the shape's Label, ceateShape does not have to look it up.
        return createShape(aShape, label, loc, false, name);
    }
}

bool ImportOCAF::getShapeColour(const TopoDS_Shape& shape, TDF_Label labelHint, Base::Color& foundColour)
{
    TDF_Label shapeLabel = labelHint;
    if (shapeLabel.IsNull() && shapeToLabelMap.contains(shape)) {
        shapeLabel = shapeToLabelMap[shape];
    }
    if (!shapeLabel.IsNull()) {
        Quantity_ColorRGBA aColor;
        if (aColorTool->GetColor(shapeLabel, XCAFDoc_ColorGen, aColor)
            || aColorTool->GetColor(shapeLabel, XCAFDoc_ColorSurf, aColor)
            || aColorTool->GetColor(shapeLabel, XCAFDoc_ColorCurv, aColor)) {
            foundColour = Tools::convertColor(aColor);
            return true;
        }
    }
    return false;
}

App::DocumentObject* ImportOCAF::createShape(
    const TopoDS_Shape& aShape,
    const TDF_Label& labelHint,
    const TopLoc_Location& loc,
    bool setPlacementFromLocation,
    const std::string& name
)
{
    Part::Feature* part = doc->addObject<Part::Feature>();

    if (setPlacementFromLocation) {
        tryPlacementFromLoc(part, loc);
    }
    if (!loc.IsIdentity()) {
        part->Shape.setValue(aShape.Moved(loc));
    }
    else {
        part->Shape.setValue(aShape);
    }

    part->Label.setValue(name);

    // Find and apply colours to the created part.
    // There may be a colour for the overall Part, and/or some of the topology that makes up the
    // shape may have their own colours.
    std::vector<Base::Color> faceColors;
    Quantity_ColorRGBA aColor;
    Base::Color color(0.8f, 0.8f, 0.8f);
    if (getShapeColour(aShape, labelHint, color)) {
        faceColors.push_back(color);
    }

    // Obtain a vector of face colors parallel with the TopExp_Explorer iteration order
    // This code assumes consistent iteration order each time, and that this
    // order also matches the ordering expected in the color vector we are creating.
    size_t face_index = 0;
    TopExp_Explorer xp(aShape, TopAbs_FACE);
    for (; xp.More(); xp.Next(), ++face_index) {
        if (getShapeColour(xp.Current(), TDF_Label(), color)) {
            if (face_index >= faceColors.size()) {
                // We've just realized we need per-element colours. Make the colour vector large
                // enough for all the elements. First count the number of faces in aShape
                int n_faces = 0;
                TopExp_Explorer xp2(aShape, TopAbs_FACE);
                while (xp2.More()) {
                    ++n_faces;
                    xp2.Next();
                }
                faceColors.resize(n_faces, color);
            }
            faceColors[face_index] = color;
        }
    }

    if (!faceColors.empty()) {
        applyColors(part, faceColors);
    }

    return part;
}

// ----------------------------------------------------------------------------

ImportOCAFCmd::ImportOCAFCmd(Handle(TDocStd_Document) h, App::Document* d, const std::string& name)
    : ImportOCAF(h, d, name)
{}

void ImportOCAFCmd::applyColors(Part::Feature* part, const std::vector<Base::Color>& colors)
{
    partColors[part] = colors;
}

// ----------------------------------------------------------------------------

ImportXCAF::ImportXCAF(Handle(TDocStd_Document) h, App::Document* d, const std::string& name)
    : hdoc(h)
    , doc(d)
    , default_name(name)
{
    aShapeTool = XCAFDoc_DocumentTool::ShapeTool(hdoc->Main());
    hColors = XCAFDoc_DocumentTool::ColorTool(hdoc->Main());
}

ImportXCAF::~ImportXCAF() = default;

void ImportXCAF::loadShapes()
{
    // collect sequence of labels to display
    TDF_LabelSequence shapeLabels, colorLabels;
    aShapeTool->GetFreeShapes(shapeLabels);
    hColors->GetColors(colorLabels);

    // set presentations and show
    for (Standard_Integer i = 1; i <= shapeLabels.Length(); i++) {
        // get the shapes and attributes
        const TDF_Label& label = shapeLabels.Value(i);
        loadShapes(label);
    }
    std::map<Standard_Integer, TopoDS_Shape>::iterator it;
    // go through solids
    for (it = mySolids.begin(); it != mySolids.end(); ++it) {
        createShape(it->second, true, true);
    }
    // go through shells
    for (it = myShells.begin(); it != myShells.end(); ++it) {
        createShape(it->second, true, true);
    }
    // go through compounds
    for (it = myCompds.begin(); it != myCompds.end(); ++it) {
        createShape(it->second, true, true);
    }
    // do the rest
    if (!myShapes.empty()) {
        BRep_Builder builder;
        TopoDS_Compound comp;
        builder.MakeCompound(comp);
        for (it = myShapes.begin(); it != myShapes.end(); ++it) {
            builder.Add(comp, it->second);
        }
        createShape(comp, true, false);
    }
}

void ImportXCAF::createShape(const TopoDS_Shape& shape, bool perface, bool setname) const
{
    Part::Feature* part;
    part = doc->addObject<Part::Feature>(default_name.c_str());
    part->Label.setValue(default_name);
    part->Shape.setValue(shape);
    std::map<Standard_Integer, Quantity_ColorRGBA>::const_iterator jt;
    jt = myColorMap.find(Part::ShapeMapHasher {}(shape));

    Base::Color partColor(0.8f, 0.8f, 0.8f);


    // set label name if defined
    if (setname && !myNameMap.empty()) {
        std::map<Standard_Integer, std::string>::const_iterator jt;
        jt = myNameMap.find(Part::ShapeMapHasher {}(shape));
        if (jt != myNameMap.end()) {
            part->Label.setValue(jt->second);
        }
    }

    // check for colors per face
    if (perface && !myColorMap.empty()) {
        TopTools_IndexedMapOfShape faces;
        TopExp_Explorer xp(shape, TopAbs_FACE);
        while (xp.More()) {
            faces.Add(xp.Current());
            xp.Next();
        }

        std::vector<Base::Color> faceColors;
        faceColors.resize(faces.Extent(), partColor);
        xp.Init(shape, TopAbs_FACE);
        while (xp.More()) {
            jt = myColorMap.find(Part::ShapeMapHasher {}(xp.Current()));
            if (jt != myColorMap.end()) {
                int index = faces.FindIndex(xp.Current());
                faceColors[index - 1] = Tools::convertColor(jt->second);
            }
            xp.Next();
        }
    }
}

void ImportXCAF::loadShapes(const TDF_Label& label)
{
    TopoDS_Shape aShape;
    if (aShapeTool->GetShape(label, aShape)) {
        if (aShapeTool->IsTopLevel(label)) {
            int ctSolids = 0, ctShells = 0, ctComps = 0;
            // add the shapes
            TopExp_Explorer xp;
            for (xp.Init(aShape, TopAbs_SOLID); xp.More(); xp.Next(), ctSolids++) {
                this->mySolids[Part::ShapeMapHasher {}(xp.Current())] = (xp.Current());
            }
            for (xp.Init(aShape, TopAbs_SHELL, TopAbs_SOLID); xp.More(); xp.Next(), ctShells++) {
                this->myShells[Part::ShapeMapHasher {}(xp.Current())] = (xp.Current());
            }
            // if no solids and no shells were found then go for compounds
            if (ctSolids == 0 && ctShells == 0) {
                for (xp.Init(aShape, TopAbs_COMPOUND); xp.More(); xp.Next(), ctComps++) {
                    this->myCompds[Part::ShapeMapHasher {}(xp.Current())] = (xp.Current());
                }
            }
            if (ctComps == 0) {
                for (xp.Init(aShape, TopAbs_FACE, TopAbs_SHELL); xp.More(); xp.Next()) {
                    this->myShapes[Part::ShapeMapHasher {}(xp.Current())] = (xp.Current());
                }
                for (xp.Init(aShape, TopAbs_WIRE, TopAbs_FACE); xp.More(); xp.Next()) {
                    this->myShapes[Part::ShapeMapHasher {}(xp.Current())] = (xp.Current());
                }
                for (xp.Init(aShape, TopAbs_EDGE, TopAbs_WIRE); xp.More(); xp.Next()) {
                    this->myShapes[Part::ShapeMapHasher {}(xp.Current())] = (xp.Current());
                }
                for (xp.Init(aShape, TopAbs_VERTEX, TopAbs_EDGE); xp.More(); xp.Next()) {
                    this->myShapes[Part::ShapeMapHasher {}(xp.Current())] = (xp.Current());
                }
            }
        }

        // getting color
        Quantity_ColorRGBA col;
        if (hColors->GetColor(label, XCAFDoc_ColorGen, col)
            || hColors->GetColor(label, XCAFDoc_ColorSurf, col)
            || hColors->GetColor(label, XCAFDoc_ColorCurv, col)) {
            // add defined color
            myColorMap[Part::ShapeMapHasher {}(aShape)] = col;
        }
        else {
            // http://www.opencascade.org/org/forum/thread_17107/
            TopoDS_Iterator it;
            for (it.Initialize(aShape); it.More(); it.Next()) {
                if (hColors->GetColor(it.Value(), XCAFDoc_ColorGen, col)
                    || hColors->GetColor(it.Value(), XCAFDoc_ColorSurf, col)
                    || hColors->GetColor(it.Value(), XCAFDoc_ColorCurv, col)) {
                    // add defined color
                    myColorMap[Part::ShapeMapHasher {}(it.Value())] = col;
                }
            }
        }

        // getting names
        Handle(TDataStd_Name) name;
        if (label.FindAttribute(TDataStd_Name::GetID(), name)) {
            TCollection_ExtendedString extstr = name->Get();
            char* str = new char[extstr.LengthOfCString() + 1];
            extstr.ToUTF8CString(str);
            std::string labelName(str);
            if (!labelName.empty()) {
                myNameMap[Part::ShapeMapHasher {}(aShape)] = labelName;
            }
            delete[] str;
        }

        if (label.HasChild()) {
            TDF_ChildIterator it;
            for (it.Initialize(label); it.More(); it.Next()) {
                loadShapes(it.Value());
            }
        }
    }
}
