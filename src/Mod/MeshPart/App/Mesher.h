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

#include <sstream>
#include <Base/Stream.h>

class TopoDS_Shape;

namespace Mesh { class MeshObject; }
namespace MeshPart {

class Mesher
{
public:
    enum Method {
        None = 0,
        Mefisto = 1,
#if defined (HAVE_NETGEN)
        Netgen = 2,
#endif
    };

    Mesher(const TopoDS_Shape&);
    ~Mesher();

    void setMethod(Method m)
    { method = m; }
    Method getMethod() const
    { return method; }

    /** @name Mefisto settings */
    //@{
    void setMaxLength(double s)
    { maxLength = s; }
    double getMaxLength() const
    { return maxLength; }
    void setMaxArea(double s)
    { maxArea = s; }
    double getMaxArea() const
    { return maxArea; }
    void setLocalLength(double s)
    { localLength = s; }
    double getLocalLength() const
    { return localLength; }
    void setDeflection(double s)
    { deflection = s; }
    double getDeflection() const
    { return deflection; }
    void setMinMaxLengths(double f, double l)
    { minLen = f; maxLen = l; }
    void getMinMaxLengths(double& f, double& l) const
    { f = minLen; l= maxLen; }
    void setRegular(bool s)
    { regular = s; }
    bool isRegular() const
    { return regular; }
    //@}

#if defined (HAVE_NETGEN)
    /** @name Netgen settings */
    //@{
    void setFineness(int s)
    { fineness = s; }
    int getFineness() const
    { return fineness; }
    void setGrowthRate(double r)
    { growthRate = r; }
    double getGrowthRate() const
    { return growthRate; }
    void setNbSegPerEdge(double v)
    { nbSegPerEdge = v;}
    double getNbSegPerEdge() const
    { return nbSegPerEdge; }
    void setNbSegPerRadius(double v)
    { nbSegPerRadius = v; }
    double getNbSegPerRadius() const
    { return nbSegPerRadius; }
    void setSecondOrder(bool on)
    { secondOrder = on; }
    bool getSecondOrder() const
    { return secondOrder; }
    void setOptimize(bool on)
    { optimize = on;}
    bool getOptimize() const
    { return optimize; }
    void setQuadAllowed(bool on)
    { allowquad = on;}
    bool isQuadAllowed() const
    { return allowquad; }
    //@}
#endif

    Mesh::MeshObject* createMesh() const;

private:
    const TopoDS_Shape& shape;
    Method method;
    double maxLength;
    double maxArea;
    double localLength;
    double deflection;
    double minLen, maxLen;
    bool regular;
#if defined (HAVE_NETGEN)
    int fineness;
    double growthRate;
    double nbSegPerEdge;
    double nbSegPerRadius;
    bool secondOrder;
    bool optimize;
    bool allowquad;
#endif
};

class MeshingOutput : public std::streambuf
{
public:
    MeshingOutput();

protected:
    int overflow(int c = EOF);
    int sync();

private:
    std::string buffer;
};

} // namespace MeshPart

#endif // MESHPART_MESHER_H
