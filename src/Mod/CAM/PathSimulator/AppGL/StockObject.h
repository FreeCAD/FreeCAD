/***************************************************************************
 *   Copyright (c) 2024 Shai Seger <shaise at gmail>                       *
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

#ifndef __stock_object_h__
#define __stock_object_h__
#include "SimShapes.h"
#include "linmath.h"

namespace MillSim
{

class StockObject
{
public:
    /// <summary>
    /// Create a stock primitive
    /// </summary>
    /// <param name="x">Stock's corner x location</param>
    /// <param name="y">Stock's corner y location</param>
    /// <param name="z">Stock's corner z location</param>
    /// <param name="l">Stock's length (along x)</param>
    /// <param name="w">Stock's width (along y)</param>
    /// <param name="h">Stock's height (along z)</param>
    StockObject();
    virtual ~StockObject();


    /// Calls the display list.
    virtual void render();
    Shape mShape;
    void SetPosition(vec3 position);
    void GenerateBoxStock(float x, float y, float z, float l, float w, float h);
    vec3 mCenter = {};
    vec3 mSize = {};

private:
    float mProfile[8] = {};
    mat4x4 modelMat;
};
}  // namespace MillSim

#endif