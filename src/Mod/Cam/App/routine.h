/***************************************************************************
 *   Copyright (c) 2007 Joachim Zettler <Joachim.Zettler@gmx.de>           *
 *   Copyright (c) 2007 Mohamad Najib Muhammad Noor <najib_bean@yahoo.co.uk>
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


/*********ROUTINE.H**********/
#ifndef ROUTINE_H
#define ROUTINE_H

/******MAIN INCLUDE******/

#include <vector>

/********FREECAD MESH**********/

// Things from the Mesh module
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/Core/TopoAlgorithm.h>
#include <Mod/Mesh/App/Core/Iterator.h>
#include <Mod/Mesh/App/MeshPy.h>
#include <Mod/Mesh/App/Mesh.h>


/******NURBS STRUCT*********/
/*! \brief This Nurbs struct will be used internally without any connections to the outside
 program
*/
typedef struct
{
    std::vector<double> CntrlArray;
    std::vector<double> KnotU;
    std::vector<double> KnotV;
    int MaxKnotU;
    int MaxKnotV;
    int MaxU;
    int MaxV;
    int DegreeU;
    int DegreeV;
}NURBS;



/*! \brief Some Mathematical Routines

 This class will be inherited by the Approx class to reduce code bloat.
 This class have all the self-written matrix and vector routines,
 and some NURBS routines from the Nurbs Book or translated from the NURBS
 Toolbox for MATLAB
*/
class CamExport Routines
{
public:
    // mehrdimensionales Newton-Verfahren mit festem Startwert 0
    static std::vector<double> NewtonStep(std::vector<double> &F,std::vector<std::vector<double> > &DF);
protected:
    double TrapezoidIntergration(const std::vector<double> &WithRespectTo, const std::vector<double> &Intergral);
    //Matrix and vectors
    void CramerSolve(std::vector< std::vector<double> > &RHS1, std::vector<double>& RHS2, std::vector<double> &LHS);
    double CalcAngle(Base::Vector3f a, Base::Vector3f b, Base::Vector3f c);
    /*! Determinant of a 2x2 Matrix */
    inline double det2(std::vector< std::vector<double> > &Matrix)
    {
        return ((Matrix[0][0] * Matrix[1][1]) - (Matrix[0][1] * Matrix[1][0]));
    };

    double AreaTriangle(Base::Vector3f &a, Base::Vector3f &b, Base::Vector3f &c);

    //NURBS
    int FindSpan(int n, int p, double u, std::vector<double> KnotSequence);
    void DersBasisFuns(int i, double u, int p, int n,
                       std::vector<double> &KnotSequence, std::vector< std::vector<double> > &Derivate);
    void Basisfun(int i, double u, int p, std::vector<double> &KnotSequence, std::vector<double> &output);
    void NrbDEval(NURBS &MainNurb, std::vector<NURBS> &DerivNurb, std::vector<double> &Point,
                  std::vector<double> &EvalPoint, std::vector<std::vector<double> > &jac);
    void PointNrbEval(std::vector<double> &p, std::vector<double> &CurPoint, NURBS &MainNurb);
    void PointBSPEval(int DegreeU, std::vector< std::vector<double> > &Cntrl, std::vector<double> &KnotSequence,
                      std::vector< double > &Output, double CurEvalPoint);
    void PointNrbDerivate(NURBS MainNurb, std::vector<NURBS> &OutNurbs);
    void bspderiv(int d, std::vector< std::vector<double> > &c, std::vector<double> &k,
                  std::vector< std::vector<double> > &OutputC, std::vector<double> &Outputk);
    void ExtendKnot(double ErrPnt, int NurbDegree, int MaxCntrl, std::vector<double> &KnotSequence);
    void Extension(std::vector<double> &KnotSequence, int SmallerIndex, int LargerIndex, double SmallerIndNewValue,
                   double LargerIndNewValue);
    void GenerateUniformKnot(int MaxCntrl, int NurbDegree, std::vector<double> &KnotSequence);
    unsigned long FindCorner(float ParamX, float ParamY, std::vector<unsigned long> &v_neighbour, std::vector <Base::Vector3f> &v_pnts);
private:
    //Variables in Use by Routine TrapezoidIntegration
    double m_h, m_result;
    int m_N;


};

#endif /*ROUTINE_H DEFINED*/

