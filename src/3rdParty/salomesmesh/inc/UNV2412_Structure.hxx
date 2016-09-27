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

#ifndef UNV2412_Structure_HeaderFile
#define UNV2412_Structure_HeaderFile

#include "SMESH_DriverUNV.hxx"

#include <vector>
#include <fstream>

namespace UNV2412{
  
  typedef std::vector<int> TNodeLabels; // Nodal connectivities
  typedef int TElementLab; // type of element label

  struct MESHDRIVERUNV_EXPORT TRecord
  {
    TRecord();

    TElementLab label;
    int fe_descriptor_id;  // FE descriptor id
    int phys_prop_tab_num;  // physical property table number
    int mat_prop_tab_num;  // material property table number
    int color;  // color
    TNodeLabels node_labels;  // node labels defining element

    //FOR BEAM ELEMENTS ONLY
    int beam_orientation;  // beam orientation node number
    int beam_fore_end;  // beam fore-end cross section number
    int beam_aft_end;  // beam  aft-end cross section number
  };
  
  typedef std::vector<TRecord> TDataSet;

  MESHDRIVERUNV_EXPORT void
    Read(std::ifstream& in_stream, TDataSet& theDataSet);

  MESHDRIVERUNV_EXPORT void
    Write(std::ofstream& out_stream, const TDataSet& theDataSet);

  MESHDRIVERUNV_EXPORT bool
    IsBeam(int theFeDescriptorId);
  MESHDRIVERUNV_EXPORT bool
    IsFace(int theFeDescriptorId);
  MESHDRIVERUNV_EXPORT bool
    IsVolume(int theFeDescriptorId);

};


#endif
