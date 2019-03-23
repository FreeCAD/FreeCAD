/***************************************************************************
 *   (c) Jürgen Riegel (juergen.riegel@web.de) 2006                        *   
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        * 
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with FreeCAD; if not, write to the Free Software        * 
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 *   Juergen Riegel 2006                                                   *
 ***************************************************************************/

/** \defgroup MemDebug Memory debugging
 *  \ingroup BASE
 *  \brief Memory debugging tools
 * \section Overview
 * In C++ applications there are a lot of ways to handle memory allocation and deallocation.
 * As many ways to do it wrong or simply forget to free memory. One way to overcome
 * this problem is e.g. usage of handle classes (like OpenCASCADE does) or use a lot of factories.
 * But all of them have drawbacks or performance penalties. One good way to get memory
 * problems hunted down is the MSCRT Heap debugging facility. This set of functions
 * opens the possibility to track and locate all kind of memory problems, e.g.
 * memory leaks.
 *
 * \section Implementation
 * The FreeCAD memory debugging is located in the Base::MemDebug class.
 */

#include "PreCompiled.h"

#ifndef _PreComp_
# ifdef _MSC_VER
#  include <cstdio>
#  include <time.h>
#  include <windows.h>
#  include <crtdbg.h>
# endif
#endif


/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "MemDebug.h"

using namespace Base;

#if defined(_MSC_VER)


/** Memory debugging class
 * This class is an interface to the Windows CRT debugging
 * facility. If the define MemDebugOn in the src/FCConfig.h is 
 * set the class gets instantiated globally and tracks all memory allocations on the heap. 
 * The result gets written in the MemLog.txt in the active directory.
 *  \par
 * NOTE: you must not instantiate this class! 
 *  
 *
 * \author Juergen Riegel
 */
class MemDebug
{
public:
  /// Construction
  MemDebug();
  /// Destruction
  virtual ~MemDebug();

protected:
  static FILE *logFile;

  /** @name static callbacks for the Crt */
  //@{
  static void __cdecl sDumpClientHook(void * pUserData, size_t nBytes);
  static int  __cdecl sAllocHook(int nAllocType, void* pvData, size_t nSize,int nBlockUse,long lRequest,const unsigned char * szFileName,int nLine);
  static int          sReportHook(int   nRptType,char *szMsg,int  *retVal);
  //@}
};

// the one and only MemDebug instance. 
#ifdef MemDebugOn
MemDebug cSingelton;
#endif


#define  SET_CRT_DEBUG_FIELD(a)   _CrtSetDbgFlag((a) | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))
#define  CLEAR_CRT_DEBUG_FIELD(a) _CrtSetDbgFlag(~(a) & _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))

FILE *MemDebug::logFile = NULL;

//**************************************************************************
// Construction/Destruction


MemDebug::MemDebug()
{
   //_CrtMemState checkPt1;
   char timeStr[15], dateStr[15];         // Used to set up log file


   // Send all reports to STDOUT, since this example is a console app
   _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE );
   _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR );
   _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE );
   _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR );
   _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE );
   _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR );
 
   // Set the debug heap to report memory leaks when the process terminates,
   // and to keep freed blocks in the linked list.
   SET_CRT_DEBUG_FIELD( _CRTDBG_LEAK_CHECK_DF | _CRTDBG_DELAY_FREE_MEM_DF );

   // Open a log file for the hook functions to use 
   if ( logFile != NULL )
     throw "Base::MemDebug::MemDebug():38: Don't call the constructor by your self!";
#if (_MSC_VER >= 1400)
   fopen_s( &logFile, "MemLog.txt", "w" );
   if ( logFile == NULL )
     throw "Base::MemDebug::MemDebug():41: File IO Error. Can't open log file...";
   _strtime_s( timeStr, 15 );
   _strdate_s( dateStr, 15 );
#elif (_MSC_VER >= 1200)
   logFile = fopen( "MemLog.txt", "w" );
   if ( logFile == NULL )
     throw "Base::MemDebug::MemDebug():41: File IO Error. Can't open log file...";
   _strtime( timeStr );
   _strdate( dateStr );
#endif
   fprintf( logFile,
            "Memory Allocation Log File for FreeCAD, run at %s on %s.\n",
            timeStr, dateStr );
   fputs( "-------------------------------------------------------------------\n", logFile );

   // Install the hook functions
   _CrtSetDumpClient( sDumpClientHook );
   _CrtSetAllocHook( sAllocHook );
   _CrtSetReportHook( sReportHook );

}

