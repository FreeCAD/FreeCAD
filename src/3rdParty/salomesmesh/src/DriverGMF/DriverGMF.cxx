// Copyright (C) 2007-2016  CEA/DEN, EDF R&D, OPEN CASCADE
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
// File      : DriverGMF.hxx
// Created   : Thu Nov 15 16:45:58 2012
// Author    : Edward AGAPOV (eap)

#include "DriverGMF.hxx"

#include <boost/filesystem.hpp>

extern "C"
{
#include "libmesh5.h"
}

namespace DriverGMF
{

  //================================================================================
  /*!
   * \brief Closes GMF mesh at destruction
   */
  //================================================================================

  MeshCloser::~MeshCloser()
  {
    if ( _gmfMeshID )
      GmfCloseMesh( _gmfMeshID );
  }

  //================================================================================
  /*!
   * \brief Checks GMF file extension
   */
  //================================================================================

  bool isExtensionCorrect( const std::string& fileName )
  {
    std::string ext  = boost::filesystem::extension(fileName);
    switch ( ext.size() ) {
    case 5: return ( ext == ".mesh" || ext == ".solb" );
    case 6: return ( ext == ".meshb" );
    case 4: return ( ext == ".sol" );
    }
    return false;
  }
}
