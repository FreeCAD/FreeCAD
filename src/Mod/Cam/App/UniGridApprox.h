/***************************************************************************
 *   Copyright (c) 2007 Joachim Zettler <Joachim.Zettler@gmx.de>           *
 *   Copyright (c) 2007 Human Rezai <human@mytum.de>                       *
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

#ifndef UNIGRIDAPPROX_H
#define UNIGRIDAPPROX_H

#include "routine.h"
#include <boost/numeric/ublas/matrix.hpp>
#include <GeomAdaptor_Surface.hxx>
#include <TopoDS_Face.hxx>

using namespace boost::numeric;


class CamExport UniGridApprox: public Routines
{
public:
    UniGridApprox(const MeshCore::MeshKernel &InputMesh, double Tol);
    ~UniGridApprox();

    bool Perform(double TOL);
    bool MeshOffset();
    bool SurfMeshParam();
    bool CompKnots(int m, int n);
    bool MatComp(int m, int n);
    bool BuildSurf();
    double CompGridError();
    double CompMeshError();
    bool WriteMatrix(ublas::matrix<double> M);

    MeshCore::MeshKernel m_Mesh;
    GeomAdaptor_Surface aAdaptorSurface;
    double m_offset;

    std::vector< std::vector<Base::Vector3f> > m_Grid;
    std::vector< std::vector<Base::Vector3f> > m_GridCopy;
    std::vector<double> m_err;
    std::vector< std::vector<double> > mG_err;
    ublas::matrix<double> Q;   //Data-Matrix
    ublas::matrix<double> CPx;
    ublas::matrix<double> CPy;
    ublas::matrix<double> CPz;
    std::vector<double> m_uParam;
    std::vector<double> m_vParam;
    std::vector<double> m_uknots;
    std::vector<double> m_vknots;
    int uCP, vCP;
    int m_um;
    int m_vm;
    int m_udeg;
    int m_vdeg;
    int n_x;
    int n_y;
    double m_Tol;

    TopoDS_Face m_Face;

protected:

};

#endif