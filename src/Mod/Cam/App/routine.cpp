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

/*********ROUTINE.CPP*********/
#include "PreCompiled.h"
#include <cmath>
#include "routine.h"


///*********BINDINGS********/
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/bindings/traits/ublas_matrix.hpp>
#include <boost/numeric/bindings/atlas/clapack.hpp>

using namespace boost::numeric::bindings;
using namespace boost::numeric;



/*! \brief Numerical Integration according to trapezoid rules

 This routine assumes that the Intergral values are already corresponding with WithRespectTo
 i.e: Intergral[i] == Intergral(WithRespectTo[i]);
 Also, it is assumed that WithRespectTo are evenly spaced with it's stepWidth
*/
double Routines::TrapezoidIntergration(const std::vector<double> &WithRespectTo, const std::vector<double> &Integral)
{
    m_h = 0;
    m_result = 0;
    m_N = Integral.size();
    if (m_N==1)  // trapzf(x,y) returns zero for a scalar x
    {
        m_result = 0;
        return m_result;
    }

    m_h = WithRespectTo[1] - WithRespectTo[0]; /* the integration stepsize */
    for (int i=1; i< m_N - 1; i++)
        m_result = m_result + Integral[i];

    m_result = m_h/2 * (Integral[0] + 2*(m_result) + Integral[m_N -1]);
    return m_result;
}

std::vector<double> Routines::NewtonStep(std::vector<double> &F,std::vector<std::vector<double> > &DF)
{
    // löst folgendes Gleichungssystem: DF*x = F
    int siz = (int) F.size();
    std::vector<double> x_new(siz);
    std::vector<int> piv(siz);            // pivotelement

    ublas::matrix<double> A(siz, siz);
    ublas::matrix<double> b(1, siz);

    // füllt blas-matrizen
    for (unsigned int i=0; i<siz; ++i)
    {
        b(0,i) = -F[i];
        for (unsigned int j=0; j<siz; ++j)
        {
            A(i,j) = DF[i][j];
        }
    }

    /*cout << b(0,0) << "," << b(0,1) << "," << b(0,2) << endl;
    cout << A(0,0) << "," << A(0,1) << "," << A(0,2) << endl;
    cout << A(1,0) << "," << A(1,1) << "," << A(1,2) << endl;
    cout << A(2,0) << "," << A(2,1) << "," << A(2,2) << endl;*/


    atlas::lu_factor(A,piv);              // führt LU-Zerlegung durch
    atlas::getrs(A,piv,b);                // löst Gl.system A*x = b (b wird mit der Lösung überschrieben)

    for (unsigned int i=0; i<siz; ++i)
    {
        x_new[i] = b(0,i);
    }

    return x_new;
}


/*! \brief Cramer-Rule Solver

 Cramer-Rule solver for 2x2 Matrix.

 RHS1 * LHS = RHS2, LHS will be computed
*/
void Routines::CramerSolve(std::vector< std::vector<double> > &RHS1, std::vector<double>& RHS2, std::vector<double> &LHS)
{
    double MainMatrixDet = det2(RHS1);
    std::vector< std::vector<double> > Temp(2,std::vector<double>(2,0.0));
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++)
            Temp[i][j] = RHS1[i][j];

    //first pass
    for (int i = 0; i < 2; i++)
        Temp[i][0] = RHS2[i];

    LHS[0] = det2(Temp) / MainMatrixDet;

    //restore
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++)
            Temp[i][j] = RHS1[i][j];

    //second pass
    for (int i = 0; i < 2; i++)
        Temp[i][1] = RHS2[i];

    LHS[1] = det2(Temp) / MainMatrixDet;
}

/*! \brief Calculate angle between two vectors

 Dependencies: Vector3D definitions and routines
*/
double Routines::CalcAngle(Base::Vector3f a,Base::Vector3f  b,Base::Vector3f c)
{
    Base::Vector3f First = a - b;
    Base::Vector3f Second = c - b;
    Base::Vector3f Third = c - a;
    //double test1 = First.Length(), test2 = Second.Length(), test3 = Third.Length();
    double UpperTerm = (First.Length() * First.Length()) + (Second.Length() *Second.Length()) - (Third.Length() * Third.Length() );
    double LowerTerm = 2 * First.Length() * Second.Length() ;
    double ang = acos( UpperTerm / LowerTerm );
    return ang;
}





