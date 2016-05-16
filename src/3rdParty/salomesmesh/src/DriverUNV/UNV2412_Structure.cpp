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

#include <fstream>      
#include <iomanip>

#include "UNV2412_Structure.hxx"
#include "UNV_Utilities.hxx"

using namespace std;
using namespace UNV;
using namespace UNV2412;

// Universal Dataset Number 2412

// Name:   Elements
// Status: Current
// Owner:  Simulation
// Revision Date: 14-AUG-1992
// -----------------------------------------------------------------------
 
// Record 1:        FORMAT(6I10)
//                  Field 1       -- element label
//                  Field 2       -- fe descriptor id
//                  Field 3       -- physical property table number
//                  Field 4       -- material property table number
//                  Field 5       -- color
//                  Field 6       -- number of nodes on element
 
// Record 2:  *** FOR NON-BEAM ELEMENTS ***
//                  FORMAT(8I10)
//                  Fields 1-n    -- node labels defining element
 
// Record 2:  *** FOR BEAM ELEMENTS ONLY ***
//                  FORMAT(3I10)
//                  Field 1       -- beam orientation node number
//                  Field 2       -- beam fore-end cross section number
//                  Field 3       -- beam  aft-end cross section number
 
// Record 3:  *** FOR BEAM ELEMENTS ONLY ***
//                  FORMAT(8I10)
//                  Fields 1-n    -- node labels defining element
 
// Records 1 and 2 are repeated for each non-beam element in the model.
// Records 1 - 3 are repeated for each beam element in the model.
 
// Example:
 
//     -1
//   2412
//          1        11         1      5380         7         2
//          0         1         1
//          1         2
//          2        21         2      5380         7         2
//          0         1         1
//          3         4
//          3        22         3      5380         7         2
//          0         1         2
//          5         6
//          6        91         6      5380         7         3
//         11        18        12
//          9        95         6      5380         7         8
//         22        25        29        30        31        26        24        23
//         14       136         8         0         7         2
//         53        54
//         36       116        16      5380         7        20
//        152       159       168       167       166       158       150       151
//        154       170       169       153       157       161       173       172
//        171       160       155       156
//     -1

// FE Descriptor Id definitions
// ____________________________

