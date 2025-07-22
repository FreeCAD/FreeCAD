// Wild Magic Source Code
// David Eberly
// http://www.geometrictools.com
// Copyright (c) 1998-2007
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 2.1 of the License, or (at
// your option) any later version.  The license is available for reading at
// either of the locations:
//     http://www.gnu.org/copyleft/lgpl.html
//     http://www.geometrictools.com/License/WildMagicLicense.pdf
// The license applies to versions 0 through 4 of Wild Magic.
//
// Version: 4.0.1 (2006/07/23)

#include "Wm4FoundationPCH.h"
#include "Wm4MeshCurvature.h"

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
MeshCurvature<Real>::MeshCurvature (int iVQuantity,
    const Vector3<Real>* akVertex, int iTQuantity, const int* aiIndex)
{
    m_iVQuantity = iVQuantity;
    m_akVertex = akVertex;
    m_iTQuantity = iTQuantity;
    m_aiIndex = aiIndex;

    // compute normal vectors
    m_akNormal = WM4_NEW Vector3<Real>[m_iVQuantity];
    std::fill_n(m_akNormal,m_iVQuantity,Vector3<Real>{0,0,0});
    int i, iV0, iV1, iV2;
    for (i = 0; i < m_iTQuantity; i++)
    {
        // get vertex indices
        iV0 = *aiIndex++;
        iV1 = *aiIndex++;
        iV2 = *aiIndex++;

        // compute the normal (length provides a weighted sum)
        Vector3<Real> kEdge1 = m_akVertex[iV1] - m_akVertex[iV0];
        Vector3<Real> kEdge2 = m_akVertex[iV2] - m_akVertex[iV0];
        Vector3<Real> kNormal = kEdge1.Cross(kEdge2);

        m_akNormal[iV0] += kNormal;
        m_akNormal[iV1] += kNormal;
        m_akNormal[iV2] += kNormal;
    }
    for (i = 0; i < m_iVQuantity; i++)
    {
        m_akNormal[i].Normalize();
    }

    // compute the matrix of normal derivatives
    Matrix3<Real>* akDNormal = WM4_NEW Matrix3<Real>[m_iVQuantity];
    Matrix3<Real>* akWWTrn = WM4_NEW Matrix3<Real>[m_iVQuantity];
    Matrix3<Real>* akDWTrn = WM4_NEW Matrix3<Real>[m_iVQuantity];
    std::fill_n(akWWTrn,m_iVQuantity,Matrix3<Real>{0,0,0,0,0,0,0,0,0});
    std::fill_n(akDWTrn,m_iVQuantity,Matrix3<Real>{0,0,0,0,0,0,0,0,0});

    int iRow, iCol;
    aiIndex = m_aiIndex;
    for (i = 0; i < m_iTQuantity; i++)
    {
        // get vertex indices
        int aiV[3];
        aiV[0] = *aiIndex++;
        aiV[1] = *aiIndex++;
        aiV[2] = *aiIndex++;

        for (int j = 0; j < 3; j++)
        {
            iV0 = aiV[j];
            iV1 = aiV[(j+1)%3];
            iV2 = aiV[(j+2)%3];

            // Compute edge from V0 to V1, project to tangent plane of vertex,
            // and compute difference of adjacent normals.
            Vector3<Real> kE = m_akVertex[iV1] - m_akVertex[iV0];
            Vector3<Real> kW = kE - (kE.Dot(m_akNormal[iV0]))*m_akNormal[iV0];
            Vector3<Real> kD = m_akNormal[iV1] - m_akNormal[iV0];
            for (iRow = 0; iRow < 3; iRow++)
            {
                for (iCol = 0; iCol < 3; iCol++)
                {
                    akWWTrn[iV0][iRow][iCol] += kW[iRow]*kW[iCol];
                    akDWTrn[iV0][iRow][iCol] += kD[iRow]*kW[iCol];
                }
            }

            // Compute edge from V0 to V2, project to tangent plane of vertex,
            // and compute difference of adjacent normals.
            kE = m_akVertex[iV2] - m_akVertex[iV0];
            kW = kE - (kE.Dot(m_akNormal[iV0]))*m_akNormal[iV0];
            kD = m_akNormal[iV2] - m_akNormal[iV0];
            for (iRow = 0; iRow < 3; iRow++)
            {
                for (iCol = 0; iCol < 3; iCol++)
                {
                    akWWTrn[iV0][iRow][iCol] += kW[iRow]*kW[iCol];
                    akDWTrn[iV0][iRow][iCol] += kD[iRow]*kW[iCol];
                }
            }
        }
    }

    // Add in N*N^T to W*W^T for numerical stability.  In theory 0*0^T gets
    // added to D*W^T, but of course no update needed in the implementation.
    // Compute the matrix of normal derivatives.
    for (i = 0; i < m_iVQuantity; i++)
    {
        for (iRow = 0; iRow < 3; iRow++)
        {
            for (iCol = 0; iCol < 3; iCol++)
            {
                akWWTrn[i][iRow][iCol] = ((Real)0.5)*akWWTrn[i][iRow][iCol] +
                    m_akNormal[i][iRow]*m_akNormal[i][iCol];
                akDWTrn[i][iRow][iCol] *= (Real)0.5;
            }
        }

        akDNormal[i] = akDWTrn[i]*akWWTrn[i].Inverse();
    }

    WM4_DELETE[] akWWTrn;
    WM4_DELETE[] akDWTrn;

    // If N is a unit-length normal at a vertex, let U and V be unit-length
    // tangents so that {U, V, N} is an orthonormal set.  Define the matrix
    // J = [U | V], a 3-by-2 matrix whose columns are U and V.  Define J^T
    // to be the transpose of J, a 2-by-3 matrix.  Let dN/dX denote the
    // matrix of first-order derivatives of the normal vector field.  The
    // shape matrix is
    //   S = (J^T * J)^{-1} * J^T * dN/dX * J = J^T * dN/dX * J
    // where the superscript of -1 denotes the inverse.  (The formula allows
    // for J built from non-perpendicular vectors.) The matrix S is 2-by-2.
    // The principal curvatures are the eigenvalues of S.  If k is a principal
    // curvature and W is the 2-by-1 eigenvector corresponding to it, then
    // S*W = k*W (by definition).  The corresponding 3-by-1 tangent vector at
    // the vertex is called the principal direction for k, and is J*W.
    m_afMinCurvature = WM4_NEW Real[m_iVQuantity];
    m_afMaxCurvature = WM4_NEW Real[m_iVQuantity];
    m_akMinDirection = WM4_NEW Vector3<Real>[m_iVQuantity];
    m_akMaxDirection = WM4_NEW Vector3<Real>[m_iVQuantity];
    for (i = 0; i < m_iVQuantity; i++)
    {
        // compute U and V given N
        Vector3<Real> kU, kV;
        Vector3<Real>::GenerateComplementBasis(kU,kV,m_akNormal[i]);

        // Compute S = J^T * dN/dX * J.  In theory S is symmetric, but
        // because we have estimated dN/dX, we must slightly adjust our
        // calculations to make sure S is symmetric.
        Real fS01 = kU.Dot(akDNormal[i]*kV);
        Real fS10 = kV.Dot(akDNormal[i]*kU);
        Real fSAvr = ((Real)0.5)*(fS01+fS10);
        Matrix2<Real> kS
        (
            kU.Dot(akDNormal[i]*kU), fSAvr,
            fSAvr, kV.Dot(akDNormal[i]*kV)
        );

        // compute the eigenvalues of S (min and max curvatures)
        Real fTrace = kS[0][0] + kS[1][1];
        Real fDet = kS[0][0]*kS[1][1] - kS[0][1]*kS[1][0];
        Real fDiscr = fTrace*fTrace - ((Real)4.0)*fDet;
        Real fRootDiscr = Math<Real>::Sqrt(Math<Real>::FAbs(fDiscr));
        m_afMinCurvature[i] = ((Real)0.5)*(fTrace - fRootDiscr);
        m_afMaxCurvature[i] = ((Real)0.5)*(fTrace + fRootDiscr);

        // compute the eigenvectors of S
        Vector2<Real> kW0(kS[0][1],m_afMinCurvature[i]-kS[0][0]);
        Vector2<Real> kW1(m_afMinCurvature[i]-kS[1][1],kS[1][0]);
        if (kW0.SquaredLength() >= kW1.SquaredLength())
        {
            kW0.Normalize();
            m_akMinDirection[i] = kW0.X()*kU + kW0.Y()*kV;
        }
        else
        {
            kW1.Normalize();
            m_akMinDirection[i] = kW1.X()*kU + kW1.Y()*kV;
        }

        kW0 = Vector2<Real>(kS[0][1],m_afMaxCurvature[i]-kS[0][0]);
        kW1 = Vector2<Real>(m_afMaxCurvature[i]-kS[1][1],kS[1][0]);
        if (kW0.SquaredLength() >= kW1.SquaredLength())
        {
            kW0.Normalize();
            m_akMaxDirection[i] = kW0.X()*kU + kW0.Y()*kV;
        }
        else
        {
            kW1.Normalize();
            m_akMaxDirection[i] = kW1.X()*kU + kW1.Y()*kV;
        }
    }

    WM4_DELETE[] akDNormal;
}
//----------------------------------------------------------------------------
template <class Real>
MeshCurvature<Real>::~MeshCurvature ()
{
    WM4_DELETE[] m_akNormal;
    WM4_DELETE[] m_afMinCurvature;
    WM4_DELETE[] m_afMaxCurvature;
    WM4_DELETE[] m_akMinDirection;
    WM4_DELETE[] m_akMaxDirection;
}
//----------------------------------------------------------------------------
template <class Real>
int MeshCurvature<Real>::GetVQuantity () const
{
    return m_iVQuantity;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>* MeshCurvature<Real>::GetVertices () const
{
    return m_akVertex;
}
//----------------------------------------------------------------------------
template <class Real>
int MeshCurvature<Real>::GetTQuantity () const
{
    return m_iTQuantity;
}
//----------------------------------------------------------------------------
template <class Real>
const int* MeshCurvature<Real>::GetIndices () const
{
    return m_aiIndex;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>* MeshCurvature<Real>::GetNormals () const
{
    return m_akNormal;
}
//----------------------------------------------------------------------------
template <class Real>
const Real* MeshCurvature<Real>::GetMinCurvatures () const
{
    return m_afMinCurvature;
}
//----------------------------------------------------------------------------
template <class Real>
const Real* MeshCurvature<Real>::GetMaxCurvatures () const
{
    return m_afMaxCurvature;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>* MeshCurvature<Real>::GetMinDirections () const
{
    return m_akMinDirection;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>* MeshCurvature<Real>::GetMaxDirections () const
{
    return m_akMaxDirection;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
class MeshCurvature<float>;

template WM4_FOUNDATION_ITEM
class MeshCurvature<double>;
//----------------------------------------------------------------------------
}
