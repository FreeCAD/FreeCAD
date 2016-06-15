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
#ifndef MED_Vector_HeaderFile
#define MED_Vector_HeaderFile

#include <vector>
#include <stdexcept>

//#if defined(_DEBUG_)
#  define MED_TVECTOR_CHECK_RANGE
//#endif

namespace MED
{

  //! Main purpose to introduce the class was to customize operator [] 
  template<typename _Tp, typename _Alloc = std::allocator<_Tp> >
  class TVector : public std::vector<_Tp, _Alloc>
  {
  public:
    typedef size_t size_type;

    typedef std::vector<_Tp, _Alloc> superclass;
    typedef typename superclass::allocator_type allocator_type;

    typedef _Tp value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;

  protected:
    void
    check_range(size_type __n) const
    {
      if (__n >= this->size())
        throw std::out_of_range("TVector [] access out of range");
    }

    const_reference
    get_value(size_type __n) const
    {
      return superclass::operator[](__n);
    }

    reference
    get_value(size_type __n)
    {
      return superclass::operator[](__n);
    }

  public:
    explicit
    TVector(const allocator_type& __a = allocator_type()): 
      superclass(__a) 
    {}
    
    TVector(size_type __n, const value_type& __val,
            const allocator_type& __a = allocator_type()):
      superclass(__n, __val, __a)
    {}
    
    explicit
    TVector(size_type __n):
      superclass(__n)
    {}

    TVector(const TVector& __x):
      superclass(__x)
    {}

    template<typename _InputIterator>
    TVector(_InputIterator __first, _InputIterator __last,
            const allocator_type& __a = allocator_type()):
      superclass(__first, __last, __a)
    {}

    template<typename _Yp, typename _Al>
    TVector(TVector<_Yp, _Al> __y):
      superclass(__y.begin(), __y.end())
    {}

    TVector&
    operator=(const TVector& __x)
    {
      superclass::operator=(__x);
      return *this;
    }

    template<typename _Yp, typename _Al>
    TVector&
    operator=(TVector<_Yp, _Al> __y)
    {
      this->assign(__y.begin(), __y.end());
      return *this;
    }

    reference
    operator[](size_type __n)
    {
#if defined(MED_TVECTOR_CHECK_RANGE)
      check_range(__n);
#endif
      return get_value(__n);
    }

    const_reference
    operator[](size_type __n) const
    {
#if defined(MED_TVECTOR_CHECK_RANGE)
      check_range(__n);
#endif
      return get_value(__n);
    }

    reference
    at(size_type __n)
    {
      check_range(__n);
      return get_value(__n);
    }

    const_reference
    at(size_type __n) const
    {
      check_range(__n);
      return get_value(__n);
    }
  };

}

#undef MED_TVECTOR_CHECK_RANGE

#endif
