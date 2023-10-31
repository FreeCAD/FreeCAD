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

#ifndef IMPORT_TOOLS_H
#define IMPORT_TOOLS_H

#include <Quantity_ColorRGBA.hxx>
#include <TDF_LabelMapHasher.hxx>
#include <TopoDS_Shape.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <App/Color.h>

namespace Import
{

struct ShapeHasher
{
    std::size_t operator()(const TopoDS_Shape& shape) const
    {
        return shape.HashCode(INT_MAX);
    }
};

struct LabelHasher
{
    std::size_t operator()(const TDF_Label& label) const
    {
        return TDF_LabelMapHasher::HashCode(label, INT_MAX);
    }
};

struct Tools
{
    static App::Color convertColor(const Quantity_ColorRGBA& rgba);
    static Quantity_ColorRGBA convertColor(const App::Color& col);
    static std::string labelName(TDF_Label label);
    static void printLabel(TDF_Label label,
                           Handle(XCAFDoc_ShapeTool) aShapeTool,
                           Handle(XCAFDoc_ColorTool) aColorTool,
                           const char* msg = nullptr);

    static void dumpLabels(TDF_Label label,
                           Handle(XCAFDoc_ShapeTool) aShapeTool,
                           Handle(XCAFDoc_ColorTool) aColorTool,
                           int depth = 0);
};

}  // namespace Import

#endif  // IMPORT_IMPORTOCAF2_H
