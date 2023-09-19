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

#include "PreCompiled.h"
#if defined(__MINGW32__)
#define WNT  // avoid conflict with GUID
#endif
#ifndef _PreComp_
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
#include <climits>
#include <gp_Pln.hxx>  // for Precision::Confusion()
#include <gp_Trsf.hxx>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Mod/Part/App/FeatureCompound.h>

#include "ImportOCAF.h"


#ifdef HAVE_TBB
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/task_group.h>
#endif

using namespace Import;

#if OCC_VERSION_HEX >= 0x070500
// See https://dev.opencascade.org/content/occt-3d-viewer-becomes-srgb-aware
#define OCC_COLOR_SPACE Quantity_TOC_sRGB
#else
#define OCC_COLOR_SPACE Quantity_TOC_RGB
#endif

static inline App::Color convertColor(const Quantity_ColorRGBA& c)
{
    Standard_Real r, g, b;
    c.GetRGB().Values(r, g, b, OCC_COLOR_SPACE);
    return App::Color(static_cast<float>(r),
                      static_cast<float>(g),
                      static_cast<float>(b),
                      1.0f - static_cast<float>(c.Alpha()));
}

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
    tryPlacementFromMatrix(part, mtrx);
}

void ImportOCAF::tryPlacementFromMatrix(App::GeoFeature* part, const Base::Matrix4D& mat)
{
    try {
        Base::Placement pl;
        pl.fromMatrix(mat);
        part->Placement.setValue(pl);
    }
    catch (const Base::ValueError& e) {
        e.ReportException();
    }
}

void ImportOCAF::loadShapes()
{
    std::vector<App::DocumentObject*> lValue;
    myRefShapes.clear();
    loadShapes(pDoc->Main(), TopLoc_Location(), default_name, "", false, lValue);
    lValue.clear();
}

void ImportOCAF::setMerge(bool merge)
{
    this->merge = merge;
}

void ImportOCAF::loadShapes(const TDF_Label& label,
                            const TopLoc_Location& loc,
                            const std::string& defaultname,
                            const std::string& assembly,
                            bool isRef,
                            std::vector<App::DocumentObject*>& lValue)
{
    int hash = 0;
#ifdef HAVE_TBB
    using namespace tbb;
    task_group g;
#endif
    TopoDS_Shape aShape;

    std::vector<App::DocumentObject*> localValue;

    if (aShapeTool->GetShape(label, aShape)) {
        hash = aShape.HashCode(HashUpper);
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
    Base::Console().Log("H:%d, N:%s, T:%d, A:%d, S:%d, C:%d, SS:%d, F:%d, R:%d, C:%d, SS:%d\n",
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
                        aShapeTool->IsSubShape(label));
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
        loadShapes(ref, part_loc, part_name, asm_name, true, lValue);
    }

    if (isRef || myRefShapes.find(hash) == myRefShapes.end()) {
        TopoDS_Shape aShape;
        if (isRef && aShapeTool->GetShape(label, aShape)) {
            myRefShapes.insert(aShape.HashCode(HashUpper));
        }

        if (aShapeTool->IsSimpleShape(label) && (isRef || aShapeTool->IsFree(label))) {
            if (!asm_name.empty()) {
                part_name = asm_name;
            }

            // TODO: The merge parameter (last one from createShape) should become an
            // Importer/Exporter option within the FreeCAD preference menu Currently it is merging
            // STEP Compound Shape into a single Shape Part::Feature which is an OpenCascade
            // computed Compound
            if (isRef) {
                createShape(label, loc, part_name, lValue, this->merge);
            }
            else {
                createShape(label, part_loc, part_name, localValue, this->merge);
            }
        }
        else {
            if (aShapeTool->IsSimpleShape(label)) {
                // We are not creating a list of Part::Feature in that case but just
                // a single Part::Feature which has as a Shape a Compound of the Subshapes contained
                // within the global shape
                // This is standard behavior of many STEP reader and avoid to register a crazy
                // amount of Shape within the Tree as STEP file do mostly contain large assemblies
                return;
            }

            // This is probably an Assembly let's try to create a Compound with the name
            for (TDF_ChildIterator it(label); it.More(); it.Next()) {
                if (isRef) {
                    loadShapes(it.Value(), part_loc, part_name, asm_name, false, localValue);
                }
                else {
                    loadShapes(it.Value(), part_loc, part_name, asm_name, isRef, localValue);
                }
            }

            if (!localValue.empty()) {
                if (aShapeTool->IsAssembly(label)) {
                    App::Part* pcPart = nullptr;
                    pcPart = static_cast<App::Part*>(doc->addObject("App::Part", asm_name.c_str()));
                    pcPart->Label.setValue(asm_name);
                    pcPart->addObjects(localValue);

                    // STEP reader is now a hierarchical reader. Node and leaf must have
                    // there local placement updated and relative to the STEP file content
                    // standard FreeCAD placement was absolute we are now moving to relative

                    tryPlacementFromLoc(pcPart, part_loc);
                    lValue.push_back(pcPart);
                }
            }
        }
    }
}

