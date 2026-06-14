// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
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

//  SMESH SMESHDS : management of mesh data and SMESH document
//  File   : SMESHDS_Hypothesis.hxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//
#ifndef _SMESHDS_HYPOTHESIS_HXX_
#define _SMESHDS_HYPOTHESIS_HXX_

#include "SMESH_SMESHDS.hxx"

#include <string>
#include <iostream>

class SMESHDS_EXPORT SMESHDS_Hypothesis
{
 public:
  SMESHDS_Hypothesis(int hypId);
  virtual ~SMESHDS_Hypothesis();

  enum hypothesis_type { PARAM_ALGO, ALGO_0D, ALGO_1D, ALGO_2D, ALGO_3D };

  const char* GetName() const;
  int         GetID()   const;
  int         GetType() const;

  virtual std::ostream & SaveTo(std::ostream & save)=0;
  virtual std::istream & LoadFrom(std::istream & load)=0;

  bool IsSameName( const SMESHDS_Hypothesis& other) const;
  virtual bool operator==(const SMESHDS_Hypothesis& other) const;
  bool operator!=(const SMESHDS_Hypothesis& other) const { return !(*this==other); }

 protected:
  std::string     _name;  // identifier of hypothesis type
  int             _hypId; // ID unique within application session
  hypothesis_type _type;  // enum hypothesis_type
};

#endif
