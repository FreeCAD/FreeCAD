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


#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include "Smoothing.h"
#include "MeshKernel.h"
#include "Algorithm.h"
#include "Elements.h"
#include "Iterator.h"
#include "Approximation.h"


using namespace MeshCore;


AbstractSmoothing::AbstractSmoothing(MeshKernel& m)
  : kernel(m)
  , tolerance(0)
  , component(Normal)
  , continuity(C0)
{
}

AbstractSmoothing::~AbstractSmoothing()
{
}

void AbstractSmoothing::initialize(Component comp, Continuity cont)
{
    this->component = comp;
    this->continuity = cont;
}

PlaneFitSmoothing::PlaneFitSmoothing(MeshKernel& m)
  : AbstractSmoothing(m)
{
}

PlaneFitSmoothing::~PlaneFitSmoothing()
{
}

void PlaneFitSmoothing::Smooth(unsigned int iterations)
{
    MeshCore::MeshPoint center;
    MeshCore::MeshPointArray PointArray = kernel.GetPoints();

    MeshCore::MeshPointIterator v_it(kernel);
    MeshCore::MeshRefPointToPoints vv_it(kernel);
    MeshCore::MeshPointArray::_TConstIterator v_beg = kernel.GetPoints().begin();

    for (unsigned int i=0; i<iterations; i++) {
        Base::Vector3f N, L;
        for (v_it.Begin(); v_it.More(); v_it.Next()) {
            MeshCore::PlaneFit pf;
            pf.AddPoint(*v_it);
            center = *v_it;
            const std::set<unsigned long>& cv = vv_it[v_it.Position()];
            if (cv.size() < 3)
                continue;

            std::set<unsigned long>::const_iterator cv_it;
            for (cv_it = cv.begin(); cv_it !=cv.end(); ++cv_it) {
                pf.AddPoint(v_beg[*cv_it]);
                center += v_beg[*cv_it];
            }

            float scale = 1.0f/((float)cv.size()+1.0f);
            center.Scale(scale,scale,scale);

            // get the mean plane of the current vertex with the surrounding vertices
            pf.Fit();
            N = pf.GetNormal();
            N.Normalize();

            // look in which direction we should move the vertex
            L.Set(v_it->x - center.x, v_it->y - center.y, v_it->z - center.z);
            if (N*L < 0.0)
                N.Scale(-1.0, -1.0, -1.0);

            // maximum value to move is distance to mean plane
            float d = std::min<float>((float)fabs(this->tolerance),(float)fabs(N*L));
            N.Scale(d,d,d);

            PointArray[v_it.Position()].Set(v_it->x - N.x, v_it->y - N.y, v_it->z - N.z);
        }

        // assign values without affecting iterators
        unsigned long count = kernel.CountPoints();
        for (unsigned long idx = 0; idx < count; idx++) {
            kernel.SetPoint(idx, PointArray[idx]);
        }
    }
}

void PlaneFitSmoothing::SmoothPoints(unsigned int iterations, const std::vector<unsigned long>& point_indices)
{
    MeshCore::MeshPoint center;
    MeshCore::MeshPointArray PointArray = kernel.GetPoints();

    MeshCore::MeshPointIterator v_it(kernel);
    MeshCore::MeshRefPointToPoints vv_it(kernel);
    MeshCore::MeshPointArray::_TConstIterator v_beg = kernel.GetPoints().begin();

    for (unsigned int i=0; i<iterations; i++) {
        Base::Vector3f N, L;
        for (std::vector<unsigned long>::const_iterator it = point_indices.begin(); it != point_indices.end(); ++it) {
            v_it.Set(*it);
            MeshCore::PlaneFit pf;
            pf.AddPoint(*v_it);
            center = *v_it;
            const std::set<unsigned long>& cv = vv_it[v_it.Position()];
            if (cv.size() < 3)
                continue;

            std::set<unsigned long>::const_iterator cv_it;
            for (cv_it = cv.begin(); cv_it !=cv.end(); ++cv_it) {
                pf.AddPoint(v_beg[*cv_it]);
                center += v_beg[*cv_it];
            }

            float scale = 1.0f/((float)cv.size()+1.0f);
            center.Scale(scale,scale,scale);

            // get the mean plane of the current vertex with the surrounding vertices
            pf.Fit();
            N = pf.GetNormal();
            N.Normalize();

            // look in which direction we should move the vertex
            L.Set(v_it->x - center.x, v_it->y - center.y, v_it->z - center.z);
            if (N*L < 0.0)
                N.Scale(-1.0, -1.0, -1.0);

            // maximum value to move is distance to mean plane
            float d = std::min<float>((float)fabs(this->tolerance),(float)fabs(N*L));
            N.Scale(d,d,d);

            PointArray[v_it.Position()].Set(v_it->x - N.x, v_it->y - N.y, v_it->z - N.z);
        }

        // assign values without affecting iterators
        unsigned long count = kernel.CountPoints();
        for (unsigned long idx = 0; idx < count; idx++) {
            kernel.SetPoint(idx, PointArray[idx]);
        }
    }
}

LaplaceSmoothing::LaplaceSmoothing(MeshKernel& m)
  : AbstractSmoothing(m), lambda(0.6307)
{
}

