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
#ifdef HAVE_TBB
    using namespace tbb;
    task_group g;
#endif
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
    TopoDS_Shape debugShape;
    int hash = aShapeTool->GetShape(label, debugShape) ? std::hash(debugShape) : 0;
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

    TopoDS_Shape aShape;
    bool foundaShape = aShapeTool->GetShape(label, aShape);

    if (isRef || !foundaShape || !myRefShapes.contains(aShape)) {
        if (isRef && foundaShape) {
            myRefShapes.insert(aShape);
        }

        if (aShapeTool->IsSimpleShape(label)) {
            if (isRef || aShapeTool->IsFree(label)) {
                if (!asm_name.empty()) {
                    part_name = asm_name;
                }
                App::DocumentObject* created = createShape(label, loc, part_name);
                // If !isRef then the label is a Free label which means there are no references to
                // it, so we want it as a top-level object in the FC drawing, so we do not return it
                // to be included in some larger object.
                return isRef ? created : nullptr;
            }
            // A simple shape that does not have a Free Label and is not reached through a ref.
            // This means the shape is reachable through a ref elsewhere in the drawing.

            // We are not creating a list of Part::Feature in that case but just
            // a single Part::Feature which has as a Shape a Compound of the Subshapes contained
            // within the global shape
            // This is standard behavior of many STEP reader and avoid to register a crazy
            // amount of Shape within the Tree as STEP file do mostly contain large assemblies
        }
        else {
            // This IsComponent (or not IsShape), probably an Assembly. Either way we create its
            // contents, and if it turns out to be an Assembly, we will create a Part and place all
            // the contents in that Part. Otherwise they will be left as top-level document objects.
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
        Base::Color faceColor;
        if (getShapeColour(xp.Current(), TDF_Label(), faceColor)) {
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
            faceColors[face_index] = faceColor;
        }
    }

    if (!faceColors.empty()) {
        applyColors(part, faceColors);
    }

    return part;
}
