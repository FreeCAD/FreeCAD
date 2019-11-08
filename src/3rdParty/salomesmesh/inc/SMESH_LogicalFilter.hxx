// Copyright (C) 2007-2016  CEA/DEN, EDF R&D, OPEN CASCADE
//
// Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
// CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
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

//  File   : SMESH_LogicalFilter.hxx
//  Module : SMESH
//
#ifndef _SMESH_LogicalFilter_HeaderFile
#define _SMESH_LogicalFilter_HeaderFile

#include "SMESH_Type.h"

#include <SUIT_SelectionFilter.h>

#include <QList>

class SMESHFILTERSSELECTION_EXPORT SMESH_LogicalFilter : public SUIT_SelectionFilter
{
 public:
  enum { LO_OR, LO_AND, LO_NOT, LO_UNDEFINED };

 public:
  SMESH_LogicalFilter( const QList<SUIT_SelectionFilter*>&, const int, bool takeOwnership=false );
  virtual ~SMESH_LogicalFilter();

  virtual bool isOk (const SUIT_DataOwner*) const;

  void                                 setFilters (const QList<SUIT_SelectionFilter*>&);
  void                                 setOperation (const int);
  const QList<SUIT_SelectionFilter*>   getFilters() const;
  int                                  getOperation() const;

private:
  void                          deleteFilters();

private:
  QList<SUIT_SelectionFilter*>  myFilters;
  int                           myOperation;
  bool                          myOwnership;
};

#endif