LaplaceSmoothing::~LaplaceSmoothing()
{
}

void LaplaceSmoothing::Umbrella(const MeshRefPointToPoints& vv_it,
                                const MeshRefPointToFacets& vf_it, double stepsize)
{
    const MeshCore::MeshPointArray& points = kernel.GetPoints();
    MeshCore::MeshPointArray::_TConstIterator v_it,
    v_beg = points.begin(), v_end = points.end();

    unsigned long pos = 0;
    for (v_it = points.begin(); v_it != v_end; ++v_it,++pos) {
        const std::set<unsigned long>& cv = vv_it[pos];
        if (cv.size() < 3)
            continue;
        if (cv.size() != vf_it[pos].size()) {
            // do nothing for border points
            continue;
        }

        unsigned int n_count = cv.size();
        double w;
        w=1.0/double(n_count);

        double delx=0.0,dely=0.0,delz=0.0;
        std::set<unsigned long>::const_iterator cv_it;
        for (cv_it = cv.begin(); cv_it !=cv.end(); ++cv_it) {
            delx += w*((v_beg[*cv_it]).x-v_it->x);
            dely += w*((v_beg[*cv_it]).y-v_it->y);
            delz += w*((v_beg[*cv_it]).z-v_it->z);
        }

        float x = (float)(v_it->x+stepsize*delx);
        float y = (float)(v_it->y+stepsize*dely);
        float z = (float)(v_it->z+stepsize*delz);
        kernel.SetPoint(pos,x,y,z);
    }
}

void LaplaceSmoothing::Umbrella(const MeshRefPointToPoints& vv_it,
                                const MeshRefPointToFacets& vf_it, double stepsize,
                                const std::vector<unsigned long>& point_indices)
{
    const MeshCore::MeshPointArray& points = kernel.GetPoints();
    MeshCore::MeshPointArray::_TConstIterator v_beg = points.begin();

    for (std::vector<unsigned long>::const_iterator pos = point_indices.begin(); pos != point_indices.end(); ++pos) {
        const std::set<unsigned long>& cv = vv_it[*pos];
        if (cv.size() < 3)
            continue;
        if (cv.size() != vf_it[*pos].size()) {
            // do nothing for border points
            continue;
        }

        unsigned int n_count = cv.size();
        double w;
        w=1.0/double(n_count);

        double delx=0.0,dely=0.0,delz=0.0;
        std::set<unsigned long>::const_iterator cv_it;
        for (cv_it = cv.begin(); cv_it !=cv.end(); ++cv_it) {
            delx += w*((v_beg[*cv_it]).x-(v_beg[*pos]).x);
            dely += w*((v_beg[*cv_it]).y-(v_beg[*pos]).y);
            delz += w*((v_beg[*cv_it]).z-(v_beg[*pos]).z);
        }

        float x = (float)((v_beg[*pos]).x+stepsize*delx);
        float y = (float)((v_beg[*pos]).y+stepsize*dely);
        float z = (float)((v_beg[*pos]).z+stepsize*delz);
        kernel.SetPoint(*pos,x,y,z);
    }
}

void LaplaceSmoothing::Smooth(unsigned int iterations)
{
    MeshCore::MeshRefPointToPoints vv_it(kernel);
    MeshCore::MeshRefPointToFacets vf_it(kernel);

    for (unsigned int i=0; i<iterations; i++) {
        Umbrella(vv_it, vf_it, lambda);
    }
}

void LaplaceSmoothing::SmoothPoints(unsigned int iterations, const std::vector<unsigned long>& point_indices)
{
    MeshCore::MeshRefPointToPoints vv_it(kernel);
    MeshCore::MeshRefPointToFacets vf_it(kernel);

    for (unsigned int i=0; i<iterations; i++) {
        Umbrella(vv_it, vf_it, lambda, point_indices);
    }
}

TaubinSmoothing::TaubinSmoothing(MeshKernel& m)
  : LaplaceSmoothing(m), micro(0.0424)
{
}

TaubinSmoothing::~TaubinSmoothing()
{
}

void TaubinSmoothing::Smooth(unsigned int iterations)
{
    MeshCore::MeshRefPointToPoints vv_it(kernel);
    MeshCore::MeshRefPointToFacets vf_it(kernel);

    // Theoretically Taubin does not shrink the surface
    iterations = (iterations+1)/2; // two steps per iteration
    for (unsigned int i=0; i<iterations; i++) {
        Umbrella(vv_it, vf_it, lambda);
        Umbrella(vv_it, vf_it, -(lambda+micro));
    }
}

void TaubinSmoothing::SmoothPoints(unsigned int iterations, const std::vector<unsigned long>& point_indices)
{
    MeshCore::MeshRefPointToPoints vv_it(kernel);
    MeshCore::MeshRefPointToFacets vf_it(kernel);

    // Theoretically Taubin does not shrink the surface
    iterations = (iterations+1)/2; // two steps per iteration
    for (unsigned int i=0; i<iterations; i++) {
        Umbrella(vv_it, vf_it, lambda, point_indices);
        Umbrella(vv_it, vf_it, -(lambda+micro), point_indices);
    }
}
