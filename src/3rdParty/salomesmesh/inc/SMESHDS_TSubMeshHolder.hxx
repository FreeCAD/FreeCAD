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
#ifndef __SMESHDS_SubMeshHolder_HXX__
#define __SMESHDS_SubMeshHolder_HXX__

#include <vector>
#include <map>

//=======================================================================
/*!
 * \brief A binder of a sub-mesh to its ID which can be negative. Provides fast
 *        access to a sub-mesh by its ID.
 *
 * Issue 52457: Addition of hypotheses is 8 time longer than meshing.
 */
//=======================================================================

template <class SUBMESH>
class SMESHDS_TSubMeshHolder
{
  std::vector< SUBMESH* >   myVec; // for ID >= 0
  std::map< int, SUBMESH* > myMap; // for ID < 0

public:

  ~SMESHDS_TSubMeshHolder()
  {
    DeleteAll();
  }
  void Add( int id, SUBMESH* sm )
  {
    if ( id < 0 )
    {
      myMap[ id ] = sm;
    }
    else
    {
      if ( myVec.size() <= id )
        myVec.resize( id+1, (SUBMESH*) NULL );
      myVec[ id ] = sm;
    }
  }
  SUBMESH* Get( int id ) const
  {
    if ( id < 0 )
    {
      typename std::map< int, SUBMESH* >::const_iterator i2sm = myMap.find( id );
      return (SUBMESH*) ( i2sm == myMap.end() ? NULL : i2sm->second );
    }
    else
    {
      return (SUBMESH*) ( id >= myVec.size() ? NULL : myVec[ id ]);
    }
  }
  void DeleteAll()
  {
    for ( size_t i = 0; i < myVec.size(); ++i )
      if ( SUBMESH* sm = myVec[i] )
      {
        myVec[i] = 0; // avoid access via Get(i)
        delete sm;
      }
    myVec.clear();

    typename std::map< int, SUBMESH* >::iterator i2sm = myMap.begin();
    for ( ; i2sm != myMap.end(); ++i2sm )
      if ( SUBMESH* sm = i2sm->second )
      {
        i2sm->second = 0; // avoid access via Get(i)
        delete sm;
      }
    myMap.clear();
  }
  int GetMinID() const
  {
    return myMap.empty() ? 0 : myMap.begin()->first;
  }
  int GetMaxID() const
  {
    return myVec.empty() ? ( myMap.empty() ? 0 : myMap.rbegin()->first ) : myVec.size();
  }

  //-----------------------------------------------------------------------
  struct Iterator : public SMDS_Iterator< SUBMESH* >
  {
    const SMESHDS_TSubMeshHolder<SUBMESH>* myHolder;
    SUBMESH* myNext;
    int myCurID, myEndID, myIDDelta;

    void init( const SMESHDS_TSubMeshHolder<SUBMESH>* holder,
               int firstID, int endID, int delta )
    {
      myHolder  = holder;
      myNext    = 0;
      myCurID   = firstID;
      myEndID   = endID;
      myIDDelta = delta;

      next();
    }

    bool more()
    {
      return myNext;
    }

    SUBMESH* next()
    {
      SUBMESH* res = myNext;
      myNext = 0;
      while ( !myNext && myCurID != myEndID )
      {
        myNext = myHolder->Get( myCurID );
        myCurID += myIDDelta;
      }
      return res;
    }
    virtual ~Iterator() {}
  };
  //-----------------------------------------------------------------------

  SMDS_Iterator< SUBMESH* >* GetIterator(const bool reverse=false) const
  {
    Iterator* iter = new Iterator;
    if ( reverse ) iter->init( this, GetMaxID(), GetMinID()-1, -1 );
    else           iter->init( this, GetMinID(), GetMaxID()+1, +1 );
    return iter;
  }
};


#endif