void ImportOCAF::createShape(const TDF_Label& label,
                             const TopLoc_Location& loc,
                             const std::string& name,
                             std::vector<App::DocumentObject*>& lValue,
                             bool mergeShape)
{
    const TopoDS_Shape& aShape = aShapeTool->GetShape(label);
#ifdef HAVE_TBB
    using namespace tbb;
    task_group g;
#endif

    if (!aShape.IsNull() && aShape.ShapeType() == TopAbs_COMPOUND) {
        TopExp_Explorer xp;
        int ctSolids = 0, ctShells = 0, ctVertices = 0, ctEdges = 0;
        std::vector<App::DocumentObject*> localValue;
        App::Part* pcPart = nullptr;

        if (mergeShape) {

            // We should do that only if there is more than a single shape inside
            // Computing Compounds takes time
            // We must keep track of the Color. If there is more than 1 Color into
            // a STEP Compound then the Merge can't be done and we cancel the operation

            BRep_Builder builder;
            TopoDS_Compound comp;
            builder.MakeCompound(comp);

            /*
                        std::vector<App::Color> colors;
                        for (xp.Init(aShape, TopAbs_SOLID); xp.More(); xp.Next(), ctSolids++) {
                            Quantity_Color aColor;
                            App::Color color(0.8f,0.8f,0.8f);
                            if (aColorTool->GetColor(xp.Current(), XCAFDoc_ColorGen, aColor) ||
                                aColorTool->GetColor(xp.Current(), XCAFDoc_ColorSurf, aColor) ||
                                aColorTool->GetColor(xp.Current(), XCAFDoc_ColorCurv, aColor)) {
                                color.r = (float)aColor.Red();
                                color.g = (float)aColor.Green();
                                color.b = (float)aColor.Blue();
                                colors.push_back(color);
                            }
                        }

                        if (colors.size() > 1) {
                            createShape(label, loc, name, lValue, false);
                            return;
                        }
            */
            for (xp.Init(aShape, TopAbs_SOLID); xp.More(); xp.Next(), ctSolids++) {
                const TopoDS_Shape& sh = xp.Current();
                if (!sh.IsNull()) {
                    builder.Add(comp, sh);
                }
            }

            for (xp.Init(aShape, TopAbs_SHELL, TopAbs_SOLID); xp.More(); xp.Next(), ctShells++) {
                const TopoDS_Shape& sh = xp.Current();
                if (!sh.IsNull()) {
                    builder.Add(comp, sh);
                }
            }

            for (xp.Init(aShape, TopAbs_EDGE); xp.More(); xp.Next(), ctEdges++) {
                const TopoDS_Shape& sh = xp.Current();
                if (!sh.IsNull()) {
                    builder.Add(comp, sh);
                }
            }

            for (xp.Init(aShape, TopAbs_VERTEX); xp.More(); xp.Next(), ctVertices++) {
                const TopoDS_Shape& sh = xp.Current();
                if (!sh.IsNull()) {
                    builder.Add(comp, sh);
                }
            }

            // Ok we got a Compound which is computed
            // Just need to add it to a Part::Feature and push it to lValue
            if (!comp.IsNull() && (ctSolids || ctShells || ctEdges || ctVertices)) {
                Part::Feature* part = static_cast<Part::Feature*>(doc->addObject("Part::Feature"));
                // Let's allocate the relative placement of the Compound from the STEP file
                tryPlacementFromLoc(part, loc);
                if (!loc.IsIdentity()) {
                    part->Shape.setValue(comp.Moved(loc));
                }
                else {
                    part->Shape.setValue(comp);
                }

                part->Label.setValue(name);
                lValue.push_back(part);

                loadColors(part, aShape);
            }
        }
        else {
            for (xp.Init(aShape, TopAbs_SOLID); xp.More(); xp.Next(), ctSolids++) {
                createShape(xp.Current(), loc, name, localValue);
            }
            for (xp.Init(aShape, TopAbs_SHELL, TopAbs_SOLID); xp.More(); xp.Next(), ctShells++) {
                createShape(xp.Current(), loc, name, localValue);
            }
        }

        if (!localValue.empty() && !mergeShape) {
            pcPart = static_cast<App::Part*>(doc->addObject("App::Part", name.c_str()));
            pcPart->Label.setValue(name);

            // localValue contain the objects that  must added to the local Part
            // We must add the PartOrigin and the Part itself
            pcPart->addObjects(localValue);

            // Let's compute relative placement of the Part
            /*
                        gp_Trsf trf;
                        Base::Matrix4D mtrx;
                        if ( loc.IsIdentity() )
                             trf = loc.Transformation();
                        else
                             trf = TopLoc_Location(loc.FirstDatum()).Transformation();
                        Part::TopoShape::convertToMatrix(trf, mtrx);
                        Base::Placement pl;
                        pl.fromMatrix(mtrx);
                        pcPart->Placement.setValue(pl);
            */
            lValue.push_back(pcPart);
        }

        if (ctSolids > 0 || ctShells > 0) {
            return;
        }
    }
    else if (!aShape.IsNull()) {
        createShape(aShape, loc, name, lValue);
    }
}

