/***************************************************************************
 *   Copyright (c) Konstantinos Poulios      (logari81@gmail.com) 2011     *
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

#ifndef FREEGCS_GEO_H
#define FREEGCS_GEO_H

namespace GCS
{

    ///////////////////////////////////////
    // Geometries
    ///////////////////////////////////////

    class Point
    {
    public:
        Point(){x = 0; y = 0;}
        double *x;
        double *y;
    };

    class Line
    {
    public:
        Line(){}
        Point p1;
        Point p2;
    };

    class Arc
    {
    public:
        Arc(){startAngle=0;endAngle=0;rad=0;}
        double *startAngle;
        double *endAngle;
        double *rad;
        Point start;
        Point end;
        Point center;
    };

    class Circle
    {
    public:
        Circle(){rad = 0;}
        Point center;
        double *rad;
    };

} //namespace GCS

#endif // FREEGCS_GEO_H
