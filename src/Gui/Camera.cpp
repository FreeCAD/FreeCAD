// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"

#include "Camera.h"

using namespace Gui;


/**
 Formulas to get quaternion for axonometric views:

 \code
from math import sqrt, degrees, asin, atan
p1=App.Rotation(App.Vector(1,0,0),90)
p2=App.Rotation(App.Vector(0,0,1),alpha)
p3=App.Rotation(p2.multVec(App.Vector(1,0,0)),beta)
p4=p3.multiply(p2).multiply(p1)

from pivy import coin
c=Gui.ActiveDocument.ActiveView.getCameraNode()
c.orientation.setValue(*p4.Q)
 \endcode

 The angles alpha and beta depend on the type of axonometry
 Isometric:
 \code
alpha=45
beta=degrees(asin(-sqrt(1.0/3.0)))
 \endcode

 Dimetric:
 \code
alpha=degrees(asin(sqrt(1.0/8.0)))
beta=degrees(-asin(1.0/3.0))
 \endcode

 Trimetric:
 \code
alpha=30.0
beta=-35.0
 \endcode

 Verification code that the axonomtries are correct:

 \code
from pivy import coin
c=Gui.ActiveDocument.ActiveView.getCameraNode()
vo=App.Vector(c.getViewVolume().getMatrix().multVecMatrix(coin.SbVec3f(0,0,0)).getValue())
vx=App.Vector(c.getViewVolume().getMatrix().multVecMatrix(coin.SbVec3f(10,0,0)).getValue())
vy=App.Vector(c.getViewVolume().getMatrix().multVecMatrix(coin.SbVec3f(0,10,0)).getValue())
vz=App.Vector(c.getViewVolume().getMatrix().multVecMatrix(coin.SbVec3f(0,0,10)).getValue())
(vx-vo).Length
(vy-vo).Length
(vz-vo).Length

# Projection
vo.z=0
vx.z=0
vy.z=0
vz.z=0

(vx-vo).Length
(vy-vo).Length
(vz-vo).Length
 \endcode

 See also:
 http://www.mathematik.uni-marburg.de/~thormae/lectures/graphics1/graphics_6_2_ger_web.html#1
 http://www.mathematik.uni-marburg.de/~thormae/lectures/graphics1/code_v2/Axonometric/qt/Axonometric.cpp
 https://de.wikipedia.org/wiki/Arkussinus_und_Arkuskosinus
*/

SbRotation Camera::rotation(Camera::Orientation view)
{
    switch (view) {
    case Top:
        return SbRotation(0, 0, 0, 1);
    case Bottom:
        return SbRotation(1, 0, 0, 0);
    case Front: {
        auto root = (float)(sqrt(2.0)/2.0);
        return SbRotation(root, 0, 0, root);
    }
    case Rear: {
        auto root = (float)(sqrt(2.0)/2.0);
        return SbRotation(0, root, root, 0);
    }
    case Right:
        return SbRotation(0.5, 0.5, 0.5, 0.5);
    case Left:
        return SbRotation(-0.5, 0.5, 0.5, -0.5);
    case Isometric:
        //from math import sqrt, degrees, asin
        //p1=App.Rotation(App.Vector(1,0,0),45)
        //p2=App.Rotation(App.Vector(0,0,1),-45)
        //p3=p2.multiply(p1)
        //return SbRotation(0.353553f, -0.146447f, -0.353553f, 0.853553f);

        //from math import sqrt, degrees, asin
        //p1=App.Rotation(App.Vector(1,0,0),90)
        //p2=App.Rotation(App.Vector(0,0,1),135)
        //p3=App.Rotation(App.Vector(-1,1,0),degrees(asin(-sqrt(1.0/3.0))))
        //p4=p3.multiply(p2).multiply(p1)
        //return SbRotation(0.17592, 0.424708, 0.820473, 0.339851);

        //from math import sqrt, degrees, asin
        //p1=App.Rotation(App.Vector(1,0,0),90)
        //p2=App.Rotation(App.Vector(0,0,1),45)
        //#p3=App.Rotation(App.Vector(1,1,0),45)
        //p3=App.Rotation(App.Vector(1,1,0),degrees(asin(-sqrt(1.0/3.0))))
        //p4=p3.multiply(p2).multiply(p1)
        return SbRotation(0.424708f, 0.17592f, 0.339851f, 0.820473f);
    case Dimetric:
        return SbRotation(0.567952f, 0.103751f, 0.146726f, 0.803205f);
    case Trimetric:
        return SbRotation(0.446015f, 0.119509f, 0.229575f, 0.856787f);
    default:
        return SbRotation(0, 0, 0, 1);
    }
}
