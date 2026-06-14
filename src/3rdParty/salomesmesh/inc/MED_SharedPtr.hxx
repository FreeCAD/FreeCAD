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
#ifndef MED_SharedPtr_HeaderFile
#define MED_SharedPtr_HeaderFile

#include <boost/shared_ptr.hpp>

namespace MED
{

  //! To extend the boost::shared_ptr to support such features automatic dynamic cast
  /*!
    All entities of the MEDWrapper package are handled as pointer.
    This class was introduced to provide correct and flexible memory management 
    for all of the MEDWrapper objects.
  */
  template<class T> class SharedPtr: public boost::shared_ptr<T>
  {
  public:
    //! Default constructor
    SharedPtr() {}

    //! Construct the class by any type of a pointer
    template<class Y>
    explicit SharedPtr(Y * p): 
      boost::shared_ptr<T>(p) 
    {}

    //! Construct the class by any specialisation of the class
    template<class Y>
    SharedPtr(SharedPtr<Y> const & r):
      boost::shared_ptr<T>(boost::dynamic_pointer_cast<T,Y>(r))
    {}

    //! Copy-constructor
    template<class Y>
    SharedPtr& 
    operator=(SharedPtr<Y> const & r)
    {
      SharedPtr<T>(r).swap(*this);
      return *this;
    }

    //! Introduce a flexible way to reset the wrapped pointer
    template<class Y> 
    SharedPtr& 
    operator()(Y * p) // Y must be complete
    {
      return operator=<Y>(SharedPtr<Y>(p));
    }

    //! Introduce a flexible way to reset the wrapped pointer
    template<class Y> 
    SharedPtr& 
    operator()(SharedPtr<Y> const & r) // Y must be complete
    {
      return operator=<Y>(SharedPtr<Y>(r));
    }

    //! To provide a flexible way to use reference to the wrapped pointer (const version)
    operator const T& () const 
    { 
      return *(this->get());
    }

    //! To provide a flexible way to use reference to the wrapped pointer
    operator T& () 
    { 
      return *(this->get());
    }
  };

}


#endif
