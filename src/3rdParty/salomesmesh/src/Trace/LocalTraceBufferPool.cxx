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

//  Author : Paul RASCLE (EDF)
//  Module : KERNEL
//  $Header$
// Cf. C++ Users Journal, June 2004, Tracing Application Execution, Tomer Abramson
//
#include <iostream>
#include <limits.h>
#include <cassert>
#include <string.h>
#include <cstdio>

#ifndef WIN32
#include <dlfcn.h>
#else
#include <windows.h>
#endif

//#define _DEVDEBUG_
#include "LocalTraceBufferPool.hxx"
#include "BaseTraceCollector.hxx"
#include "LocalTraceCollector.hxx"
#include "FileTraceCollector.hxx"
#include "utilities.h"

// In case of truncated message, end of trace contains "...\n\0"

#define TRUNCATED_MESSAGE "...\n"
#define MAXMESS_LENGTH MAX_TRACE_LENGTH-5

// Class static attributes initialisation

LocalTraceBufferPool* LocalTraceBufferPool::_singleton = 0;
//#ifndef WIN32
//pthread_mutex_t LocalTraceBufferPool::_singletonMutex;
//#else
pthread_mutex_t LocalTraceBufferPool::_singletonMutex =
  PTHREAD_MUTEX_INITIALIZER;
//#endif
BaseTraceCollector *LocalTraceBufferPool::_myThreadTrace = 0;

// ============================================================================
/*!
 *  Guarantees a unique object instance of the class (singleton thread safe).
 *  When the LocalTraceBufferPool instance is created, the trace collector is
 *  also created (singleton). Type of trace collector to create depends on 
 *  environment variable "SALOME_trace":
 *  - "local" implies standard err trace, LocalTraceCollector is launched.
 *  - "file" implies trace in /tmp/tracetest.log
 *  - "file:pathname" implies trace in file pathname
 *  - anything else like "other" : try to load dynamically a library named
 *    otherTraceCollector, and invoque C method instance() to start a singleton
 *    instance of the trace collector. Example: with_loggerTraceCollector, for
 *    CORBA Log.
 */
// ============================================================================

LocalTraceBufferPool* LocalTraceBufferPool::instance()
{
  if (_singleton == 0) // no need of lock when singleton already exists
    {
      int ret;
      ret = pthread_mutex_lock(&_singletonMutex); // acquire lock to be alone
      if (_singleton == 0)                     // another thread may have got
        {                                      // the lock after the first test
          DEVTRACE("New buffer pool");
          LocalTraceBufferPool* myInstance = new LocalTraceBufferPool(); 

          new DESTRUCTOR_OF<LocalTraceBufferPool> (*myInstance);
          _singleton = myInstance;

          // --- start a trace Collector

          char* traceKind = getenv("SALOME_trace");

          if ( !traceKind || strcmp(traceKind,"local")==0 ) // mkr : 27.11.2006 : PAL13967 - Distributed supervision graphs - Problem with "SALOME_trace"
            {
              _myThreadTrace = LocalTraceCollector::instance();
            }
          else if (strncmp(traceKind,"file",strlen("file"))==0)
            {
              const char *fileName;
              if (strlen(traceKind) > strlen("file"))
                fileName = &traceKind[strlen("file")+1];
              else
                fileName = "/tmp/tracetest.log";
              
              _myThreadTrace = FileTraceCollector::instance(fileName);
            }
          else // --- try a dynamic library
            {
#ifndef WIN32
              void* handle;
              std::string impl_name = std::string ("lib") + traceKind 
#ifdef __APPLE__
                + std::string("TraceCollector.dylib");
#else
                + std::string("TraceCollector.so");
#endif
              handle = dlopen( impl_name.c_str() , RTLD_LAZY | RTLD_GLOBAL ) ;
#else
              HINSTANCE handle;
              std::string impl_name = std::string ("lib") + traceKind + std::string(".dll");
			  handle = LoadLibraryA( impl_name.c_str() );
#endif
              if ( handle )
                {
                  typedef BaseTraceCollector * (*FACTORY_FUNCTION) (void);
#ifndef WIN32
                  FACTORY_FUNCTION TraceCollectorFactory =
                    (FACTORY_FUNCTION) dlsym(handle, "SingletonInstance");
#else
                  FACTORY_FUNCTION TraceCollectorFactory =
                    (FACTORY_FUNCTION)GetProcAddress(handle, "SingletonInstance");
#endif
                  if ( !TraceCollectorFactory )
                  {
					  std::cerr << "Can't resolve symbol: SingletonInstance" <<std::endl;
#ifndef WIN32
                      std::cerr << "dlerror: " << dlerror() << std::endl;
#endif
                      exit( 1 );
                    }
                  _myThreadTrace = (TraceCollectorFactory) ();
                }
              else
                {
                  std::cerr << "library: " << impl_name << " not found !" << std::endl;
                  assert(handle); // to give file and line
                  exit(1);        // in case assert is deactivated
                }             
            }
          DEVTRACE("New buffer pool: end");
        }
      ret = pthread_mutex_unlock(&_singletonMutex); // release lock
    }
  return _singleton;
}

