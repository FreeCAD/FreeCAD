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

//  SALOME Utils : general SALOME's definitions and tools
//  File:       Utils_Mutex.hxx
//  Author:     Sergey ANIKIN
//  Module :    SALOME
//  $Header$
//
#ifndef Utils_Mutex_HeaderFile
#define Utils_Mutex_HeaderFile

#include "SALOME_Utils.hxx"

#include <pthread.h>

class UTILS_EXPORT Utils_Mutex
{
public:
  Utils_Mutex();
  ~Utils_Mutex();

  void lock();
  void unlock();
    
private:
  pthread_mutex_t myMutex;
  pthread_mutex_t myHelperMutex;
  pthread_t       myThread;
  int             myCount;
};

class UTILS_EXPORT Utils_Locker
{
public:
  Utils_Locker( Utils_Mutex* );
  virtual ~Utils_Locker();

private:
  Utils_Mutex* myMutex;
};

#endif
