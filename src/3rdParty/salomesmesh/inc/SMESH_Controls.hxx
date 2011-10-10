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
#ifndef _SMESH_CONTROLS_HXX_
#define _SMESH_CONTROLS_HXX_

#include <boost/shared_ptr.hpp>

namespace SMESH{
  namespace Controls{

    class Functor;
    typedef boost::shared_ptr<Functor> FunctorPtr;


    class NumericalFunctor;
    typedef boost::shared_ptr<NumericalFunctor> NumericalFunctorPtr;
  
  
    class Predicate;
    typedef boost::shared_ptr<Predicate> PredicatePtr;

  }
}


#endif
