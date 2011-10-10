/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef MESH_SMOOTHING_H
#define MESH_SMOOTHING_H

namespace MeshCore
{
class MeshKernel;
class MeshRefPointToPoints;
class MeshRefPointToFacets;

/** Base class for smoothing algorithms. */
class MeshExport AbstractSmoothing
{
public:
    enum Component { 
        Tangential,         ///< Smooth tangential direction
        Normal,             ///< Smooth normal direction
        TangentialNormal    ///< Smooth tangential and normal direction
    };

    enum Continuity { 
        C0, 
        C1, 
        C2 
    };

    AbstractSmoothing(MeshKernel&);
    virtual ~AbstractSmoothing();
    void initialize(Component comp, Continuity cont);

    /** Smooth the triangle mesh. */
    virtual void Smooth(unsigned int) = 0;

protected:
    MeshKernel& kernel;

    float tolerance;
    Component   component;
    Continuity  continuity;
};

class MeshExport MeshSmoothing : public AbstractSmoothing
{
public:
    MeshSmoothing(MeshKernel&);
    virtual ~MeshSmoothing();
    void Smooth(unsigned int);
};

class MeshExport LaplaceSmoothing : public AbstractSmoothing
{
public:
    LaplaceSmoothing(MeshKernel&);
    virtual ~LaplaceSmoothing();
    void Smooth(unsigned int);
    void SetLambda(double l) { lambda = l;}

protected:
    void Umbrella(const MeshRefPointToPoints&,
                  const MeshRefPointToFacets&, double);

protected:
    double lambda;
};

class MeshExport TaubinSmoothing : public LaplaceSmoothing
{
public:
    TaubinSmoothing(MeshKernel&);
    virtual ~TaubinSmoothing();
    void Smooth(unsigned int);
    void SetMicro(double m) { micro = m;}

protected:
    double micro;
};

} // namespace MeshCore


#endif  // MESH_SMOOTHING_H 
