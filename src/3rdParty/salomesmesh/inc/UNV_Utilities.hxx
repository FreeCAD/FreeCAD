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

#ifndef MED_Utilities_HeaderFile
#define MED_Utilities_HeaderFile

#include "SMESH_DriverUNV.hxx"

#include <iostream>     
#include <sstream>      
#include <fstream>
#include <string>
#include <stdexcept>
#include <cassert>
#include <cstdlib>

namespace UNV{
  using namespace std;

  const size_t theMaxLineLen = 82; // 80 for text + 2 for "\r\n"

  class MESHDRIVERUNV_EXPORT PrefixPrinter{
    static int myCounter;
  public:
    PrefixPrinter();
    ~PrefixPrinter();

    static string GetPrefix();
  };

  /**
   * @returns \p false when error occurred, \p true otherwise.
   * Adjusts the \p in_stream to the beginning of the
   * dataset \p ds_name.
   */
  inline bool beginning_of_dataset(std::istream& in_file, const std::string& ds_name)
  {
    assert (in_file.good());
    assert (!ds_name.empty());

    std::string olds, news;

    in_file.seekg(0);
    while(!in_file.eof() && !in_file.fail())
    {
      in_file >> olds >> news;
      /*
       * a "-1" followed by a number means the beginning of a dataset
       * stop combing at the end of the file
       */
      while( ((olds != "-1") || (news == "-1")))
      {
        olds = news;
        in_file >> news;

        if ( in_file.eof() || in_file.fail() )
        {
          in_file.clear();
          return false;
        }
      }
      if (news == ds_name)
        return true;
    }
    // We didn't found the card
    // Let's rewind the file handler and return an error
    in_file.clear();
    return false;
  }

  /**
   * Method for converting exponential notation
   * from "D" to "e", for example
   * \p 3.141592654D+00 \p --> \p 3.141592654e+00
   * in order to make it readable for C++.
   */
  inline double D_to_e(std::string& number)
  {
    /* find "D" in string, start looking at 
     * 6th element, to improve speed.
     * We dont expect a "D" earlier
     */
    const int position = number.find("D",6);
    if(position != std::string::npos){
      number.replace(position, 1, "e"); 
    }
    return atof (number.c_str());
  }
  
  /**
   * @returns \p false when file is incorrect, \p true otherwise.
   * Check file with name \p theFileName for correct terminate
   * string, i.e. the next to the last line is equal to "    -1",
   */
  inline bool check_file(const std::string theFileName)
  {
    std::ifstream in_stream(theFileName.c_str());
    if (!in_stream)
      return false;
    std::string olds, news;
    while (!in_stream.eof()){
      olds = news;
      std::getline(in_stream, news, '\n');
    }
    return (olds == "    -1");
  }

  /*!
   * \brief reads a whole line
   *  \param in_stream - source stream
   *  \param next - if true, first reads the current line up to the end
   *  which is necessary after reading using >> operator
   *  \retval std::string - the result line
   */
  inline std::string read_line(std::ifstream& in_stream, const bool next=true)
  {
    std::string resLine;
    std::getline( in_stream, resLine );
    if ( next )
      std::getline( in_stream, resLine );

    if ( resLine.size() > 0 && resLine[ resLine.size()-1 ] == '\r' )
      resLine.resize( resLine.size()-1 );
    return resLine;
  }
};


#ifndef MESSAGE

#define MESSAGE(msg) std::cout<<__FILE__<<"["<<__LINE__<<"]::"<<msg<<endl;

#define BEGMSG(msg) std::cout<<UNV::PrefixPrinter::GetPrefix()<<msg

#define ADDMSG(msg) std::cout<<msg

#endif


#ifndef EXCEPTION

#define EXCEPTION(TYPE, MSG) {\
  std::ostringstream aStream;\
  aStream<<__FILE__<<"["<<__LINE__<<"]::"<<MSG;\
  throw TYPE(aStream.str());\
}

#endif

#endif
