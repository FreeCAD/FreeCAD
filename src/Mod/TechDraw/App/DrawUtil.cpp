
/***************************************************************************
 *   Copyright (c) 2015 WandererFan <wandererfan@gmail.com>                *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <sstream>
# include <cstring>
# include <cstdlib>
# include <exception>
# include <boost/regex.hpp>
# include <QString>
# include <QStringList>
# include <QRegExp>
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>

#include "DrawUtil.h"

namespace DrawUtil {

//==============================================================================
// convenient utility functions for TechDraw Module
//==============================================================================
extern "C" {
int TechDrawExport getIndexFromName(std::string geomName)
{
   boost::regex re("\\d+$");                                           //one of more digits at end of string
   boost::match_results<std::string::const_iterator> what;
   boost::match_flag_type flags = boost::match_default;
   char* endChar;
   std::string::const_iterator begin = geomName.begin();
   std::string::const_iterator end = geomName.end();
   std::stringstream ErrorMsg;

   if (!geomName.empty()) {
      if (boost::regex_search(begin, end, what, re, flags)) {
         return int (std::strtol(what.str().c_str(), &endChar, 10));         //TODO: use std::stoi() in c++11
      } else {
         ErrorMsg << "getIndexFromName: malformed geometry name - " << geomName;
         throw Base::Exception(ErrorMsg.str());
      }
   } else {
         throw Base::Exception("getIndexFromName - empty geometry name");
   }
}

std::string TechDrawExport getGeomTypeFromName(std::string geomName)
{
   boost::regex re("^[a-zA-Z]*");                                           //one or more letters at start of string
   boost::match_results<std::string::const_iterator> what;
   boost::match_flag_type flags = boost::match_default;
   std::string::const_iterator begin = geomName.begin();
   std::string::const_iterator end = geomName.end();
   std::stringstream ErrorMsg;

   if (!geomName.empty()) {
      if (boost::regex_search(begin, end, what, re, flags)) {
         return what.str();         //TODO: use std::stoi() in c++11
      } else {
         ErrorMsg << "In getGeomTypeFromName: malformed geometry name - " << geomName;
         throw Base::Exception(ErrorMsg.str());
      }
   } else {
         throw Base::Exception("getGeomTypeFromName - empty geometry name");
   }
}

std::string TechDrawExport makeGeomName(std::string geomType, int index)
{
    std::stringstream newName;
    newName << geomType << index;
    return newName.str();
}
} //end extern "C"
} //end namespace DrawUtil