/*! \brief Algorithm A2.1 from NURBS Book Page 68*/
int Routines::FindSpan(int n, int p, double u, std::vector<double> KnotSequence)
{
    if (u == KnotSequence[n+1])
        return n;
    int low, high, mid, i;
    i = 0;
    low = p;
    high = n+1;
    mid = (low+high) / 2;
    while (u < KnotSequence[mid] || u >= KnotSequence[mid+1])
    {
        if (u < KnotSequence[mid])
            high = mid;
        else
            low = mid;
        mid = (low + high) / 2;
        i++;
    }

    return mid;

}


/*! \brief Algorithm A2.4 from NURBS Book page 70*/
void Routines::Basisfun(int i, double u, int p, std::vector<double> &KnotSequence, std::vector<double> &output)
{
    double temp, saved;
    std::vector<double> leftie(p+1, 0.0);
    std::vector<double> rightie(p+1, 0.0);
    output[0] = 1.0;
    for (int j = 1; j <= p; j++)
    {
        leftie[j] = u - KnotSequence[i+1-j];
        rightie[j] = KnotSequence[i+j] - u;
        saved = 0.0;
        for (int r = 0; r < j; r++)
        {
            temp = output[r] / (rightie[r+1] + leftie[j-r]);
            output[r] = saved + (rightie[r+1]*temp);
            saved = leftie[j-r]*temp;
        }
        output[j] = saved;
    }
}
/*! \brief Algorithm A2.3 from NURBS Book Page 72 */
void Routines::DersBasisFuns(int i, double u, int p, int n,
                             std::vector<double> &KnotSequence, std::vector< std::vector<double> > &Derivate)
{
    std::vector< std::vector<double> > ndu(p+1, std::vector<double>(p+1,0.0));
    std::vector< std::vector<double> > a(2, std::vector<double>(p+1,0.0));
    std::vector<double> leftie(p+1, 0.0);
    std::vector<double> rightie(p+1, 0.0);
    ndu[0][0] = 1.0;
    for (int j = 1; j <= p; j++)
    {
        leftie[j] = u - KnotSequence[i+1-j];
        rightie[j] = KnotSequence[i+j] - u;
        double saved = 0.0;
        for (int r = 0; r < j; r++)
        {
            ndu[j][r] = rightie[r+1] + leftie[j-r];
            double temp = ndu[r][j-1] / ndu[j][r];

            ndu[r][j] = saved + rightie[r+1]*temp;
            saved = leftie[j-r]*temp;
        }
        ndu[j][j] = saved;
    }
    for (int j = 0; j <= p; j++)
        Derivate[0][j] = ndu[j][p];

    for (int r = 0; r <= p; r++)
    {
        int j1, j2;
        int s1 = 0;
        int s2 = 1;
        a[0][0] = 1.0;
        for (int k = 1; k <= n; k++)
        {
            double d = 0.0;
            int j;
            int rk = r - k;
            int pk = p - k;
            if (r >= k)
            {
                a[s2][0] = a[s1][0] / ndu[pk+1][rk];
                d += a[s2][0] * ndu[rk][pk];
            }

            if (rk >= -1)
                j1 = 1;
            else j1 = -rk;

            if (r -1 <= pk)
                j2 = k -1;
            else j2 = p - r;

            for (j = j1; j <= j2; j++)
            {
                a[s2][j] = (a[s1][j]-a[s1][j-1]) / ndu[pk+1][rk+j];
                d += a[s2][j] *  ndu[rk+j][pk];
            }

            if (r <= pk)
            {
                a[s2][k] = -a[s1][k-1] / ndu[pk+1][r];
                d += a[s2][k] * ndu[r][pk];
            }

            Derivate[k][r] = d;
            j = s1;
            s1 = s2;
            s2 = j;


        }
    }
    int r = p;
    for (int k = 1; k <= n; k++)
    {
        for (int j = 0; j <= p; j++)
            Derivate[k][j] *= r;

        r*= (p-k);
    }


}
/*! \brief Translation from nrbeval.m from NURBS Toolbox. WARNING: ONLY POINT EVALUATION ARE CONSIDERED!!!! */
void Routines::PointNrbEval(std::vector<double> &p, std::vector<double> &CurPoint,  NURBS &MainNurb)
{
    if (!p.empty())
        p.clear();
    int i = 0, j = 0;
    //val = reshape(nurbs.coefs,4*num1,num2);
    std::vector< std::vector<double> > CntrlPoints(((MainNurb.MaxU+1)*3), std::vector<double>((MainNurb.MaxV+1), 0.0));
    for (unsigned int a = 0; a < MainNurb.CntrlArray.size(); )
    {
        CntrlPoints[i][j] = MainNurb.CntrlArray[a];
        CntrlPoints[i+1][j] = MainNurb.CntrlArray[a+1];
        CntrlPoints[i+2][j] = MainNurb.CntrlArray[a+2];
        i += 3;
        a += 3;
        if (i == ((MainNurb.MaxU+1)*3))
        {
            i = 0;
            j++;
        }

    }
    //v direction...?
    i = 0;
    std::vector<double> Output(CntrlPoints.size(), 0.0);
    PointBSPEval(MainNurb.DegreeV, CntrlPoints, MainNurb.KnotV, Output, CurPoint[1]);
    std::vector< std::vector<double> > RedoneOutput(3, std::vector<double>(MainNurb.MaxU+1, 0.0));
    for (unsigned int a = 0; a < Output.size(); )
    {
        RedoneOutput[0][i] = Output[a];
        RedoneOutput[1][i] = Output[a+1];
        RedoneOutput[2][i] = Output[a+2];
        a += 3;
        i++;
    }
    std::vector<double> OutVect(4,0.0);
    PointBSPEval(MainNurb.DegreeU, RedoneOutput, MainNurb.KnotU, OutVect,CurPoint[0]);
    p.push_back(OutVect[0]);
    p.push_back(OutVect[1]);
    p.push_back(OutVect[2]);


}

