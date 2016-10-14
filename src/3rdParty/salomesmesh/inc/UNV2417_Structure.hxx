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

#ifndef UNV2417_Structure_HeaderFile
#define UNV2417_Structure_HeaderFile

#include <map>
#include <vector>
#include <fstream>      
#include <string>       


namespace UNV2417{

  typedef std::vector<int> TListOfId; // Nodal connectivitiesList of Id

  struct TRecord{
    std::string    GroupName;
    TListOfId NodeList;
    TListOfId ElementList;
  };

  typedef int TGroupId; // type of element label
  typedef std::map<TGroupId, TRecord> TDataSet;

  void Read(std::ifstream& in_stream, TDataSet& theDataSet);
  void ReadGroup(const std::string& myGroupLabel, std::ifstream& in_stream, TDataSet& theDataSet);

  void Write(std::ofstream& out_stream, const TDataSet& theDataSet);
};


#endif