void ImportOCAF::createShape(const TopoDS_Shape& aShape,
                             const TopLoc_Location& loc,
                             const std::string& name,
                             std::vector<App::DocumentObject*>& lvalue)
{
    Part::Feature* part = static_cast<Part::Feature*>(doc->addObject("Part::Feature"));

    if (!loc.IsIdentity()) {
        // part->Shape.setValue(aShape.Moved(TopLoc_Location(loc.FirstDatum())));
        part->Shape.setValue(aShape.Moved(loc));
    }
    else {
        part->Shape.setValue(aShape);
    }

    part->Label.setValue(name);
    lvalue.push_back(part);

    loadColors(part, aShape);
}

void ImportOCAF::loadColors(Part::Feature* part, const TopoDS_Shape& aShape)
{
    Quantity_ColorRGBA aColor;
    App::Color color(0.8f, 0.8f, 0.8f);
    if (aColorTool->GetColor(aShape, XCAFDoc_ColorGen, aColor)
        || aColorTool->GetColor(aShape, XCAFDoc_ColorSurf, aColor)
        || aColorTool->GetColor(aShape, XCAFDoc_ColorCurv, aColor)) {
        color = convertColor(aColor);
        std::vector<App::Color> colors;
        colors.push_back(color);
        applyColors(part, colors);
    }

    TopTools_IndexedMapOfShape faces;
    TopExp_Explorer xp(aShape, TopAbs_FACE);
    while (xp.More()) {
        faces.Add(xp.Current());
        xp.Next();
    }

    bool found_face_color = false;
    std::vector<App::Color> faceColors;
    faceColors.resize(faces.Extent(), color);
    xp.Init(aShape, TopAbs_FACE);
    while (xp.More()) {
        if (aColorTool->GetColor(xp.Current(), XCAFDoc_ColorGen, aColor)
            || aColorTool->GetColor(xp.Current(), XCAFDoc_ColorSurf, aColor)
            || aColorTool->GetColor(xp.Current(), XCAFDoc_ColorCurv, aColor)) {
            int index = faces.FindIndex(xp.Current());
            color = convertColor(aColor);
            faceColors[index - 1] = color;
            found_face_color = true;
        }
        xp.Next();
    }

    if (found_face_color) {
        applyColors(part, faceColors);
    }
}

// ----------------------------------------------------------------------------

ImportOCAFCmd::ImportOCAFCmd(Handle(TDocStd_Document) h, App::Document* d, const std::string& name)
    : ImportOCAF(h, d, name)
{}

void ImportOCAFCmd::applyColors(Part::Feature* part, const std::vector<App::Color>& colors)
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
    part = static_cast<Part::Feature*>(doc->addObject("Part::Feature", default_name.c_str()));
    part->Label.setValue(default_name);
    part->Shape.setValue(shape);
    std::map<Standard_Integer, Quantity_ColorRGBA>::const_iterator jt;
    jt = myColorMap.find(shape.HashCode(INT_MAX));

    App::Color partColor(0.8f, 0.8f, 0.8f);
#if 0  // TODO
    Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(part);
    if (vp && vp->isDerivedFrom(PartGui::ViewProviderPart::getClassTypeId())) {
        if (jt != myColorMap.end()) {
            App::Color color;
            color.r = jt->second.Red();
            color.g = jt->second.Green();
            color.b = jt->second.Blue();
            static_cast<PartGui::ViewProviderPart*>(vp)->ShapeColor.setValue(color);
        }

        partColor = static_cast<PartGui::ViewProviderPart*>(vp)->ShapeColor.getValue();
    }
