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
// Version: 4.0.0 (2006/06/28)

#ifndef WM4DELPOLYGONEDGE_H
#define WM4DELPOLYGONEDGE_H

#include "Wm4FoundationLIB.h"
#include "Wm4VEManifoldMesh.h"
#include "Wm4DelTriangle.h"

namespace Wm4
{

template <class Real>
class WM4_FOUNDATION_ITEM DelPolygonEdge : public VEManifoldMesh::Edge
{
public:
    DelPolygonEdge (int iV0 = -1, int iV1 = -1, int iNullIndex = -1,
            DelTriangle<Real>* pkTri = 0);

    static VEManifoldMesh::EPtr ECreator (int iV0, int iV1);

    int NullIndex;
    DelTriangle<Real>* Tri;
};

typedef DelPolygonEdge<float> DelPolygonEdgef;
typedef DelPolygonEdge<double> DelPolygonEdged;

}

#endif
