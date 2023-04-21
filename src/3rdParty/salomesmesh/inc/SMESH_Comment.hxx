// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
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

//  SMESH SMESH : implementation of SMESH idl descriptions
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

using namespace std;

/*!
 * \brief Class to generate string from any type
 */
class SMESH_Comment : public string
{
  ostringstream _s ;

public :

  SMESH_Comment():string("") {}

  SMESH_Comment(const SMESH_Comment& c):string() {
    _s << c.c_str() ;
    this->string::operator=( _s.str() );
  }

  SMESH_Comment & operator=(const SMESH_Comment& c) {
    _s << c.c_str() ;
    this->string::operator=( _s.str() );
    return *this;
  }

  template <class T>
  SMESH_Comment( const T &anything ) {
    _s << anything ;
    this->string::operator=( _s.str() );
  }

  template <class T>
  SMESH_Comment & operator<<( const T &anything ) {
    _s << anything ;
    this->string::operator=( _s.str() );
    return *this ;
  }

  operator char*() const {
    return (char*)c_str();
  }
};


#endif