MemDebug::~MemDebug()
{
  _CrtMemDumpAllObjectsSince( NULL );
  //_CrtCheckMemory( );

  // This fflush needs to be removed...
  fflush( logFile );
  fclose( logFile );
}


//**************************************************************************
// separator for other implementation aspects


/* REPORT HOOK FUNCTION
   --------------------
   Again, report hook functions can serve a very wide variety of purposes.
   This one logs error and assertion failure debug reports in the
   log file, along with 'Damage' reports about overwritten memory.

   By setting the retVal parameter to zero, we are instructing _CrtDbgReport
   to return zero, which causes execution to continue. If we want the function
   to start the debugger, we should have _CrtDbgReport return one.
*/
int MemDebug::sReportHook(int   nRptType,char *szMsg,int  *retVal)
{
   char *RptTypes[] = { "Warning", "Error", "Assert" };

   if ( ( nRptType > 0 ) || ( strstr( szMsg, "HEAP CORRUPTION DETECTED" ) ) )
      fprintf( logFile, "%s: %s", RptTypes[nRptType], szMsg );

   retVal = 0;

   return( 7 );         // Allow the report to be made as usual (True = 7, False = 0)
}

/* ALLOCATION HOOK FUNCTION
   -------------------------
   An allocation hook function can have many, many different
   uses. This one simply logs each allocation operation in a file.
*/
int __cdecl MemDebug::sAllocHook(
   int      nAllocType,
   void   * pvData,
   size_t   nSize,
   int      nBlockUse,
   long     lRequest,
   const unsigned char * szFileName,
   int      nLine
   )
{
  char *operation[] = { "       :", "Alloc  :", "Realloc:", "Free   :" };
   char *blockType[] = { "Free", "Normal", "CRT", "Ignore", "Client" };

   if ( nBlockUse == _CRT_BLOCK )   // Ignore internal C runtime library allocations
      return( 7 ); // (True = 7, False = 0)

   _ASSERT( ( nAllocType > 0 ) && ( nAllocType < 4 ) );
   _ASSERT( ( nBlockUse >= 0 ) && ( nBlockUse < 5 ) );

   if( nBlockUse !=4 )
     return(7);
   
   fprintf( logFile, 
            "%s (#%7d) %12Iu byte (%s) in %s line %d",
            operation[nAllocType],lRequest, nSize, blockType[nBlockUse],szFileName, nLine);
   if ( pvData != NULL )
      fprintf( logFile, " at %p\n", pvData );
   else
     fprintf( logFile, "\n" );

   return( 7 );         // Allow the memory operation to proceed (True = 7, False = 0)
}


/* CLIENT DUMP HOOK FUNCTION
   -------------------------
   A hook function for dumping a Client block usually reports some
   or all of the contents of the block in question.  The function
   below also checks the data in several ways, and reports corruption
   or inconsistency as an assertion failure.
*/
void __cdecl MemDebug::sDumpClientHook(
   void * pUserData,
   size_t nBytes
   )
{
   long requestNumber=0;
  _CrtIsMemoryBlock(pUserData,(unsigned int)nBytes,&requestNumber,NULL,NULL);
  fprintf( logFile, "Leak   : (#%7d) %12Iu bytes (%p)  \n", requestNumber, nBytes, pUserData );

}

// -----------------------------------------------------

MemCheck::MemCheck()
{
    // Store a memory checkpoint in the s1 memory-state structure
    _CrtMemCheckpoint( &s1 );
}

MemCheck::~MemCheck()
{
    // Store a 2nd memory checkpoint in s2
    _CrtMemCheckpoint( &s2 );
    if ( _CrtMemDifference( &s3, &s1, &s2 ) )
        _CrtMemDumpStatistics( &s3 );
}

void MemCheck::setNextCheckpoint()
{
    // Store a 2nd memory checkpoint in s2
    _CrtMemCheckpoint( &s2 );
    if ( _CrtMemDifference( &s3, &s1, &s2 ) )
        _CrtMemDumpStatistics( &s3 );

    // Store a memory checkpoint in the s1 memory-state structure
    _CrtMemCheckpoint( &s1 );
}

bool MemCheck::checkMemory()
{
    return _CrtCheckMemory() ? true : false;
}

bool MemCheck::dumpLeaks()
{
    return _CrtDumpMemoryLeaks() ? true : false;
}

bool MemCheck::isValidHeapPointer(const void* userData)
{
    return _CrtIsValidHeapPointer(userData) ? true : false;
}

#endif
