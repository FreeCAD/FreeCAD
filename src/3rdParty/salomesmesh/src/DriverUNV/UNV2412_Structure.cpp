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
#include <fstream>	
#include <iomanip>

#include "UNV2412_Structure.hxx"
#include "UNV_Utilities.hxx"

using namespace std;
using namespace UNV;
using namespace UNV2412;

#ifdef _DEBUG_
static int MYDEBUG = 1;
#else
static int MYDEBUG = 0;
#endif

static string _label_dataset = "2412";

UNV2412::TRecord::TRecord():
  phys_prop_tab_num(2),
  mat_prop_tab_num(1),
  color(7),
  beam_orientation(0),
  beam_fore_end(0),
  beam_aft_end(0)
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

  TElementLab aLabel;
  for(; !in_stream.eof();){
    in_stream >> aLabel ;
    if(aLabel == -1){
      // end of dataset is reached
      break;
    }
    
    int n_nodes;
    TRecord aRec;
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
    for(int j=0; j < n_nodes; j++){
      // read node labels
      in_stream>>aRec.node_labels[j];             
    }

    theDataSet.insert(TDataSet::value_type(aLabel,aRec));
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
  for(; anIter != theDataSet.end(); anIter++){
    const TElementLab& aLabel = anIter->first;
    const TRecord& aRec = anIter->second;
    out_stream<<std::setw(10)<<aLabel;  /* element ID */
    out_stream<<std::setw(10)<<aRec.fe_descriptor_id;  /* type of element */
    out_stream<<std::setw(10)<<aRec.phys_prop_tab_num;
    out_stream<<std::setw(10)<<aRec.mat_prop_tab_num;
    out_stream<<std::setw(10)<<aRec.color;
    out_stream<<std::setw(10)<<aRec.node_labels.size()<<std::endl;  /* No. of nodes per element */

    if(IsBeam(aRec.fe_descriptor_id)){
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
  switch (theFeDescriptorId){
    
  case 71: // TRI3
  case 72:
  case 74:

  case 41: // Plane Stress Linear Triangle - TRI3
  case 91: // Thin Shell Linear Triangle - TRI3

  case 42: // Plane Stress Quadratic Triangle - TRI6
  case 92: // Thin Shell Quadratic Triangle - TRI6

  case 43: // Plane Stress Cubic Triangle

  case 44: // Plane Stress Linear Quadrilateral - QUAD4
  case 94: // Thin Shell   Linear Quadrilateral -  QUAD4

  case 45: // Plane Stress Quadratic Quadrilateral - QUAD8
  case 95: // Thin Shell   Quadratic Quadrilateral - QUAD8

  case 46: // Plane Stress Cubic Quadrilateral

    return true;
  }
  return false;
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