//    11  Rod
//    21  Linear beam
//    22  Tapered beam
//    23  Curved beam
//    24  Parabolic beam
//    31  Straight pipe
//    32  Curved pipe
//    41  Plane Stress Linear Triangle
//    42  Plane Stress Parabolic Triangle
//    43  Plane Stress Cubic Triangle
//    44  Plane Stress Linear Quadrilateral
//    45  Plane Stress Parabolic Quadrilateral
//    46  Plane Strain Cubic Quadrilateral
//    51  Plane Strain Linear Triangle
//    52  Plane Strain Parabolic Triangle
//    53  Plane Strain Cubic Triangle
//    54  Plane Strain Linear Quadrilateral
//    55  Plane Strain Parabolic Quadrilateral
//    56  Plane Strain Cubic Quadrilateral
//    61  Plate Linear Triangle
//    62  Plate Parabolic Triangle
//    63  Plate Cubic Triangle
//    64  Plate Linear Quadrilateral
//    65  Plate Parabolic Quadrilateral
//    66  Plate Cubic Quadrilateral
//    71  Membrane Linear Quadrilateral
//    72  Membrane Parabolic Triangle
//    73  Membrane Cubic Triangle
//    74  Membrane Linear Triangle
//    75  Membrane Parabolic Quadrilateral
//    76  Membrane Cubic Quadrilateral
//    81  Axisymetric Solid Linear Triangle
//    82  Axisymetric Solid Parabolic Triangle
//    84  Axisymetric Solid Linear Quadrilateral
//    85  Axisymetric Solid Parabolic Quadrilateral
//    91  Thin Shell Linear Triangle
//    92  Thin Shell Parabolic Triangle
//    93  Thin Shell Cubic Triangle
//    94  Thin Shell Linear Quadrilateral
//    95  Thin Shell Parabolic Quadrilateral
//    96  Thin Shell Cubic Quadrilateral
//    101 Thick Shell Linear Wedge
//    102 Thick Shell Parabolic Wedge
//    103 Thick Shell Cubic Wedge
//    104 Thick Shell Linear Brick
//    105 Thick Shell Parabolic Brick
//    106 Thick Shell Cubic Brick
//    111 Solid Linear Tetrahedron
//    112 Solid Linear Wedge
//    113 Solid Parabolic Wedge
//    114 Solid Cubic Wedge
//    115 Solid Linear Brick
//    116 Solid Parabolic Brick
//    117 Solid Cubic Brick
//    118 Solid Parabolic Tetrahedron
//    121 Rigid Bar
//    122 Rigid Element
//    136 Node To Node Translational Spring
//    137 Node To Node Rotational Spring
//    138 Node To Ground Translational Spring
//    139 Node To Ground Rotational Spring
//    141 Node To Node Damper
//    142 Node To Gound Damper
//    151 Node To Node Gap
//    152 Node To Ground Gap
//    161 Lumped Mass
//    171 Axisymetric Linear Shell
//    172 Axisymetric Parabolic Shell
//    181 Constraint
//    191 Plastic Cold Runner
//    192 Plastic Hot Runner
//    193 Plastic Water Line
//    194 Plastic Fountain
//    195 Plastic Baffle
//    196 Plastic Rod Heater
//    201 Linear node-to-node interface
//    202 Linear edge-to-edge interface
//    203 Parabolic edge-to-edge interface
//    204 Linear face-to-face interface
//    208 Parabolic face-to-face interface
//    212 Linear axisymmetric interface
//    213 Parabolic axisymmetric interface
//    221 Linear rigid surface
//    222 Parabolic rigin surface
//    231 Axisymetric linear rigid surface
//    232 Axisymentric parabolic rigid surface



static string _label_dataset = "2412";

UNV2412::TRecord::TRecord():
  label(-1),
  fe_descriptor_id(-1),
  phys_prop_tab_num(2),
  mat_prop_tab_num(1),
  color(7),
  beam_orientation(0),
  beam_fore_end(1), // default values
  beam_aft_end(1) // default values
{}

void UNV2412::Read(std::ifstream& in_stream, TDataSet& theDataSet)
{
  if(!in_stream.good())
    EXCEPTION(runtime_error,"ERROR: Input file not good.");

  /*
   * adjust the \p istream to our
   * position
   */
  if(!beginning_of_dataset(in_stream,_label_dataset))
    EXCEPTION(runtime_error,"ERROR: Could not find "<<_label_dataset<<" dataset!");

  TRecord aRec;
  while( !in_stream.eof())
  {
    in_stream >> aRec.label ;
    if (aRec.label == -1)
      // end of dataset is reached
      break;
    
    int n_nodes;
    in_stream>>aRec.fe_descriptor_id;
    in_stream>>aRec.phys_prop_tab_num;
    in_stream>>aRec.mat_prop_tab_num;
    in_stream>>aRec.color;
    in_stream>>n_nodes;

    if(IsBeam(aRec.fe_descriptor_id)){
      in_stream>>aRec.beam_orientation;
      in_stream>>aRec.beam_fore_end;
      in_stream>>aRec.beam_aft_end;
    }

    aRec.node_labels.resize(n_nodes);
    for(int j=0; j < n_nodes; j++)
      // read node labels
      in_stream>>aRec.node_labels[j];             

    theDataSet.push_back(aRec);
  }

}


