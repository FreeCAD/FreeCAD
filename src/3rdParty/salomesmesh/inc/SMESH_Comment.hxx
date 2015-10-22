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
//  SMESH SMESH : implementaion of SMESH idl descriptions
// File      : SMESH_Comment.hxx
// Created   : Wed Mar 14 18:28:45 2007
// Author    : Edward AGAPOV (eap)
// Module    : SMESH
// $Header: 
//
#ifndef SMESH_Comment_HeaderFile
#define SMESH_Comment_HeaderFile

# include <string>
# include <sstream>


/*!
 * \brief Class to generate string from any type
 */
class SMESH_Comment : public std::string
{
  std::ostringstream _s ;

public :

  SMESH_Comment():std::string("") {}

  SMESH_Comment(const SMESH_Comment& c):std::string() {
    _s << c.c_str() ;
    this->std::string::operator=( _s.str() );
  }

  template <class T>
  SMESH_Comment( const T &anything ) {
    _s << anything ;
    this->std::string::operator=( _s.str() );
  }

  template <class T>
  SMESH_Comment & operator<<( const T &anything ) {
    _s << anything ;
    this->std::string::operator=( _s.str() );
    return *this ;
  }

  operator char*() const {
    return (char*)c_str();
  }
};


#endif
