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

#ifndef _SMESH_CONTROLS_HXX_
#define _SMESH_CONTROLS_HXX_

// This file is named incosistently with others, i.e. not SMESHDS_Controls.hxx,
// because it was moved from ../Controls/SMESH_Controls.hxx.
// It was moved here for the sake of SMESHDS_GroupOnFilter

#include "SMDSAbs_ElementType.hxx"

#include <boost/shared_ptr.hpp>

#ifdef WIN32
 #if defined SMESHCONTROLS_EXPORTS || defined SMESHControls_EXPORTS
  #define SMESHCONTROLS_EXPORT __declspec( dllexport )
 #else
  #define SMESHCONTROLS_EXPORT __declspec( dllimport )
 #endif
#else
 #define SMESHCONTROLS_EXPORT
#endif

class SMDS_Mesh;

namespace SMESH{
  namespace Controls{

    /*
      Class       : Functor
      Description : Root of all Functors defined in ../Controls/SMESH_ControlsDef.hxx
    */
    class SMESHCONTROLS_EXPORT Functor
    {
    public:
      virtual ~Functor(){}
      virtual void SetMesh( const SMDS_Mesh* theMesh ) = 0;
      virtual SMDSAbs_ElementType GetType() const = 0;
    };
    typedef boost::shared_ptr<Functor> FunctorPtr;


    class NumericalFunctor;
    typedef boost::shared_ptr<NumericalFunctor> NumericalFunctorPtr;
  
    /*
      Class       : Predicate
      Description : Base class for all predicates
    */
    class SMESHCONTROLS_EXPORT Predicate: public virtual Functor{
    public:
      virtual bool IsSatisfy( long theElementId ) = 0;
      virtual SMDSAbs_ElementType GetType() const = 0;
    };
    typedef boost::shared_ptr<Predicate> PredicatePtr;

  }
}

typedef SMESH::Controls::PredicatePtr SMESH_PredicatePtr;


#endif