// ============================================================================
/*!
 *  Called by trace producers within their threads. The trace message is copied
 *  in specific buffer from a circular pool of buffers.
 *  Waits until there is a free buffer in the pool, gets the first available
 *  buffer, fills it with the message.
 *  Messages are printed in a separate thread (see retrieve method)
 */
// ============================================================================

int LocalTraceBufferPool::insert(int traceType, const char* msg)
{

  // get immediately a message number to control sequence (mutex protected)

  unsigned long myMessageNumber = lockedIncrement(_position);

  // wait until there is a free buffer in the pool

#ifdef __APPLE__
  dispatch_semaphore_wait(_freeBufferSemaphore, DISPATCH_TIME_FOREVER);
#else
  int ret = -1;
  while (ret)
    {
      ret = sem_wait(&_freeBufferSemaphore);
      if (ret) perror(" LocalTraceBufferPool::insert, sem_wait");
    }
#endif
  // get the next free buffer available (mutex protected) 

  unsigned long myInsertPos = lockedIncrement(_insertPos);

  // fill the buffer with message, thread id and type (normal or abort)

  strncpy(_myBuffer[myInsertPos%TRACE_BUFFER_SIZE].trace,
          msg,
          MAXMESS_LENGTH); // last chars always "...\n\0" if msg too long
  _myBuffer[myInsertPos%TRACE_BUFFER_SIZE].threadId =pthread_self();//thread id
  _myBuffer[myInsertPos%TRACE_BUFFER_SIZE].traceType = traceType;
  _myBuffer[myInsertPos%TRACE_BUFFER_SIZE].position = myMessageNumber;


  // increment the full buffer semaphore
  // (if previously 0, awake thread in charge of trace)

#ifdef __APPLE__
  dispatch_semaphore_signal(_fullBufferSemaphore);
#else
  ret = sem_post(&_fullBufferSemaphore);
#endif


  // returns the number of free buffers
#ifdef __APPLE__
  return 0;
#else
  sem_getvalue(&_freeBufferSemaphore, &ret);
  return ret;  
#endif
}

// ============================================================================
/*!
 *  Called by the thread in charge of printing trace messages.
 *  Waits until there is a buffer with a message to print.
 *  Gets the first buffer to print, copies it int the provided buffer
 */
// ============================================================================

int LocalTraceBufferPool::retrieve(LocalTrace_TraceInfo& aTrace)
{

  // wait until there is a buffer in the pool, with a message to print

#ifdef __APPLE__
    dispatch_semaphore_wait(_fullBufferSemaphore, DISPATCH_TIME_FOREVER);
#else
  int ret = -1;
  while (ret)
    {
      ret = sem_wait(&_fullBufferSemaphore);
      if (ret) MESSAGE (" LocalTraceBufferPool::retrieve, sem_wait");
    }
#endif
  // get the next buffer to print

  unsigned long myRetrievePos = lockedIncrement(_retrievePos);

  // copy the buffer from the pool to the provided buffer

  memcpy((void*)&aTrace,
         (void*)&_myBuffer[myRetrievePos%TRACE_BUFFER_SIZE],
         sizeof(aTrace));

  // increment the free buffer semaphore
  // (if previously 0, awake one of the threads waiting to put a trace, if any)
  // there is no way to preserve the order of waiting threads if several
  // threads are waiting to put a trace: the waken up thread is not
  // necessarily the first thread to wait.

#ifdef __APPLE__
  dispatch_semaphore_signal(_freeBufferSemaphore);
#else
  ret = sem_post(&_freeBufferSemaphore);
#endif

  // returns the number of full buffers

#ifdef __APPLE__
  return 0;
#else
  sem_getvalue(&_fullBufferSemaphore, &ret);
  return ret;
#endif
}

