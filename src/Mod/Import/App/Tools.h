// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <limits>

#include <Quantity_ColorRGBA.hxx>
#include <TopoDS_Shape.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <Base/Color.h>

#include <Standard_Version.hxx>

namespace Import
{

struct ShapeHasher
{
    std::size_t operator()(const TopoDS_Shape& shape) const
    {
#if OCC_VERSION_HEX >= 0x070800
        return std::hash<TopoDS_Shape> {}(shape);
#else
        return shape.HashCode(std::numeric_limits<int>::max());
#endif
    }
};

struct LabelHasher
{
    std::size_t operator()(const TDF_Label& label) const
    {
#if OCC_VERSION_HEX >= 0x070800
        return std::hash<TDF_Label> {}(label);
#else
        return TDF_LabelMapHasher::HashCode(label, std::numeric_limits<int>::max());
#endif
    }
};

struct Tools
{
    static Base::Color convertColor(const Quantity_ColorRGBA& rgba);
    static Quantity_ColorRGBA convertColor(const Base::Color& col);
    static std::string labelName(TDF_Label label);
    static void printLabel(
        TDF_Label label,
        Handle(XCAFDoc_ShapeTool) aShapeTool,
        Handle(XCAFDoc_ColorTool) aColorTool,
        const char* msg = nullptr
    );

    static void dumpLabels(
        TDF_Label label,
        Handle(XCAFDoc_ShapeTool) aShapeTool,
        Handle(XCAFDoc_ColorTool) aColorTool,
        int depth = 0
    );
};

}  // namespace Import
