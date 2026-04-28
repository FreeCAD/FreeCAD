// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <sstream>

#include <Base/Stream.h>

#ifdef HAVE_SMESH
# include <SMESH_Version.h>
#endif

class TopoDS_Shape;
class SMESH_Gen;
class SMESH_Mesh;

namespace Mesh
{
class MeshObject;
}
namespace MeshPart
{

class Mesher
{
public:
    enum Method
    {
        None = 0,
        Mefisto = 1,
#if defined(HAVE_NETGEN)
        Netgen = 2,
#endif
        Standard = 3
    };

    explicit Mesher(const TopoDS_Shape&);
    ~Mesher();

    void setMethod(Method m)
    {
        method = m;
    }
    Method getMethod() const
    {
        return method;
    }

    /** @name Mefisto settings */
    //@{
    void setMaxLength(double s)
    {
        maxLength = s;
    }
    double getMaxLength() const
    {
        return maxLength;
    }
    void setMaxArea(double s)
    {
        maxArea = s;
    }
    double getMaxArea() const
    {
        return maxArea;
    }
    void setLocalLength(double s)
    {
        localLength = s;
    }
    double getLocalLength() const
    {
        return localLength;
    }
    void setDeflection(double s)
    {
        deflection = s;
    }
    double getDeflection() const
    {
        return deflection;
    }
    void setAngularDeflection(double s)
    {
        angularDeflection = s;
    }
    double getAngularDeflection() const
    {
        return angularDeflection;
    }
    void setMinMaxLengths(double f, double l)
    {
        minLen = f;
        maxLen = l;
    }
    void getMinMaxLengths(double& f, double& l) const
    {
        f = minLen;
        l = maxLen;
    }
    void setRegular(bool s)
    {
        regular = s;
    }
    bool isRegular() const
    {
        return regular;
    }
    void setRelative(bool s)
    {
        relative = s;
    }
    bool isRelative() const
    {
        return relative;
    }
    void setSegments(bool s)
    {
        segments = s;
    }
    bool isSegments() const
    {
        return segments;
    }
    void setColors(const std::vector<uint32_t>& c)
    {
        colors = c;
    }
    //@}

#if defined(HAVE_NETGEN)
    /** @name Netgen settings */
    //@{
    void setFineness(int s)
    {
        fineness = s;
    }
    int getFineness() const
    {
        return fineness;
    }
    void setGrowthRate(double r)
    {
        growthRate = r;
    }
    double getGrowthRate() const
    {
        return growthRate;
    }
    void setNbSegPerEdge(double v)
    {
        nbSegPerEdge = v;
    }
    double getNbSegPerEdge() const
    {
        return nbSegPerEdge;
    }
    void setNbSegPerRadius(double v)
    {
        nbSegPerRadius = v;
    }
    double getNbSegPerRadius() const
    {
        return nbSegPerRadius;
    }
    void setSecondOrder(bool on)
    {
        secondOrder = on;
    }
    bool getSecondOrder() const
    {
        return secondOrder;
    }
    void setOptimize(bool on)
    {
        optimize = on;
    }
    bool getOptimize() const
    {
        return optimize;
    }
    void setQuadAllowed(bool on)
    {
        allowquad = on;
    }
    bool isQuadAllowed() const
    {
        return allowquad;
    }
    //@}
#endif

    Mesh::MeshObject* createMesh() const;

private:
    Mesh::MeshObject* createStandard() const;
    Mesh::MeshObject* createFrom(SMESH_Mesh*) const;

private:
    const TopoDS_Shape& shape;
    Method method {None};
    double maxLength {0};
    double maxArea {0};
    double localLength {0};
    double deflection {0};
    double angularDeflection {0.5};
    double minLen {0}, maxLen {0};
    bool relative {false};
    bool regular {false};
    bool segments {false};
#if defined(HAVE_NETGEN)
    int fineness {5};
    double growthRate {0};
    double nbSegPerEdge {0};
    double nbSegPerRadius {0};
    bool secondOrder {false};
    bool optimize {true};
    bool allowquad {false};
#endif
    std::vector<uint32_t> colors;

    static SMESH_Gen* _mesh_gen;
};

class MeshingOutput: public std::streambuf
{
public:
    MeshingOutput();

protected:
    int overflow(int c = EOF) override;
    int sync() override;

private:
    std::string buffer;
};

}  // namespace MeshPart
