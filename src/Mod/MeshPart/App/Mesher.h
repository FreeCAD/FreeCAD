/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef MESHPART_MESHER_H
#define MESHPART_MESHER_H

class TopoDS_Shape;

namespace Mesh { class MeshObject; }
namespace MeshPart {

class Mesher
{
public:
    Mesher(const TopoDS_Shape&);
    ~Mesher();

    void setMaxLength(float s)
    { maxLength = s; }
    float getMaxLength() const
    { return maxLength; }
    void setMaxArea(float s)
    { maxArea = s; }
    float getMaxArea() const
    { return maxArea; }
    void setLocalLength(float s)
    { localLength = s; }
    float getLocalLength() const
    { return localLength; }
    void setDeflection(float s)
    { deflection = s; }
    float getDeflection() const
    { return deflection; }
    void setRegular(bool s)
    { regular = s; }
    bool isRegular() const
    { return regular; }

    Mesh::MeshObject* createMesh() const;

private:
    const TopoDS_Shape& shape;
    float maxLength;
    float maxArea;
    float localLength;
    float deflection;
    bool regular;
};

} // namespace MeshPart

#endif // MESHPART_MESHER_H
