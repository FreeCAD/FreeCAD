/****************************************************************************
 *   Copyright (c) 2017 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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

#ifndef PART_LIBAREA_H
#define PART_LIBAREA_H

#include <Mod/Path/libarea/Area.h>

namespace Part
{

class Libarea {
public:
    CArea myArea;
    gp_Pln myPlane;
    gp_Trsf myMat;

    Libarea(const gp_Pln& plane);

    void Add(const TopoDS_Shape &shape, double deflection=0.01);
    void Add(const TopoDS_Face &face, double deflection=0.01);
    void Add(const TopoDS_Wire& wire, double deflection=0.01);

    TopoDS_Shape Offset(double offset, 
                        short algo, 
                        GeomAbs_JoinType join,
                        bool allowOpenResult,
                        bool fill);

    TopoDS_Shape ToShape(const std::list<CCurve> &curves,bool fill);

    static TopoDS_Shape makeOffset(const TopoDS_Shape &shape, 
                                    double offset, 
                                    short joinType, 
                                    bool fill, 
                                    bool allowOpenResult, 
                                    int algo);
};

} //namespace Part

#endif //PART_LIBAREA_H
