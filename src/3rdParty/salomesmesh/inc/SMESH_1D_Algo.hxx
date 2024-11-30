//  Copyright (C) 2007-2008  CEA/DEN, EDF R&D, OPEN CASCADE
//
//  Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
//  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
//  See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//  SMESH SMESH : implementation of SMESH idl descriptions
//  File   : SMESH_1D_Algo.hxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//  $Header: /home/server/cvs/SMESH/SMESH_SRC/src/SMESH/SMESH_1D_Algo.hxx,v 1.8.2.1 2008/11/27 12:25:15 abd Exp $
//
#ifndef _SMESH_1D_ALGO_HXX_
#define _SMESH_1D_ALGO_HXX_

#include "SMESH_SMESH.hxx"

#include "SMESH_Algo.hxx"

class SMESH_EXPORT SMESH_1D_Algo:
  public SMESH_Algo
{
public:
  SMESH_1D_Algo(int hypId, int studyId,  SMESH_Gen* gen);
  virtual ~SMESH_1D_Algo();
};

#endif
