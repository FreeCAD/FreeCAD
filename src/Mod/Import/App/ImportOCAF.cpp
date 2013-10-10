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
# define WNT // avoid conflict with GUID
#endif
#ifndef _PreComp_
# include <climits>
# include <Standard_Version.hxx>
# include <BRep_Builder.hxx>
# include <Handle_TDocStd_Document.hxx>
# include <Handle_XCAFApp_Application.hxx>
# include <TDocStd_Document.hxx>
# include <XCAFApp_Application.hxx>
# include <XCAFDoc_DocumentTool.hxx>
# include <XCAFDoc_ShapeTool.hxx>
# include <XCAFDoc_ColorTool.hxx>
# include <XCAFDoc_Location.hxx>
# include <TDF_Label.hxx>
# include <TDF_LabelSequence.hxx>
# include <TDF_ChildIterator.hxx>
# include <TDataStd_Name.hxx>
# include <Quantity_Color.hxx>
# include <STEPCAFControl_Reader.hxx>
# include <STEPCAFControl_Writer.hxx>
# include <STEPControl_Writer.hxx>
# include <IGESCAFControl_Reader.hxx>
# include <IGESCAFControl_Writer.hxx>
# include <IGESControl_Controller.hxx>
# include <Interface_Static.hxx>
# include <Transfer_TransientProcess.hxx>
# include <XSControl_WorkSession.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopTools_MapOfShape.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS_Iterator.hxx>
# include <APIHeaderSection_MakeHeader.hxx>
# include <OSD_Exception.hxx>
#if OCC_VERSION_HEX >= 0x060500
# include <TDataXtd_Shape.hxx>
# else
# include <TDataStd_Shape.hxx>
# endif
#endif

#include "ImportOCAF.h"
#include <Base/Console.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectPy.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/ProgressIndicator.h>
#include <Mod/Part/App/ImportIges.h>
#include <Mod/Part/App/ImportStep.h>


using namespace Import;


ImportOCAF::ImportOCAF(Handle_TDocStd_Document h, App::Document* d, const std::string& name)
    : pDoc(h), doc(d), default_name(name)
{
    aShapeTool = XCAFDoc_DocumentTool::ShapeTool (pDoc->Main());
    aColorTool = XCAFDoc_DocumentTool::ColorTool(pDoc->Main());
}

ImportOCAF::~ImportOCAF()
{
}

void ImportOCAF::loadShapes()
{
    myRefShapes.clear();
    loadShapes(pDoc->Main(), TopLoc_Location(), default_name, "", false);
}

