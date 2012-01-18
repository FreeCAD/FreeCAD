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

    class Displacement {
      
    public:
        Displacement(){x = 0; y = 0; z = 0;}
        double *x, *y, *z;
    };
    
    class Point {
     
    public:
      Point() { x=y=z=0;}
      double x,y,z;
    };

    class Normal : public Point {
     
    public:
      Normal() {};
    };
    
    class Quaternion {
      
    public:
      Quaternion() {a = b = c = d = 0;};      
      double *a, *b, *c, *d;
    };
    
    class Solid {
      
    public:
      Solid() {};
      Quaternion 	q;
      Displacement 	d;
      Normal 		n;
      Point 		p;
    };
} //namespace GCS

#endif // FREEGCS_GEO_H
