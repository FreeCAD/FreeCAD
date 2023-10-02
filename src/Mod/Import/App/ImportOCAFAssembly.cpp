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
#include <Quantity_Color.hxx>
#include <Standard_Version.hxx>
#include <TDF_ChildIterator.hxx>
#include <TDF_Label.hxx>
#include <TDataStd_Name.hxx>
#include <TDocStd_Document.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_Location.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <climits>
#include <sstream>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Mod/Part/App/PartFeature.h>

#include "ImportOCAFAssembly.h"


using namespace Import;


ImportOCAFAssembly::ImportOCAFAssembly(Handle(TDocStd_Document) h,
                                       App::Document* d,
                                       const std::string& name,
                                       App::DocumentObject* target)
    : pDoc(h)
    , doc(d)
    , default_name(name)
    , targetObj(target)
{
    aShapeTool = XCAFDoc_DocumentTool::ShapeTool(pDoc->Main());
    aColorTool = XCAFDoc_DocumentTool::ColorTool(pDoc->Main());
}

ImportOCAFAssembly::~ImportOCAFAssembly()
{}

void ImportOCAFAssembly::loadShapes()
{
    myRefShapes.clear();
    loadShapes(pDoc->Main(), TopLoc_Location(), default_name, "", false, 0);
}


void ImportOCAFAssembly::loadAssembly()
{
    myRefShapes.clear();
    loadShapes(pDoc->Main(), TopLoc_Location(), default_name, "", false, 0);
}


std::string ImportOCAFAssembly::getName(const TDF_Label& label)
{
    Handle(TDataStd_Name) name;
    std::string part_name;
    if (label.FindAttribute(TDataStd_Name::GetID(), name)) {
        TCollection_ExtendedString extstr = name->Get();
        char* str = new char[extstr.LengthOfCString() + 1];
        extstr.ToUTF8CString(str);
        part_name = str;
        delete[] str;
        return part_name;
    }

    return "";
}


void ImportOCAFAssembly::loadShapes(const TDF_Label& label,
                                    const TopLoc_Location& loc,
                                    const std::string& defaultname,
                                    const std::string& assembly,
                                    bool isRef,
                                    int dep)
{
    int hash = 0;
    TopoDS_Shape aShape;
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
            for (std::string::iterator it = part_name.begin(); it != part_name.end(); ++it) {
                if (*it != ' ') {
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
    const char* s;
    if (!hLoc.IsNull()) {
        s = hLoc->Get().IsIdentity() ? "0" : "1";
    }
    else {
        s = "0";
    }

    std::stringstream str;

    Base::Console().Log("H:%-9d \tN:%-30s \tTop:%d, Asm:%d, Shape:%d, Compound:%d, Simple:%d, "
                        "Free:%d, Ref:%d, Component:%d, SubShape:%d\tTrf:%s-- Dep:%d  \n",
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
                        aShapeTool->IsSubShape(label),
                        s,
                        dep);

    label.Dump(str);
    Base::Console().Message(str.str().c_str());
#endif

    std::string asm_name = assembly;
    if (aShapeTool->IsAssembly(label)) {
        asm_name = part_name;
    }

    TDF_Label ref;
    if (aShapeTool->IsReference(label) && aShapeTool->GetReferredShape(label, ref)) {
        loadShapes(ref, part_loc, part_name, asm_name, true, dep + 1);
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
            if (isRef) {
                createShape(label, loc, part_name);
            }
            else {
                createShape(label, part_loc, part_name);
            }
        }
        else {
            for (TDF_ChildIterator it(label); it.More(); it.Next()) {
                loadShapes(it.Value(), part_loc, part_name, asm_name, isRef, dep + 1);
            }
        }
    }
}

void ImportOCAFAssembly::createShape(const TDF_Label& label,
                                     const TopLoc_Location& loc,
                                     const std::string& name)
{
    Base::Console().Log("-create Shape\n");
    const TopoDS_Shape& aShape = aShapeTool->GetShape(label);
    if (!aShape.IsNull() && aShape.ShapeType() == TopAbs_COMPOUND) {
        TopExp_Explorer xp;
        int ctSolids = 0, ctShells = 0;
        for (xp.Init(aShape, TopAbs_SOLID); xp.More(); xp.Next(), ctSolids++) {
            createShape(xp.Current(), loc, name);
        }
        for (xp.Init(aShape, TopAbs_SHELL, TopAbs_SOLID); xp.More(); xp.Next(), ctShells++) {
            createShape(xp.Current(), loc, name);
        }
        if (ctSolids > 0 || ctShells > 0) {
            return;
        }
    }
    createShape(aShape, loc, name);
}

void ImportOCAFAssembly::createShape(const TopoDS_Shape& aShape,
                                     const TopLoc_Location& loc,
                                     const std::string& name)
{
    Part::Feature* part = static_cast<Part::Feature*>(doc->addObject("Part::Feature"));
    if (!loc.IsIdentity()) {
        part->Shape.setValue(aShape.Moved(loc));
    }
    else {
        part->Shape.setValue(aShape);
    }
    part->Label.setValue(name);

    Quantity_Color aColor;
    App::Color color(0.8f, 0.8f, 0.8f);
    if (aColorTool->GetColor(aShape, XCAFDoc_ColorGen, aColor)
        || aColorTool->GetColor(aShape, XCAFDoc_ColorSurf, aColor)
        || aColorTool->GetColor(aShape, XCAFDoc_ColorCurv, aColor)) {
        color.r = (float)aColor.Red();
        color.g = (float)aColor.Green();
        color.b = (float)aColor.Blue();
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
            color.r = (float)aColor.Red();
            color.g = (float)aColor.Green();
            color.b = (float)aColor.Blue();
            faceColors[index - 1] = color;
            found_face_color = true;
        }
        xp.Next();
    }

    if (found_face_color) {
        applyColors(part, faceColors);
    }
}
