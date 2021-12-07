/***************************************************************************
 *   Copyright (c) 2007 Joachim Zettler <Joachim.Zettler@gmx.de>           *
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
/**\file
\brief Mostly functors for less operators can be found here
*/

#ifndef STUFF_H
#define STUFF_H

#include <TopoDS_Wire.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>

/**\brief Functor to sort a pair of type pair<float,TopoDS_Wire> 

Mostly used in the cutting class as a sorting of the different resulting wires is necessary if there is
more then one resulting cutting curve
\param _Left A pair which consists of a float value (mostly here the Z-Level is stored) and a TopoDS_Wire
\param _Right Exactly the same as _Left but here it is used to compare with _First
\return A Boolean Value which is either true != 0 or false == 0 depending which Z-Level is higher or lower
*/
bool inline FloatWirePairHuge(const std::pair<float,TopoDS_Wire>& _Left, const std::pair<float,TopoDS_Wire>& _Right)
{
    return _Left.first > _Right.first;
}


/**\brief Functor to sort two float values 

Just a test function which is able to sort also from high to low and not only from low to high 
(like the less operator is normally doing it)
\param _Left A float value
\param _Right A float value for the comparison with the _Left
\return A Boolean Value which is either true != 0 or false == 0 depending which float value is higher
*/
bool inline FloatHuge(const float _Left, const float _Right)
{
    return _Left > _Right;
}

/**\brief A container to have an easy access to an edge with start and endpoint*/
struct edge_container
{
    TopoDS_Edge edge;
    gp_Pnt firstPoint;
    gp_Pnt lastPoint;

};

/**\brief A container used for projecting points on faces along a normal vector*/
struct projectPointContainer
{
    gp_Pnt point;
    gp_Dir normalvector;
    TopoDS_Face face;
};

#endif /*DEFINE STUFF_H*/

