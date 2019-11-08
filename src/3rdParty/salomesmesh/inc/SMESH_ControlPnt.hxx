// Copyright (C) 2007-2016  CEA/DEN, EDF R&D
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//

// Author : Lioka RAZAFINDRAZAKA (CEA)

#ifndef SMESH_CONTROLPNT_H
#define SMESH_CONTROLPNT_H

#include "SMESH_Utils.hxx"

#include <gp_Pnt.hxx>

class TopoDS_Shape;
class TopoDS_Edge ;
class TopoDS_Face ;
class TopoDS_Solid;

#include <vector>

namespace SMESHUtils
{
  /*!
   * \brief Control point: coordinates and element size at these coordinates
   */
  struct SMESHUtils_EXPORT ControlPnt : public gp_Pnt
  {
    ControlPnt()
      : gp_Pnt(), size(0) {}
    ControlPnt( const gp_Pnt& aPnt, double theSize)
      : gp_Pnt( aPnt ), size( theSize ) {}
    ControlPnt(double theX,double theY,double theZ)
      : gp_Pnt(theX, theY, theZ), size(0) {}
    ControlPnt(double theX,double theY,double theZ, double theSize)
      : gp_Pnt(theX, theY, theZ), size( theSize ) {}

    double Size() const { return size; };
    void SetSize( double theSize ) { size = theSize; };

    double size;
  };

  // Functions to get sample point from shapes
  SMESHUtils_EXPORT void createControlPoints( const TopoDS_Shape&        theShape, 
                            const double&              theSize, 
                            std::vector< ControlPnt >& thePoints );

  SMESHUtils_EXPORT void createPointsSampleFromEdge( const TopoDS_Edge&       theEdge, 
                                   const double&            theSize, 
                                   std::vector<ControlPnt>& thePoints );

  SMESHUtils_EXPORT void createPointsSampleFromFace( const TopoDS_Face&       theFace, 
                                   const double&            theSize, 
                                   std::vector<ControlPnt>& thePoints );

  SMESHUtils_EXPORT void createPointsSampleFromSolid( const TopoDS_Solid&      theSolid, 
                                    const double&            theSize, 
                                    std::vector<ControlPnt>& thePoints );

}
#endif
