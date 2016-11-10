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

// Declarations needed for usage of DriverMED

#include "SMDSAbs_ElementType.hxx"
#include "SMESH_DriverMED.hxx"

#include <boost/shared_ptr.hpp>

class DriverMED_Family;
typedef boost::shared_ptr<DriverMED_Family> DriverMED_FamilyPtr;

namespace DriverMED
{
  // Implemetation is in DriverMED_W_Field.cxx

  /*
   * Returns MED element geom type (MED::EGeometrieElement) by SMDS type
   */
  MESHDRIVERMED_EXPORT int GetMedGeoType( SMDSAbs_EntityType smdsType );
  
  /*
   * Returns SMDS element geom type by MED type (MED::EGeometrieElement)
   */
  MESHDRIVERMED_EXPORT SMDSAbs_EntityType GetSMDSType( int medType );
}