#endif

    // set label name if defined
    if (setname && !myNameMap.empty()) {
        std::map<Standard_Integer, std::string>::const_iterator jt;
        jt = myNameMap.find(shape.HashCode(INT_MAX));
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

        bool found_face_color = false;
        std::vector<App::Color> faceColors;
        faceColors.resize(faces.Extent(), partColor);
        xp.Init(shape, TopAbs_FACE);
        while (xp.More()) {
            jt = myColorMap.find(xp.Current().HashCode(INT_MAX));
            if (jt != myColorMap.end()) {
                int index = faces.FindIndex(xp.Current());
                faceColors[index - 1] = convertColor(jt->second);
                found_face_color = true;
            }
            xp.Next();
        }

        if (found_face_color) {
#if 0  // TODO
            Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(part);
            if (vp && vp->isDerivedFrom(PartGui::ViewProviderPartExt::getClassTypeId())) {
                static_cast<PartGui::ViewProviderPartExt*>(vp)->DiffuseColor.setValues(faceColors);
            }
#endif
        }
    }
}

void ImportXCAF::loadShapes(const TDF_Label& label)
{
    TopoDS_Shape aShape;
    if (aShapeTool->GetShape(label, aShape)) {
        // if (aShapeTool->IsReference(label)) {
        //     TDF_Label reflabel;
        //     if (aShapeTool->GetReferredShape(label, reflabel)) {
        //         loadShapes(reflabel);
        //     }
        // }
        if (aShapeTool->IsTopLevel(label)) {
            int ctSolids = 0, ctShells = 0, ctComps = 0;
            // add the shapes
            TopExp_Explorer xp;
            for (xp.Init(aShape, TopAbs_SOLID); xp.More(); xp.Next(), ctSolids++) {
                this->mySolids[xp.Current().HashCode(INT_MAX)] = (xp.Current());
            }
            for (xp.Init(aShape, TopAbs_SHELL, TopAbs_SOLID); xp.More(); xp.Next(), ctShells++) {
                this->myShells[xp.Current().HashCode(INT_MAX)] = (xp.Current());
            }
            // if no solids and no shells were found then go for compounds
            if (ctSolids == 0 && ctShells == 0) {
                for (xp.Init(aShape, TopAbs_COMPOUND); xp.More(); xp.Next(), ctComps++) {
                    this->myCompds[xp.Current().HashCode(INT_MAX)] = (xp.Current());
                }
            }
            if (ctComps == 0) {
                for (xp.Init(aShape, TopAbs_FACE, TopAbs_SHELL); xp.More(); xp.Next()) {
                    this->myShapes[xp.Current().HashCode(INT_MAX)] = (xp.Current());
                }
                for (xp.Init(aShape, TopAbs_WIRE, TopAbs_FACE); xp.More(); xp.Next()) {
                    this->myShapes[xp.Current().HashCode(INT_MAX)] = (xp.Current());
                }
                for (xp.Init(aShape, TopAbs_EDGE, TopAbs_WIRE); xp.More(); xp.Next()) {
                    this->myShapes[xp.Current().HashCode(INT_MAX)] = (xp.Current());
                }
                for (xp.Init(aShape, TopAbs_VERTEX, TopAbs_EDGE); xp.More(); xp.Next()) {
                    this->myShapes[xp.Current().HashCode(INT_MAX)] = (xp.Current());
                }
            }
        }

        // getting color
        Quantity_ColorRGBA col;
        if (hColors->GetColor(label, XCAFDoc_ColorGen, col)
            || hColors->GetColor(label, XCAFDoc_ColorSurf, col)
            || hColors->GetColor(label, XCAFDoc_ColorCurv, col)) {
            // add defined color
            myColorMap[aShape.HashCode(INT_MAX)] = col;
        }
        else {
            // http://www.opencascade.org/org/forum/thread_17107/
            TopoDS_Iterator it;
            for (it.Initialize(aShape); it.More(); it.Next()) {
                if (hColors->GetColor(it.Value(), XCAFDoc_ColorGen, col)
                    || hColors->GetColor(it.Value(), XCAFDoc_ColorSurf, col)
                    || hColors->GetColor(it.Value(), XCAFDoc_ColorCurv, col)) {
                    // add defined color
                    myColorMap[it.Value().HashCode(INT_MAX)] = col;
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
                myNameMap[aShape.HashCode(INT_MAX)] = labelName;
            }
            delete[] str;
        }

#if 0
        // http://www.opencascade.org/org/forum/thread_15174/
        if (aShapeTool->IsAssembly(label)) {
            TDF_LabelSequence shapeLabels;
            aShapeTool->GetComponents(label, shapeLabels);
            Standard_Integer nbShapes = shapeLabels.Length();
            for (Standard_Integer i = 1; i <= nbShapes; i++) {
                loadShapes(shapeLabels.Value(i));
            }
        }
#endif

        if (label.HasChild()) {
            TDF_ChildIterator it;
            for (it.Initialize(label); it.More(); it.Next()) {
                loadShapes(it.Value());
            }
        }
    }
}