/*! \brief Translation from bspeval.m from NURBS Toolbox. WARNING: ONLY POINT EVALUATION ARE CONSIDERED!!!! */
void Routines::PointBSPEval(int Degree, std::vector< std::vector<double> > &Cntrl, std::vector<double> &KnotSequence,
                            std::vector< double > &Output, double CurEvalPoint)
{
    std::vector<double> Basis(Degree+1, 0.0);
    int s = FindSpan(Cntrl[0].size() - 1,Degree,CurEvalPoint,KnotSequence);
    Basisfun(s,CurEvalPoint,Degree,KnotSequence,Basis);
    int tmp1 = s - Degree;
    for (unsigned int row = 0;row < Cntrl.size();row++)
    {
        double tmp2 = 0.0;
        for (int i = 0; i <= Degree; i++)
            tmp2 += Basis[i] *Cntrl[row][tmp1+i];
        Output[row] = tmp2;
    }

}
/*! \brief Translation from nrbderivate.m from NURBS Toolbox. WARNING: ONLY POINT EVALUATION ARE CONSIDERED!!!! */
void Routines::PointNrbDerivate(NURBS MainNurb, std::vector<NURBS> &OutNurbs)
{
    if (!OutNurbs.empty())
        OutNurbs.clear();
    NURBS Temp;
    int i = 0, j = 0, k = 0;
    std::vector< std::vector<double> > ControlDerivate;
    std::vector<double> KnotDerivate;
    std::vector< std::vector<double> > CntrlPoints(((MainNurb.MaxV+1)*3), std::vector<double>((MainNurb.MaxU+1), 0.0));
    for (unsigned int a = 0; a < MainNurb.CntrlArray.size(); )
    {
        CntrlPoints[i][j] = MainNurb.CntrlArray[a];
        CntrlPoints[i+1][j] = MainNurb.CntrlArray[a+1];
        CntrlPoints[i+2][j] = MainNurb.CntrlArray[a+2];
        j++;
        a += 3;
        i = k*3;
        if (j == (MainNurb.MaxU+1))
        {
            j = 0;
            k++;
            i = k*3;
        }

    }

    //U derivate
    bspderiv(MainNurb.DegreeU, CntrlPoints, MainNurb.KnotU, ControlDerivate, KnotDerivate);
    Temp.CntrlArray.resize(ControlDerivate.size() * ControlDerivate[0].size());
    i = 0, j = 0, k = 0;
    for (unsigned int a = 0; a < Temp.CntrlArray.size(); )
    {
        Temp.CntrlArray[a] = ControlDerivate[i][j];
        Temp.CntrlArray[a+1] = ControlDerivate[i+1][j];
        Temp.CntrlArray[a+2] = ControlDerivate[i+2][j];
        j++;
        a += 3;
        i = k*3;
        if (j == (int)ControlDerivate[i].size())
        {
            j = 0;
            k++;
            i = k*3;
        }
    }
    Temp.KnotU = KnotDerivate;
    Temp.KnotV = MainNurb.KnotV;
    Temp.MaxKnotU = Temp.KnotU.size();
    Temp.MaxKnotV = Temp.KnotV.size();
    Temp.MaxU = ControlDerivate[0].size();
    Temp.MaxV = ControlDerivate.size() / 3;
    Temp.DegreeU = Temp.MaxKnotU - Temp.MaxU - 1;
    Temp.DegreeV = Temp.MaxKnotV - Temp.MaxV - 1;
    Temp.MaxU--;
    Temp.MaxV--;
    OutNurbs.push_back(Temp);
    //reset
    i = 0, j = 0, k = 0;
    CntrlPoints.clear();
    ControlDerivate.clear();
    KnotDerivate.clear();
    CntrlPoints.resize((MainNurb.MaxU+1)*3);
    for (unsigned int a = 0; a < CntrlPoints.size(); a++)
        CntrlPoints[a].resize(MainNurb.MaxV+1);

    for (unsigned int a = 0; a < MainNurb.CntrlArray.size(); )
    {
        CntrlPoints[i][j] = MainNurb.CntrlArray[a];
        CntrlPoints[i+1][j] = MainNurb.CntrlArray[a+1];
        CntrlPoints[i+2][j] = MainNurb.CntrlArray[a+2];
        i += 3;
        a += 3;
        if (i == ((MainNurb.MaxU+1)*3))
        {
            i = 0;
            j++;
        }

    }
    //V derivate
    bspderiv(MainNurb.DegreeV, CntrlPoints, MainNurb.KnotV, ControlDerivate, KnotDerivate);
    Temp.CntrlArray.resize(ControlDerivate.size() * ControlDerivate[0].size());
    i = 0, j = 0, k = 0;
    for (unsigned int a = 0; a < Temp.CntrlArray.size(); )
    {
        Temp.CntrlArray[a] = ControlDerivate[i][j];
        Temp.CntrlArray[a+1] = ControlDerivate[i+1][j];
        Temp.CntrlArray[a+2] = ControlDerivate[i+2][j];
        a += 3;
        i += 3;
        if (i == (int)ControlDerivate.size())
        {
            j++;
            i = 0;
        }
    }
    Temp.KnotU = MainNurb.KnotU;
    Temp.KnotV = KnotDerivate;
    Temp.MaxKnotU = Temp.KnotU.size();
    Temp.MaxKnotV = Temp.KnotV.size();
    Temp.MaxU = ControlDerivate.size() / 3;
    Temp.MaxV = ControlDerivate[0].size();
    Temp.DegreeU = Temp.MaxKnotU - Temp.MaxU - 1;
    Temp.DegreeV = Temp.MaxKnotV - Temp.MaxV - 1;
    Temp.MaxU--;
    Temp.MaxV--;
    OutNurbs.push_back(Temp);
}

