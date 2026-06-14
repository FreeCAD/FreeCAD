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

#include "UNV164_Structure.hxx"
#include "UNV_Utilities.hxx"

#include <fstream>
#include <cstdio>
#include <cmath>

using namespace std;
using namespace UNV;
using namespace UNV164;

static string _label_dataset = "164";

void UNV164::Read(std::ifstream& in_stream, TRecord& theUnitsRecord )
{
  if(!in_stream.good())
    EXCEPTION(runtime_error,"ERROR: Input file not good.");

  if(!beginning_of_dataset(in_stream,_label_dataset))
    return;

  string num_buf;
  char line[theMaxLineLen] = "";

  in_stream >> theUnitsRecord.units_code;
  in_stream.readsome( line, 20 );
  theUnitsRecord.units_description = line;
  in_stream >> theUnitsRecord.temp_mode;

  for ( int i = 0; i < 4; i++ )
  {
    in_stream >> num_buf;
    theUnitsRecord.factors[i] = D_to_e(num_buf);
  }
}

void UNV164::Write(std::ofstream& out_stream)
{
  if(!out_stream.good())
    EXCEPTION(runtime_error,"ERROR: Output file not good.");
  
  out_stream<<"    -1" << endl;
  out_stream<<"   "<<_label_dataset << endl;

  out_stream<<"         1  SI: Meter (newton)         2"                                    << endl;
  out_stream<<"    1.0000000000000000E+0    1.0000000000000000E+0    1.0000000000000000E+0" << endl;
  out_stream<<"    2.7314999999999998E+2"                                                   << endl;

  out_stream<<"    -1"  << endl;
}

UNV164::TRecord::TRecord()
{
  units_code        = 1;
  units_description = "SI: Meter (newton)";
  temp_mode         = 2;
  factors[0]        = 1.0;
  factors[1]        = 1.0;
  factors[2]        = 1.0;
  factors[3]        = 273.15;
}
