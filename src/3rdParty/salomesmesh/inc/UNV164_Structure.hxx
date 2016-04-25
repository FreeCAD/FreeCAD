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

#ifndef UNV164_Structure_HeaderFile
#define UNV164_Structure_HeaderFile

// Universal Dataset Number: 164
// Name:   Units
// Status: Current
// Owner:  General
// Revision Date: 19-AUG-1987
// -----------------------------------------------------------------------

// Record 1:       FORMAT(I10,20A1,I10)
//                 Field 1      -- units code
//                                 = 1 - SI: Meter (newton)
//                                 = 2 - BG: Foot (pound f)
//                                 = 3 - MG: Meter (kilogram f)
//                                 = 4 - BA: Foot (poundal)
//                                 = 5 - MM: mm (milli newton)
//                                 = 6 - CM: cm (centi newton)
//                                 = 7 - IN: Inch (pound f)
//                                 = 8 - GM: mm (kilogram f)
//                                 = 9 - US: USER_DEFINED
//                                 = 10- MN: mm (newton)
//                 Field 2      -- units description (used for
//                                 documentation only)
//                 Field 3      -- temperature mode
//                                 = 1 - absolute
//                                 = 2 - relative
// Record 2:       FORMAT(3D25.17)
//                 Unit factors for converting universal file units to SI.
//                 To convert from universal file units to SI divide by
//                 the appropriate factor listed below.
//                 Field 1      -- length
//                 Field 2      -- force
//                 Field 3      -- temperature
//                 Field 4      -- temperature offset

// Example:

//     -1
//    164
//          2Foot (pound f)               2
//   3.28083989501312334D+00  2.24808943099710480D-01  1.79999999999999999D+00
//   4.59670000000000002D+02
//     -1

#include "SMESH_DriverUNV.hxx"

#include <string>

namespace UNV164
{
  enum { LENGTH_FACTOR, FORCE_FACTOR, TEMP_FACTOR, TEMP_OFFSET };

  struct MESHDRIVERUNV_EXPORT TRecord
  {
    int         units_code;
    std::string units_description;
    int         temp_mode;
    double      factors[4];
    TRecord();
  };
  
  MESHDRIVERUNV_EXPORT void
  Read(std::ifstream& in_stream, TRecord& theUnitsRecord);

  MESHDRIVERUNV_EXPORT void
  Write(std::ofstream& out_stream );

};


#endif