/*! \brief Translation from bspderiv.m from NURBS Toolbox. WARNING: ONLY POINT EVALUATION ARE CONSIDERED!!!! */
void Routines::bspderiv(int d, std::vector< std::vector<double> > &c, std::vector<double> &k,
                        std::vector< std::vector<double> > &OutputC, std::vector<double> &Outputk)
{
    OutputC.resize(c.size());
    for (unsigned int i = 0; i < OutputC.size();  i++)
        OutputC[i].resize(c[i].size()-1);

    double tmp = 0.0;

    for (unsigned int i = 0; i < OutputC[0].size(); i++)
    {
        tmp = d / (k[i+d+1]-k[i+1]);
        for (unsigned int j = 0; j < OutputC.size(); j++)
            OutputC[j][i] = tmp * (c[j][i+1] -  c[j][i]);
    }
    Outputk.resize(k.size() - 2);
    for (unsigned int i = 0; i < Outputk.size(); i++)
        Outputk[i] = k[i+1];
}

/*! \brief Translation from nrbdeval.m from NURBS Toolbox. WARNING: ONLY POINT EVALUATION ARE CONSIDERED!!!! */
void Routines::NrbDEval(NURBS &MainNurb, std::vector<NURBS> &DerivNurb, std::vector<double> &Point,
                        std::vector<double> &EvalPoint, std::vector<std::vector<double> > &jac)
{
    if (!EvalPoint.empty())
        EvalPoint.clear();
    if (!jac.empty())
        jac.clear();
    std::vector<double> TempoPointDeriv;
    PointNrbEval(EvalPoint, Point, MainNurb);
    for (unsigned int i = 0; i < DerivNurb.size(); i++)
    {
        PointNrbEval(TempoPointDeriv, Point, DerivNurb[i]);
        jac.push_back(TempoPointDeriv);
        TempoPointDeriv.clear();
    }
}

