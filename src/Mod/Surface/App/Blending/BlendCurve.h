/***************************************************************************
 *   Copyright (c) 2022 Matteo Grellier <matteogrellier@gmail.com>         *
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

#ifndef BLEND_CURVE_H
#define BLEND_CURVE_H

#include <Geom_BezierCurve.hxx>
#include <Mod/Surface/SurfaceGlobal.h>
#include <Base/BaseClass.h>
#include <Mod/Surface/App/Blending/BlendPoint.h>

namespace Surface
{
/*!
* Create a BezierCurve interpolating a list of BlendPoints
*/
class SurfaceExport BlendCurve: public Base::BaseClass
{
public:
    std::vector<BlendPoint> blendPoints;

    BlendCurve();
    /*!
    *  Constructor
    *\param std::vector<BlendPoint>
    */
    BlendCurve(std::vector<BlendPoint> blendPointsList);
    virtual ~BlendCurve();
    /*!
    *  Perform the interpolate algorithm
    *\return the BezierCurve
    */
    Handle(Geom_BezierCurve) compute();
    /*!
    *  Set the size of the first derivative of a BlendPoint
    *\param int index of the BlendPoint to modify
    *\param double new size
    *\param bool interpret new size relative to chordlength
    */
    void setSize(int, double, bool);

    virtual PyObject *getPyObject(void);
};
}// namespace Surface

#endif

