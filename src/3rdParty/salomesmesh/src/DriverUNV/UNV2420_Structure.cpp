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

#include "UNV2420_Structure.hxx"
#include "UNV_Utilities.hxx"

#include <fstream>
#include <cstdio>
#include <cmath>

using namespace std;
using namespace UNV;
using namespace UNV2420;

static string _label_dataset = "2420";

void UNV2420::Read(std::ifstream& in_stream,
                   std::string&   part_name, // can re-store a mesh name
                   TDataSet&      theDataSet)
{
  if(!in_stream.good())
    EXCEPTION(runtime_error,"ERROR: Input file not good.");

  /*
   * adjust the \p istream to our
   * position
   */
  if(!beginning_of_dataset(in_stream,_label_dataset))
    return;

  string num_buf;
  int part_uid;

  in_stream >> part_uid; // Record 1
  part_name = read_line( in_stream );  // Record 2

  while ( !in_stream.eof() )
  {
    TRecord aRec;

    // Record 3
    in_stream >> aRec.coord_sys_label;
    if ( aRec.coord_sys_label == -1 ) // end of dataset is reached
      break;
    in_stream >> aRec.coord_sys_type;
    in_stream >> aRec.coord_sys_color;

    aRec.coord_sys_name = read_line( in_stream ); // Record 4

    // Records 5-8: rows of Transformation Matrix
    for ( int row = 0; row < 4; ++row )
      for ( int i = 0; i < 3; i++ )
      {
        in_stream >> num_buf;
        aRec.matrix[row][i] = D_to_e(num_buf);
      }
    // Store a CS data only if it requires conversion into the global Cartesian CS
    if ( aRec.coord_sys_type != 0 || !aRec.isIdentityMatrix() ) // 0 - Cartesian CS
      theDataSet.push_back( aRec );
  }
}


void UNV2420::Write(std::ofstream&     out_stream,
                    const std::string& part_name)
//                    const TDataSet& theDataSet)
{
  if(!out_stream.good())
    EXCEPTION(runtime_error,"ERROR: Output file not good.");
  
  out_stream<<"    -1"  << endl;
  out_stream<<"  "<<_label_dataset << endl;

  out_stream<<"         1"                     << endl; // R1: Part UID
  if ( part_name.empty() )
    out_stream<<"SMESH_Mesh"                   << endl; // R2: Part Name
  else
    out_stream<< part_name                     << endl;
  out_stream<<"         1         0         0" << endl; // R3: Label, Type, Color

  out_stream<<"Global Cartesian Coordinate System" << endl; // R4: Name
  out_stream<<"    1.0000000000000000E+0    0.0000000000000000E+0    0.0000000000000000E+0" << endl;
  out_stream<<"    0.0000000000000000E+0    1.0000000000000000E+0    0.0000000000000000E+0" << endl;
  out_stream<<"    0.0000000000000000E+0    0.0000000000000000E+0    1.0000000000000000E+0" << endl;
  out_stream<<"    0.0000000000000000E+0    0.0000000000000000E+0    0.0000000000000000E+0" << endl;

  out_stream<<"    -1"  << endl;
}


bool UNV2420::TRecord::isIdentityMatrix() const
{
  bool isIdentity = true;
  for ( int row = 0; row < 4 && isIdentity; ++row )
    for ( int i = 0; i < 3; i++ )
    {
      if ( matrix[row][i] != ( row==i ? 1. : 0. ))
      {
        isIdentity = false;
        break;
      }
    }
  return isIdentity;
}

void UNV2420::TRecord::ApplyMatrix( double* c ) const
{
  const double x = matrix[0][0] * c[0] + matrix[0][1] * c[1] + matrix[0][2] * c[2];
  const double y = matrix[1][0] * c[0] + matrix[1][1] * c[1] + matrix[1][2] * c[2];
  const double z = matrix[2][0] * c[0] + matrix[2][1] * c[1] + matrix[2][2] * c[2];
  c[0] = x + matrix[3][0];
  c[1] = y + matrix[3][1];
  c[2] = z + matrix[3][2];
}

void UNV2420::TRecord::FromCylindricalCS( double* coords )
{
  const double x = coords[0] * cos( coords[1] );
  const double y = coords[0] * sin( coords[1] );
  coords[0] = x;
  coords[1] = y;
}

void UNV2420::TRecord::FromSphericalCS  ( double* coords )
{
  const double sin2 = sin( coords[2] );
  const double x = coords[0] * cos( coords[1] ) * sin2;
  const double y = coords[0] * sin( coords[1] ) * sin2;
  const double z = coords[0] * cos( coords[2] );
  coords[0] = x;
  coords[1] = y;
  coords[2] = z;
}