/*! \brief Uniform Knot Generator */
void Routines::GenerateUniformKnot(int MaxCntrl, int NurbDegree, std::vector<double> &KnotSequence)
{
    if (!KnotSequence.empty())
        KnotSequence.clear();

    for (int i = 0; i < (MaxCntrl + NurbDegree + 2); i++)
        KnotSequence.push_back(0);

    for (int i = NurbDegree; i < MaxCntrl; i++)
        KnotSequence[i+1] = (double)(i - NurbDegree + 1.0) / (double)(MaxCntrl - NurbDegree + 1.0);

    for (unsigned int i = MaxCntrl+1; i < KnotSequence.size(); i++)
        KnotSequence[i] = 1;

}

/*! \brief Find the corner points

 Idea: From the neighbour list, find the shortest distance of a point to the given parameter X and Y.
 Parameter X and Y are the bounding boxes parameter of the mesh (combinations of Min and Max of X and Y)
*/
unsigned long Routines::FindCorner(float ParamX, float ParamY, std::vector<unsigned long> &v_neighbour, std::vector <Base::Vector3f> &v_pnts)
{
    unsigned int j = 0;
    bool change = false;
    //double distance = sqrt(((ParamX - ParameterMesh.GetPoint(v_neighbour[0])[0])*(ParamX - ParameterMesh.GetPoint(v_neighbour[0])[0])) +
    // ((ParamY - ParameterMesh.GetPoint(v_neighbour[0])[1])*(ParamY - ParameterMesh.GetPoint(v_neighbour[0])[1])));
    double distance = sqrt(((ParamX - v_pnts[0][0])*(ParamX -  v_pnts[0][0])) +
                           ((ParamY -  v_pnts[0][1])*(ParamY -  v_pnts[0][1])));
    for (unsigned int k = 1; k < v_neighbour.size(); ++k)
    {
        double eps= sqrt(((ParamX -  v_pnts[k][0])*(ParamX -  v_pnts[k][0])) +
                         ((ParamY -  v_pnts[k][1])*(ParamY -  v_pnts[k][1])));
        if (eps < distance)
        {
            distance = eps;
            j = k;
            change = true;
        }

    }
    if (change)
        return v_neighbour[j];
    else return v_neighbour[0];
}

