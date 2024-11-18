/****************************************************************************
 *   Copyright (c) 2018 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
#include <TDataStd_Name.hxx>
#include <TDF_ChildIterator.hxx>
#include <TDF_Tool.hxx>
#endif

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "Tools.h"
#include <Base/Console.h>
#include <Mod/Part/App/TopoShape.h>

#if OCC_VERSION_HEX >= 0x070500
// See https://dev.opencascade.org/content/occt-3d-viewer-becomes-srgb-aware
#define OCC_COLOR_SPACE Quantity_TOC_sRGB
#else
#define OCC_COLOR_SPACE Quantity_TOC_RGB
#endif

FC_LOG_LEVEL_INIT("Import", true, true)

using namespace Import;

App::Color Tools::convertColor(const Quantity_ColorRGBA& rgba)
{
    Standard_Real red, green, blue;
    rgba.GetRGB().Values(red, green, blue, OCC_COLOR_SPACE);
    return App::Color(static_cast<float>(red),
                      static_cast<float>(green),
                      static_cast<float>(blue),
                      1.0f - static_cast<float>(rgba.Alpha()));
}

Quantity_ColorRGBA Tools::convertColor(const App::Color& col)
{
    return Quantity_ColorRGBA(Quantity_Color(col.r, col.g, col.b, OCC_COLOR_SPACE), 1.0f - col.a);
}

static inline std::ostream& operator<<(std::ostream& os, const Quantity_ColorRGBA& rgba)
{
    App::Color color = Tools::convertColor(rgba);
    auto toHex = [](float v) {
        return boost::format("%02X") % static_cast<int>(v * 255);
    };
    return os << "#" << toHex(color.r) << toHex(color.g) << toHex(color.b) << toHex(color.a);
}

std::string Tools::labelName(TDF_Label label)
{
    std::string txt;
    Handle(TDataStd_Name) name;
    if (!label.IsNull() && label.FindAttribute(TDataStd_Name::GetID(), name)) {
        TCollection_ExtendedString extstr = name->Get();
        char* str = new char[extstr.LengthOfCString() + 1];
        extstr.ToUTF8CString(str);
        txt = str;
        delete[] str;
        boost::trim(txt);
    }
    return txt;
}

void Tools::printLabel(TDF_Label label,
                       Handle(XCAFDoc_ShapeTool) aShapeTool,
                       Handle(XCAFDoc_ColorTool) aColorTool,
                       const char* msg)
{
    if (label.IsNull() || !FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
        return;
    }
    if (!msg) {
        msg = "Label: ";
    }
    TCollection_AsciiString entry;
    TDF_Tool::Entry(label, entry);
    std::ostringstream ss;
    ss << msg << entry << ", " << labelName(label) << (aShapeTool->IsShape(label) ? ", shape" : "")
       << (aShapeTool->IsTopLevel(label) ? ", topLevel" : "")
       << (aShapeTool->IsFree(label) ? ", free" : "")
       << (aShapeTool->IsAssembly(label) ? ", assembly" : "")
       << (aShapeTool->IsSimpleShape(label) ? ", simple" : "")
       << (aShapeTool->IsCompound(label) ? ", compound" : "")
       << (aShapeTool->IsReference(label) ? ", reference" : "")
       << (aShapeTool->IsComponent(label) ? ", component" : "")
       << (aShapeTool->IsSubShape(label) ? ", subshape" : "");
    if (aShapeTool->IsSubShape(label)) {
        auto shape = aShapeTool->GetShape(label);
        if (!shape.IsNull()) {
            ss << ", " << Part::TopoShape::shapeName(shape.ShapeType(), true);
        }
    }
    if (aShapeTool->IsShape(label)) {
        Quantity_ColorRGBA c;
        if (aColorTool->GetColor(label, XCAFDoc_ColorGen, c)) {
            ss << ", gc: " << c;
        }
        if (aColorTool->GetColor(label, XCAFDoc_ColorSurf, c)) {
            ss << ", sc: " << c;
        }
        if (aColorTool->GetColor(label, XCAFDoc_ColorCurv, c)) {
            ss << ", cc: " << c;
        }
    }

    ss << std::endl;
    Base::Console().Notify<Base::LogStyle::Log>("ImportOCAF2", ss.str().c_str());
}

void Tools::dumpLabels(TDF_Label label,
                       Handle(XCAFDoc_ShapeTool) aShapeTool,
                       Handle(XCAFDoc_ColorTool) aColorTool,
                       int depth)
{
    std::string indent(depth * 2, ' ');
    printLabel(label, aShapeTool, aColorTool, indent.c_str());
    TDF_ChildIterator it;
    for (it.Initialize(label); it.More(); it.Next()) {
        dumpLabels(it.Value(), aShapeTool, aColorTool, depth + 1);
    }
}