// ============================================================================
/*!
 *  Gives the number of buffers to print.
 *  Usage : when the thread in charge of messages print id to be stopped,
 *  check if there is still something to print, before stop.
 *  There is no need of mutex here, provided there is only one thread to
 *  retrieve and print the buffers.
 */
// ============================================================================

unsigned long LocalTraceBufferPool::toCollect()
{
  return _insertPos - _retrievePos;
}

// ============================================================================
/*!
 * Constructor : initialize pool of buffers, semaphores and mutex.
 */
// ============================================================================

LocalTraceBufferPool::LocalTraceBufferPool()
{
  //cerr << "LocalTraceBufferPool::LocalTraceBufferPool()" << endl;

  _insertPos   = ULONG_MAX;  // first increment will give 0
  _retrievePos = ULONG_MAX;
  _position=0;               // first message will have number = 1

  memset(_myBuffer, 0, sizeof(_myBuffer)); // to guarantee end of strings = 0
  for (int i=0; i<TRACE_BUFFER_SIZE; i++)
    strcpy(&(_myBuffer[i].trace[MAXMESS_LENGTH]),TRUNCATED_MESSAGE);
  int ret;
#ifdef __APPLE__
  dispatch_semaphore_t* sem1 = &_freeBufferSemaphore, *sem2 = &_fullBufferSemaphore;
  *sem1 = dispatch_semaphore_create(TRACE_BUFFER_SIZE);
  *sem2 = dispatch_semaphore_create(0);
#else
  ret=sem_init(&_freeBufferSemaphore, 0, TRACE_BUFFER_SIZE); // all buffer free
  if (ret!=0) IMMEDIATE_ABORT(ret);
  ret=sem_init(&_fullBufferSemaphore, 0, 0);                 // 0 buffer full
  if (ret!=0) IMMEDIATE_ABORT(ret);
#endif
  ret=pthread_mutex_init(&_incrementMutex,NULL); // default = fast mutex
  if (ret!=0) IMMEDIATE_ABORT(ret);

  //cerr << "LocalTraceBufferPool::LocalTraceBufferPool()-end" << endl;
}

// ============================================================================
/*!
 * Destructor : release memory associated with semaphores and mutex
 */
// ============================================================================

LocalTraceBufferPool::~LocalTraceBufferPool()
{
  int ret = pthread_mutex_lock(&_singletonMutex); // acquire lock to be alone
  if (_singleton)
    {
      DEVTRACE("LocalTraceBufferPool::~LocalTraceBufferPool()");
      delete (_myThreadTrace);
      _myThreadTrace = 0;
      int ret;
#ifdef __APPLE__
      dispatch_release(_freeBufferSemaphore);
      dispatch_release(_fullBufferSemaphore);
#else
      ret=sem_destroy(&_freeBufferSemaphore);
      ret=sem_destroy(&_fullBufferSemaphore);
#endif
      ret=pthread_mutex_destroy(&_incrementMutex);
      DEVTRACE("LocalTraceBufferPool::~LocalTraceBufferPool()-end");
      _singleton = 0;
    }
  ret = pthread_mutex_unlock(&_singletonMutex); // release lock
}

// ============================================================================
/*!
 * pool counters are incremented under a mutex protection
 */
// ============================================================================

unsigned long LocalTraceBufferPool::lockedIncrement(unsigned long& pos)
{
  int ret;
  ret = pthread_mutex_lock(&_incrementMutex);   // lock access to counters
  unsigned long mypos = ++pos;
  ret = pthread_mutex_unlock(&_incrementMutex); // release lock
  return mypos;
}