/*! \brief Calculate the Area of the triangle, bounded by three vectors from origin. A (bad) diagram is included in source
     code */
double Routines::AreaTriangle(Base::Vector3f &a, Base::Vector3f &b, Base::Vector3f &c)
{
    /*   a
    *    x----->x  b
    *     \    /
    *      \  /
    *       \/
    *       V
    *  x c
    */
    Base::Vector3f First = b - a;
    Base::Vector3f Second = c - a;
    std::vector < std::vector<double> > Matrix(2, std::vector<double>(2));
    Matrix[0][0] = First.x;
    Matrix[1][0] = First.y;
    Matrix[0][1] = Second.x;
    Matrix[1][1] = Second.y;
    return (0.5 * det2(Matrix));
}

/*! \brief Knot Extension

 This method will extend the knots by 2. ErrPnt will tell where the knots should be inserted, NurbDegree
 and MaxCntrl will make sure that the amount of 0's and 1's are left untouched.

 The new values will be calculated here and will be inserted in Extension function
*/
void Routines::ExtendKnot(double ErrPnt, int NurbDegree, int MaxCntrl, std::vector<double> &KnotSequence)
{
    double tol_knot = 0.02;
    int ind_1 = 0;
    double new1 = 0;
    int ind_2 = 0;
    double new2 = 0;
    bool flag = false;

    if (KnotSequence.size()>40)
        tol_knot = 0.015;

    for (unsigned int i = NurbDegree; i < (KnotSequence.size() - NurbDegree); i++)
    {
        if ((KnotSequence[i] > ErrPnt) && (ErrPnt >= KnotSequence[i-1]))
        {
            ind_1 = i;
            new1 = (KnotSequence[ind_1] + KnotSequence[ind_1-1]) / 2;

        }
    }
    //START CHANGE
    if (!flag)
    {
        double mid = (double)KnotSequence.size() / 2.0;

        if (ind_1 == mid)
        {

            KnotSequence.resize(KnotSequence.size() + 2);
            //MainNurb.MaxKnotU += 2;
            for (int i = KnotSequence.size() - 1; i > ind_1; i--)
                KnotSequence[i] = KnotSequence[i-2];
            KnotSequence[ind_1] = KnotSequence[ind_1-1] + ((new1 - KnotSequence[ind_1-1]) / 2.0);
            KnotSequence[ind_1+1] = new1 +((new1 - KnotSequence[ind_1-1]) / 2.0);

        }
        else
        {
            double temp = mid - ((double)ind_1);
            ind_2 = (int)(mid + temp);
            new2 = (KnotSequence[ind_2-1] + KnotSequence[ind_2]) / 2.0;
            if (ind_1 < ind_2)
                Extension(KnotSequence,ind_1,ind_2,new1,new2);
            else
                Extension(KnotSequence,ind_2,ind_1,new2,new1);
        }
    }
    else
    {
        if (ind_1 < ind_2)
            Extension(KnotSequence,ind_1,ind_2,new1,new2);
        else
            Extension(KnotSequence,ind_2,ind_1,new2,new1);
    }
}
/*! \brief This routine is because it is done over and over again...
*/
void Routines::Extension(std::vector<double> &KnotSequence, int SmallerIndex, int LargerIndex, double SmallerIndNewValue,
                         double LargerIndNewValue)
{
    KnotSequence.resize(KnotSequence.size() + 1);

    for (int i = KnotSequence.size() - 1; i > SmallerIndex; i--)
        KnotSequence[i] = KnotSequence[i-1];

    KnotSequence[SmallerIndex] = SmallerIndNewValue;

    LargerIndex++;
    KnotSequence.resize(KnotSequence.size() + 1);

    for (int i = KnotSequence.size() - 1; i > LargerIndex; i--)
        KnotSequence[i] = KnotSequence[i-1];

    KnotSequence[LargerIndex] = LargerIndNewValue;
}