void ImportOCAF::loadShapes(const TDF_Label& label, const TopLoc_Location& loc, const std::string& defaultname, const std::string& assembly, bool isRef)
{
    int hash = 0;
    TopoDS_Shape aShape;
    if (aShapeTool->GetShape(label,aShape)) {
        hash = aShape.HashCode(HashUpper);
    }

    Handle(TDataStd_Name) name;
    std::string part_name = defaultname;
    if (label.FindAttribute(TDataStd_Name::GetID(),name)) {
        TCollection_ExtendedString extstr = name->Get();
        char* str = new char[extstr.LengthOfCString()+1];
        extstr.ToUTF8CString(str);
        part_name = str;
        delete [] str;
        if (part_name.empty()) {
            part_name = defaultname;
        }
        else {
            bool ws=true;
            for (std::string::iterator it = part_name.begin(); it != part_name.end(); ++it) {
                if (*it != ' ') {
                    ws = false;
                    break;
                }
            }
            if (ws)
                part_name = defaultname;
        }
    }

    TopLoc_Location part_loc = loc;
    Handle(XCAFDoc_Location) hLoc;
    if (label.FindAttribute(XCAFDoc_Location::GetID(), hLoc)) {
        if (isRef)
            part_loc = part_loc * hLoc->Get();
        else
            part_loc = hLoc->Get();
    }

#ifdef FC_DEBUG
    Base::Console().Message("H:%d, N:%s, T:%d, A:%d, S:%d, C:%d, SS:%d, F:%d, R:%d, C:%d, SS:%d\n",
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

    std::string asm_name = assembly;
    if (aShapeTool->IsAssembly(label)) {
        asm_name = part_name;
    }

    TDF_Label ref;
    if (aShapeTool->IsReference(label) && aShapeTool->GetReferredShape(label, ref)) {
        loadShapes(ref, part_loc, part_name, asm_name, true);
    }

    if (isRef || myRefShapes.find(hash) == myRefShapes.end()) {
        TopoDS_Shape aShape;
        if (isRef && aShapeTool->GetShape(label, aShape))
            myRefShapes.insert(aShape.HashCode(HashUpper));

        if (aShapeTool->IsSimpleShape(label) && (isRef || aShapeTool->IsFree(label))) {
            if (!asm_name.empty())
                part_name = asm_name;
            if (isRef)
                createShape(label, loc, part_name);
            else
                createShape(label, part_loc, part_name);
        }
        else {
            for (TDF_ChildIterator it(label); it.More(); it.Next()) {
                loadShapes(it.Value(), part_loc, part_name, asm_name, isRef);
            }
        }
    }
}

void ImportOCAF::createShape(const TDF_Label& label, const TopLoc_Location& loc, const std::string& name)
{
    const TopoDS_Shape& aShape = aShapeTool->GetShape(label);
    if (!aShape.IsNull() && aShape.ShapeType() == TopAbs_COMPOUND) {
        TopExp_Explorer xp;
        int ctSolids = 0, ctShells = 0;
        for (xp.Init(aShape, TopAbs_SOLID); xp.More(); xp.Next(), ctSolids++)
            createShape(xp.Current(), loc, name);
        for (xp.Init(aShape, TopAbs_SHELL, TopAbs_SOLID); xp.More(); xp.Next(), ctShells++)
            createShape(xp.Current(), loc, name);
        if (ctSolids > 0 || ctShells > 0)
            return;
    }

    createShape(aShape, loc, name);
}

void ImportOCAF::createShape(const TopoDS_Shape& aShape, const TopLoc_Location& loc, const std::string& name)
{
    Part::Feature* part = static_cast<Part::Feature*>(doc->addObject("Part::Feature"));
    if (!loc.IsIdentity())
        part->Shape.setValue(aShape.Moved(loc));
    else
        part->Shape.setValue(aShape);
    part->Label.setValue(name);

    Quantity_Color aColor;
    App::Color color(0.8f,0.8f,0.8f);
    if (aColorTool->GetColor(aShape, XCAFDoc_ColorGen, aColor) ||
        aColorTool->GetColor(aShape, XCAFDoc_ColorSurf, aColor) ||
        aColorTool->GetColor(aShape, XCAFDoc_ColorCurv, aColor)) {
        color.r = (float)aColor.Red();
        color.g = (float)aColor.Green();
        color.b = (float)aColor.Blue();
        std::vector<App::Color> colors;
        colors.push_back(color);
        applyColors(part, colors);
#if 0//TODO
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(part);
        if (vp && vp->isDerivedFrom(PartGui::ViewProviderPart::getClassTypeId())) {
            color.r = aColor.Red();
            color.g = aColor.Green();
            color.b = aColor.Blue();
            static_cast<PartGui::ViewProviderPart*>(vp)->ShapeColor.setValue(color);
        }
#endif
    }

    TopTools_IndexedMapOfShape faces;
    TopExp_Explorer xp(aShape,TopAbs_FACE);
    while (xp.More()) {
        faces.Add(xp.Current());
        xp.Next();
    }
    bool found_face_color = false;
    std::vector<App::Color> faceColors;
    faceColors.resize(faces.Extent(), color);
    xp.Init(aShape,TopAbs_FACE);
    while (xp.More()) {
        if (aColorTool->GetColor(xp.Current(), XCAFDoc_ColorGen, aColor) ||
            aColorTool->GetColor(xp.Current(), XCAFDoc_ColorSurf, aColor) ||
            aColorTool->GetColor(xp.Current(), XCAFDoc_ColorCurv, aColor)) {
            int index = faces.FindIndex(xp.Current());
            color.r = (float)aColor.Red();
            color.g = (float)aColor.Green();
            color.b = (float)aColor.Blue();
            faceColors[index-1] = color;
            found_face_color = true;
        }
        xp.Next();
    }

    if (found_face_color) {
        applyColors(part, faceColors);
#if 0//TODO
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(part);
        if (vp && vp->isDerivedFrom(PartGui::ViewProviderPartExt::getClassTypeId())) {
            static_cast<PartGui::ViewProviderPartExt*>(vp)->DiffuseColor.setValues(faceColors);
        }
#endif
    }
}

// ----------------------------------------------------------------------------

ExportOCAF::ExportOCAF(Handle_TDocStd_Document h)
    : pDoc(h)
{
    aShapeTool = XCAFDoc_DocumentTool::ShapeTool(pDoc->Main());
    aColorTool = XCAFDoc_DocumentTool::ColorTool(pDoc->Main());
    rootLabel = TDF_TagSource::NewChild(pDoc->Main());
}

void ExportOCAF::saveShape(Part::Feature* part, const std::vector<App::Color>& colors)
{
    const TopoDS_Shape& shape = part->Shape.getValue();

    // Add shape and name
    //TDF_Label shapeLabel = hShapeTool->AddShape(shape, Standard_False);
    TDF_Label shapeLabel= TDF_TagSource::NewChild(rootLabel);
#if OCC_VERSION_HEX >= 0x060500
    TDataXtd_Shape::Set(shapeLabel, shape);
#else
    TDataStd_Shape::Set(shapeLabel, shape);
#endif
    TDataStd_Name::Set(shapeLabel, TCollection_ExtendedString(part->Label.getValue(), 1));

    // Add color information
    Quantity_Color col;

    std::set<int> face_index;
    TopTools_IndexedMapOfShape faces;
    TopExp_Explorer xp(shape,TopAbs_FACE);
    while (xp.More()) {
        face_index.insert(faces.Add(xp.Current()));
        xp.Next();
    }

    // define color per face?
    if (colors.size() == face_index.size()) {
        xp.Init(shape,TopAbs_FACE);
        while (xp.More()) {
            int index = faces.FindIndex(xp.Current());
            if (face_index.find(index) != face_index.end()) {
                face_index.erase(index);
                TDF_Label faceLabel= TDF_TagSource::NewChild(shapeLabel);
#if OCC_VERSION_HEX >= 0x060500
                TDataXtd_Shape::Set(faceLabel, xp.Current());
#else
                TDataStd_Shape::Set(faceLabel, xp.Current());
#endif
                const App::Color& color = colors[index-1];
                Quantity_Parameter mat[3];
                mat[0] = color.r;
                mat[1] = color.g;
                mat[2] = color.b;
                col.SetValues(mat[0],mat[1],mat[2],Quantity_TOC_RGB);
                aColorTool->SetColor(faceLabel, col, XCAFDoc_ColorSurf);
            }
            xp.Next();
        }
    }
    else if (!colors.empty()) {
        App::Color color = colors.front();
        Quantity_Parameter mat[3];
        mat[0] = color.r;
        mat[1] = color.g;
        mat[2] = color.b;
        col.SetValues(mat[0],mat[1],mat[2],Quantity_TOC_RGB);
        aColorTool->SetColor(shapeLabel, col, XCAFDoc_ColorGen);
    }
}

// ----------------------------------------------------------------------------

ImportXCAF::ImportXCAF(Handle_TDocStd_Document h, App::Document* d, const std::string& name)
    : hdoc(h), doc(d), default_name(name)
{
    aShapeTool = XCAFDoc_DocumentTool::ShapeTool (hdoc->Main());
    hColors = XCAFDoc_DocumentTool::ColorTool(hdoc->Main());
}

ImportXCAF::~ImportXCAF()
{
}

void ImportXCAF::loadShapes()
{
    // collect sequence of labels to display
    TDF_LabelSequence shapeLabels, colorLabels;
    aShapeTool->GetFreeShapes (shapeLabels);
    hColors->GetColors(colorLabels);

    // set presentations and show
    for (Standard_Integer i=1; i <= shapeLabels.Length(); i++ ) {
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
    part->Shape.setValue(shape);
    std::map<Standard_Integer, Quantity_Color>::const_iterator jt;
    jt = myColorMap.find(shape.HashCode(INT_MAX));

    App::Color partColor(0.8f,0.8f,0.8f);
#if 0//TODO
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
        TopExp_Explorer xp(shape,TopAbs_FACE);
        while (xp.More()) {
            faces.Add(xp.Current());
            xp.Next();
        }

        bool found_face_color = false;
        std::vector<App::Color> faceColors;
        faceColors.resize(faces.Extent(), partColor);
        xp.Init(shape,TopAbs_FACE);
        while (xp.More()) {
            jt = myColorMap.find(xp.Current().HashCode(INT_MAX));
            if (jt != myColorMap.end()) {
                int index = faces.FindIndex(xp.Current());
                App::Color color;
                color.r = (float)jt->second.Red();
                color.g = (float)jt->second.Green();
                color.b = (float)jt->second.Blue();
                faceColors[index-1] = color;
                found_face_color = true;
            }
            xp.Next();
        }

        if (found_face_color) {
#if 0//TODO
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
    if (aShapeTool->GetShape(label,aShape)) {
        //if (aShapeTool->IsReference(label)) {
        //    TDF_Label reflabel;
        //    if (aShapeTool->GetReferredShape(label, reflabel)) {
        //        loadShapes(reflabel);
        //    }
        //}
        if (aShapeTool->IsTopLevel(label)) {
            int ctSolids = 0, ctShells = 0, ctComps = 0;
            // add the shapes
            TopExp_Explorer xp;
            for (xp.Init(aShape, TopAbs_SOLID); xp.More(); xp.Next(), ctSolids++)
                this->mySolids[xp.Current().HashCode(INT_MAX)] = (xp.Current());
            for (xp.Init(aShape, TopAbs_SHELL, TopAbs_SOLID); xp.More(); xp.Next(), ctShells++)
                this->myShells[xp.Current().HashCode(INT_MAX)] = (xp.Current());
            // if no solids and no shells were found then go for compounds
            if (ctSolids == 0 && ctShells == 0) {
                for (xp.Init(aShape, TopAbs_COMPOUND); xp.More(); xp.Next(), ctComps++)
                    this->myCompds[xp.Current().HashCode(INT_MAX)] = (xp.Current());
            }
            if (ctComps == 0) {
                for (xp.Init(aShape, TopAbs_FACE, TopAbs_SHELL); xp.More(); xp.Next())
                    this->myShapes[xp.Current().HashCode(INT_MAX)] = (xp.Current());
                for (xp.Init(aShape, TopAbs_WIRE, TopAbs_FACE); xp.More(); xp.Next())
                    this->myShapes[xp.Current().HashCode(INT_MAX)] = (xp.Current());
                for (xp.Init(aShape, TopAbs_EDGE, TopAbs_WIRE); xp.More(); xp.Next())
                    this->myShapes[xp.Current().HashCode(INT_MAX)] = (xp.Current());
                for (xp.Init(aShape, TopAbs_VERTEX, TopAbs_EDGE); xp.More(); xp.Next())
                    this->myShapes[xp.Current().HashCode(INT_MAX)] = (xp.Current());
            }
        }

        // getting color
        Quantity_Color col;
        if (hColors->GetColor(label, XCAFDoc_ColorGen, col) ||
            hColors->GetColor(label, XCAFDoc_ColorSurf, col) ||
            hColors->GetColor(label, XCAFDoc_ColorCurv, col)) {
            // add defined color
            myColorMap[aShape.HashCode(INT_MAX)] = col;
        }
        else {
            // http://www.opencascade.org/org/forum/thread_17107/
            TopoDS_Iterator it;
            for (it.Initialize(aShape);it.More(); it.Next()) {
                if (hColors->GetColor(it.Value(), XCAFDoc_ColorGen, col) ||
                    hColors->GetColor(it.Value(), XCAFDoc_ColorSurf, col) ||
                    hColors->GetColor(it.Value(), XCAFDoc_ColorCurv, col)) {
                    // add defined color
                    myColorMap[it.Value().HashCode(INT_MAX)] = col;
                }
            }
        }

        // getting names
        Handle(TDataStd_Name) name;
        if (label.FindAttribute(TDataStd_Name::GetID(),name)) {
            TCollection_ExtendedString extstr = name->Get();
            char* str = new char[extstr.LengthOfCString()+1];
            extstr.ToUTF8CString(str);
            std::string label(str);
            if (!label.empty())
                myNameMap[aShape.HashCode(INT_MAX)] = label;
            delete [] str;
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
