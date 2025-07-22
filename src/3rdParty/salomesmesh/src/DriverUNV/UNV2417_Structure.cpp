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

#include "UNV2417_Structure.hxx"
#include "UNV_Utilities.hxx"

#include <fstream>      
#include <iomanip>

using namespace std;
using namespace UNV;
using namespace UNV2417;

static string _group_labels[] = {"2417", "2429", "2430", "2432",
                                 "2435", "2452", "2467", "2477"};
#define NBGROUP 8

static string _label_dataset = "2467";

void UNV2417::Read(std::ifstream& in_stream, TDataSet& theDataSet)
{
  if(!in_stream.good())
    EXCEPTION(runtime_error,"ERROR: Input file not good.");

  std::string olds, news;
  
  while(true){
    in_stream >> olds >> news;
    /*
     * a "-1" followed by a number means the beginning of a dataset
     * stop combing at the end of the file
     */
    while( ((olds != "-1") || (news == "-1") ) && !in_stream.eof() ){     
      olds = news;
      in_stream >> news;
    }
    if(in_stream.eof())
      return;
    for (int i = 0; i < NBGROUP; i++) {
      if (news == _group_labels[i]) {
        ReadGroup(news, in_stream, theDataSet);
      }
    }
  }
}



void UNV2417::ReadGroup(const std::string& myGroupLabel, std::ifstream& in_stream, TDataSet& theDataSet)
{
  TGroupId aId;
  for(; !in_stream.eof();){
    in_stream >> aId ;
    if(aId == -1){
      // end of dataset is reached
      break;
    }

    int n_nodes;
    TRecord aRec;
    int aTmp;
    in_stream>>aTmp; // miss not necessary values
    in_stream>>aTmp;
    in_stream>>aTmp;
    in_stream>>aTmp;
    in_stream>>aTmp;
    in_stream>>aTmp;
    in_stream>>n_nodes;

    std::getline(in_stream, aRec.GroupName, '\n'); // Finalise previous reading
    std::getline(in_stream, aRec.GroupName, '\n');

    int aElType;
    int aElId;
    int aNum;
    for(int j=0; j < n_nodes; j++){
      in_stream>>aElType;
      in_stream>>aElId;
      if ((myGroupLabel.compare("2435") == 0) ||
          (myGroupLabel.compare("2452") == 0) ||
          (myGroupLabel.compare("2467") == 0) ||
          (myGroupLabel.compare("2477") == 0)) {
        in_stream>>aTmp;
        in_stream>>aTmp;
      }
      switch (aElType) {
      case 7: // Nodes
        aNum = aRec.NodeList.size();
        aRec.NodeList.resize(aNum + 1);
        aRec.NodeList[aNum] = aElId;
        break;
      case 8: // Elements
        aNum = aRec.ElementList.size();
        aRec.ElementList.resize(aNum + 1);
        aRec.ElementList[aNum] = aElId;
        break;
      }
    }
    theDataSet.insert(TDataSet::value_type(aId,aRec));
  }

}


void UNV2417::Write(std::ofstream& out_stream, const TDataSet& theDataSet)
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
    const TGroupId& aLabel = anIter->first;
    const TRecord& aRec = anIter->second;
    int aNbNodes = aRec.NodeList.size();
    int aNbElements = aRec.ElementList.size();
    int aNbRecords = aNbNodes + aNbElements;

    out_stream<<std::setw(10)<<aLabel;  /* group ID */
    out_stream<<std::setw(10)<<0;  
    out_stream<<std::setw(10)<<0;
    out_stream<<std::setw(10)<<0;
    out_stream<<std::setw(10)<<0;
    out_stream<<std::setw(10)<<0;
    out_stream<<std::setw(10)<<0;
    out_stream<<std::setw(10)<<aNbRecords<<std::endl; 

    out_stream<<aRec.GroupName<<std::endl;
    int aRow = 0;
    int i;
    for (i = 0; i < aNbNodes; i++) {
      if (aRow == 2) {
        out_stream<<std::endl; 
        aRow = 0;
      }
      out_stream<<std::setw(10)<<7;
      out_stream<<std::setw(10)<<aRec.NodeList[i];
      out_stream<<std::setw(10)<<0;
      out_stream<<std::setw(10)<<0;
      aRow++;
    }
    for (i = 0; i < aNbElements; i++) {
      if (aRow == 2) {
        out_stream<<std::endl; 
        aRow = 0;
      }
      out_stream<<std::setw(10)<<8;
      out_stream<<std::setw(10)<<aRec.ElementList[i];
      out_stream<<std::setw(10)<<0;
      out_stream<<std::setw(10)<<0;
      aRow++;
    }
    out_stream<<std::endl; 
  }

  /*
   * Write end of dataset
   */
  out_stream<<"    -1\n";
}