void UNV2412::Write(std::ofstream& out_stream, const TDataSet& theDataSet)
{
  if(!out_stream.good())
    EXCEPTION(runtime_error,"ERROR: Output file not good.");
  
  /*
   * Write beginning of dataset
   */
  out_stream<<"    -1\n";
  out_stream<<"  "<<_label_dataset<<"\n";

  TDataSet::const_iterator anIter = theDataSet.begin();
  for(; anIter != theDataSet.end(); anIter++)
  {
    const TRecord& aRec = *anIter;
    out_stream<<std::setw(10)<<aRec.label;  /* element ID */
    out_stream<<std::setw(10)<<aRec.fe_descriptor_id;  /* type of element */
    out_stream<<std::setw(10)<<aRec.phys_prop_tab_num;
    out_stream<<std::setw(10)<<aRec.mat_prop_tab_num;
    out_stream<<std::setw(10)<<aRec.color;
    out_stream<<std::setw(10)<<aRec.node_labels.size()<<std::endl;  /* No. of nodes per element */

    if(IsBeam(aRec.fe_descriptor_id))
    {
      out_stream<<std::setw(10)<<aRec.beam_orientation;
      out_stream<<std::setw(10)<<aRec.beam_fore_end;
      out_stream<<std::setw(10)<<aRec.beam_aft_end<<std::endl;
    }

    int n_nodes = aRec.node_labels.size();
    int iEnd = (n_nodes-1)/8 + 1;
    for(int i = 0, k = 0; i < iEnd; i++){
      int jEnd = n_nodes - 8*(i+1);
      if(jEnd < 0) 
        jEnd = 8 + jEnd;
      else
        jEnd = 8;
      for(int j = 0; j < jEnd ; k++, j++){
        out_stream<<std::setw(10)<<aRec.node_labels[k];
      }
      out_stream<<std::endl;
    }
  }

  /*
   * Write end of dataset
   */
  out_stream<<"    -1\n";
}


bool UNV2412::IsBeam(int theFeDescriptorId){
  switch (theFeDescriptorId){
  case 11: // edge with 2 nodes
  case 21: 
  case 22: // edge with 3 nodes
  case 23: // curved beam
  case 24:
  case 25:
    return true;
  }
  return false;
}


bool UNV2412::IsFace(int theFeDescriptorId){
  return ( 41 <= theFeDescriptorId && theFeDescriptorId <= 96 );
//   switch (theFeDescriptorId){
    
//   case 71: // TRI3
//   case 72:
//   case 74:

//   case 41: // Plane Stress Linear Triangle - TRI3
//   case 51: // Plane Strain Linear Triangle
//   case 91: // Thin Shell Linear Triangle - TRI3

//   case 42: // Plane Stress Quadratic Triangle - TRI6
//   case 52: // Plane Strain Parabolic Triangle
//   case 92: // Thin Shell Quadratic Triangle - TRI6

//   case 43: // Plane Stress Cubic Triangle

//   case 44: // Plane Stress Linear Quadrilateral - QUAD4
//   case 94: // Thin Shell   Linear Quadrilateral -  QUAD4

//   case 45: // Plane Stress Quadratic Quadrilateral - QUAD8
//   case 95: // Thin Shell   Quadratic Quadrilateral - QUAD8

//   case 46: // Plane Stress Cubic Quadrilateral

//     return true;
//   }
//  return false;
}


bool UNV2412::IsVolume(int theFeDescriptorId){
  //if(!IsBeam(theFeDescriptorId) && !IsFace(theFeDescriptorId))
  //  return true;
  switch (theFeDescriptorId){

  case 111: // Solid Linear Tetrahedron - TET4
  case 118: // Solid Quadratic Tetrahedron - TET10

  case 112: // Solid Linear Prism - PRISM6
  case 113: // Solid Quadratic Prism - PRISM15

  case 115: // Solid Linear Brick - HEX8
  case 116: // Solid Quadratic Brick - HEX20

  case 117: // Solid Cubic Brick

  case 114: // pyramid of 13 nodes (quadratic)
    return true;
  }
  return false;
}
