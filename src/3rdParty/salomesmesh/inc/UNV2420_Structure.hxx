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

#ifndef UNV2420_Structure_HeaderFile
#define UNV2420_Structure_HeaderFile

// Name:   Coordinate Systems
// -----------------------------------------------------------------------

// Record 1:        FORMAT (1I10)
//                  Field 1       -- Part UID

// Record 2:        FORMAT (40A2)
//                  Field 1       -- Part Name

// Record 3:        FORMAT (3I10)
//                  Field 1       -- Coordinate System Label
//                  Field 2       -- Coordinate System Type
//                                   = 0, Cartesian
//                                   = 1, Cylindrical
//                                   = 2, Spherical
//                  Field 3       -- Coordinate System Color

// Record 4:        FORMAT (40A2)
//                  Field 1       -- Coordinate System Name

// Record 5:        FORMAT (1P3D25.16)
//                  Field 1-3     -- Transformation Matrix Row 1

// Record 6:        FORMAT (1P3D25.16)
//                  Field 1-3     -- Transformation Matrix Row 2

// Record 7:        FORMAT (1P3D25.16)
//                  Field 1-3     -- Transformation Matrix Row 3

// Record 8:        FORMAT (1P3D25.16)
//                  Field 1-3     -- Transformation Matrix Row 4

// Records 3 thru 8 are repeated for each Coordinate System in the Part.

// Example:
//     -1
//   2420
//        100
// Untitled
//          6         1        15
// FEMAP Global Cylindrical (6)
//     1.0000000000000000E+0    0.0000000000000000E+0    0.0000000000000000E+0
//     0.0000000000000000E+0    1.0000000000000000E+0    0.0000000000000000E+0
//     0.0000000000000000E+0    0.0000000000000000E+0    1.0000000000000000E+0
//     0.0000000000000000E+0    0.0000000000000000E+0    0.0000000000000000E+0
//          7         2        15
// Coordinate System 4
//     1.0000000000000000E+0    0.0000000000000000E+0    0.0000000000000000E+0
//     0.0000000000000000E+0    1.0000000000000000E+0    0.0000000000000000E+0
//     0.0000000000000000E+0    0.0000000000000000E+0    1.0000000000000000E+0
//     0.0000000000000000E+0    0.0000000000000000E+0    0.0000000000000000E+0
//     -1

#include "SMESH_DriverUNV.hxx"

#include <string>
#include <vector>

namespace UNV2420
{
  enum { Cartesian=0, Cylindrical, Spherical };

  typedef int TCSLabel; // type of coord system label

  struct MESHDRIVERUNV_EXPORT TRecord
  {
    TCSLabel    coord_sys_label; 
    int         coord_sys_type;  // { Cartesian=0, Cylindrical, Spherical }
    int         coord_sys_color;
    std::string coord_sys_name;
    double      matrix[4][3];

    bool        isIdentityMatrix() const;
    void        ApplyMatrix      ( double* coords ) const;
    static void FromCylindricalCS( double* coords );
    static void FromSphericalCS  ( double* coords );
  };
  
  typedef std::vector<TRecord> TDataSet;

  MESHDRIVERUNV_EXPORT void
  Read(std::ifstream& in_stream,
       std::string&   part_name, // can re-store a mesh name
       TDataSet&      theDataSet);

  MESHDRIVERUNV_EXPORT void
  Write(std::ofstream&     out_stream,
        const std::string& part_name); // can store a mesh name
  //    const TDataSet&    theDataSet);

};


#endif
