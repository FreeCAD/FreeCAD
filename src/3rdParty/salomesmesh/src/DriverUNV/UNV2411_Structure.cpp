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
#include <stdio.h>	

#include "UNV2411_Structure.hxx"
#include "UNV_Utilities.hxx"

using namespace std;
using namespace UNV;
using namespace UNV2411;

static string _label_dataset = "2411";

UNV2411::TRecord::TRecord():
  exp_coord_sys_num(0),
  disp_coord_sys_num(0),
  color(11)//(0) -  0019936: EDF 794 SMESH : Export UNV : Node color and group id
{}

void UNV2411::Read(std::ifstream& in_stream, TDataSet& theDataSet)
{
  if(!in_stream.good())
    EXCEPTION(runtime_error,"ERROR: Input file not good.");

  /*
   * adjust the \p istream to our
   * position
   */
  if(!beginning_of_dataset(in_stream,_label_dataset))
    EXCEPTION(runtime_error,"ERROR: Could not find "<<_label_dataset<<" dataset!");

  /**
   * always 3 coordinates in the UNV file, no matter
   * which dimensionality libMesh is in
   */
  TNodeLab aLabel;
  std::string num_buf;
  for(; !in_stream.eof();){
    in_stream >> aLabel ;
    if(aLabel == -1){
      // end of dataset is reached
      break;
    }

    TRecord aRec;
    in_stream>>aRec.exp_coord_sys_num;
    in_stream>>aRec.disp_coord_sys_num;
    in_stream>>aRec.color;

    /*
     * take care of the
     * floating-point data
     */
    for(int d = 0; d < 3; d++){
      in_stream>>num_buf;
      aRec.coord[d] = D_to_e(num_buf);
    }

    theDataSet.insert(TDataSet::value_type(aLabel,aRec));
  }
}


void UNV2411::Write(std::ofstream& out_stream, const TDataSet& theDataSet)
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
    const TNodeLab& aLabel = anIter->first;
    const TRecord& aRec = anIter->second;
    char buf[78];
    sprintf(buf, "%10d%10d%10d%10d\n", 
	    aLabel,
	    aRec.exp_coord_sys_num,
	    aRec.disp_coord_sys_num,
	    aRec.color);
    out_stream<<buf;

    // the coordinates
    sprintf(buf, "%25.16E%25.16E%25.16E\n", 
	    aRec.coord[0],
	    aRec.coord[1],
	    aRec.coord[2]);
    out_stream<<buf;
  }
  
  
  /*
   * Write end of dataset
   */
  out_stream<<"    -1\n";
}
