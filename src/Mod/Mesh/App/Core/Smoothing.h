// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <limits>
#include <vector>

#include "Definitions.h"


namespace MeshCore
{
class MeshKernel;
class MeshRefPointToPoints;
class MeshRefPointToFacets;
class MeshRefFacetToFacets;

/** Base class for smoothing algorithms. */
class MeshExport AbstractSmoothing
{
public:
    enum Component
    {
        Tangential,       ///< Smooth tangential direction
        Normal,           ///< Smooth normal direction
        TangentialNormal  ///< Smooth tangential and normal direction
    };

    enum Continuity
    {
        C0,
        C1,
        C2
    };

    explicit AbstractSmoothing(MeshKernel&);
    virtual ~AbstractSmoothing();
    AbstractSmoothing(const AbstractSmoothing&) = delete;
    AbstractSmoothing(AbstractSmoothing&&) = delete;
    AbstractSmoothing& operator=(const AbstractSmoothing&) = delete;
    AbstractSmoothing& operator=(AbstractSmoothing&&) = delete;

    void initialize(Component comp, Continuity cont);

    /** Smooth the triangle mesh. */
    virtual void Smooth(unsigned int) = 0;
    virtual void SmoothPoints(unsigned int, const std::vector<PointIndex>&) = 0;

protected:
    // NOLINTBEGIN
    MeshKernel& kernel;

    Component component {Normal};
    Continuity continuity {C0};
    // NOLINTEND
};

class MeshExport PlaneFitSmoothing: public AbstractSmoothing
{
public:
    explicit PlaneFitSmoothing(MeshKernel&);
    void SetMaximum(float max)
    {
        maximum = max;
    }
    void Smooth(unsigned int) override;
    void SmoothPoints(unsigned int, const std::vector<PointIndex>&) override;

private:
    float maximum {std::numeric_limits<float>::max()};
};

class MeshExport LaplaceSmoothing: public AbstractSmoothing
{
public:
    explicit LaplaceSmoothing(MeshKernel&);
    void Smooth(unsigned int) override;
    void SmoothPoints(unsigned int, const std::vector<PointIndex>&) override;
    void SetLambda(double l)
    {
        lambda = l;
    }
    double GetLambda() const
    {
        return lambda;
    }

protected:
    void Umbrella(const MeshRefPointToPoints&, const MeshRefPointToFacets&, double);
    void Umbrella(
        const MeshRefPointToPoints&,
        const MeshRefPointToFacets&,
        double,
        const std::vector<PointIndex>&
    );

private:
    double lambda {0.6307};
};

class MeshExport TaubinSmoothing: public LaplaceSmoothing
{
public:
    explicit TaubinSmoothing(MeshKernel&);
    void Smooth(unsigned int) override;
    void SmoothPoints(unsigned int, const std::vector<PointIndex>&) override;
    void SetMicro(double m)
    {
        micro = m;
    }

private:
    double micro {0.0424};
};

/*!
 * \brief The MedianFilterSmoothing class
 * Smoothing based on median filter from the paper:
 * Mesh Median Filter for Smoothing 3-D Polygonal Surfaces
 */
class MeshExport MedianFilterSmoothing: public AbstractSmoothing
{
public:
    explicit MedianFilterSmoothing(MeshKernel&);
    void SetWeight(int w)
    {
        weights = w;
    }
    void Smooth(unsigned int) override;
    void SmoothPoints(unsigned int, const std::vector<PointIndex>&) override;

private:
    void UpdatePoints(
        const MeshRefFacetToFacets&,
        const MeshRefPointToFacets&,
        const std::vector<PointIndex>&
    );

private:
    int weights {1};
};

}  // namespace MeshCore
