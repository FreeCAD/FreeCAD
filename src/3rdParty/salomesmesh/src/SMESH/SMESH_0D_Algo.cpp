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
//  SMESH SMESH : implementaion of SMESH idl descriptions
//  File   : SMESH_0D_Algo.cxx
//  Module : SMESH
//  $Header: /home/server/cvs/SMESH/SMESH_SRC/src/SMESH/SMESH_0D_Algo.cxx,v 1.2.2.1 2008/11/27 12:25:15 abd Exp $
//
#include "SMESH_0D_Algo.hxx"
#include "SMESH_Gen.hxx"

//=============================================================================
/*!
 *  
 */
//=============================================================================

SMESH_0D_Algo::SMESH_0D_Algo(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_Algo(hypId, studyId, gen)
{
  _type = ALGO_0D;
  gen->_map0D_Algo[hypId] = this;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

SMESH_0D_Algo::~SMESH_0D_Algo()
{
}

