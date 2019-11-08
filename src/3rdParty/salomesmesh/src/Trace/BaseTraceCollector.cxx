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

//  File   : BaseTraceCollector.cxx
//  Author : Paul RASCLE (EDF)
//  Module : KERNEL
//  $Header$
//
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>

#include "BaseTraceCollector.hxx"
#include "LocalTraceBufferPool.hxx"

// Class attributes initialisation, for class method BaseTraceCollector::run

BaseTraceCollector* BaseTraceCollector::_singleton = 0;
#ifndef WIN32
pthread_mutex_t BaseTraceCollector::_singletonMutex;
#else
pthread_mutex_t BaseTraceCollector::_singletonMutex =
  PTHREAD_MUTEX_INITIALIZER;
#endif
sem_t BaseTraceCollector::_sem;
int BaseTraceCollector::_threadToClose = 0;
pthread_t* BaseTraceCollector::_threadId = 0; // used to control single run

// ============================================================================
/*!
 *  Destructor: virtual, implemented in derived classes.
 *  Wait until printing thread ends (BaseTraceCollector::run)
 */
// ============================================================================

BaseTraceCollector:: ~BaseTraceCollector()
{
}

// ============================================================================
/*!
 * Constructor: no need of LocalTraceBufferPool object initialization here,
 * thread safe singleton used in LocalTraceBufferPool::instance()
 * See derived classes.
 */
// ============================================================================

BaseTraceCollector::BaseTraceCollector()
{
}


