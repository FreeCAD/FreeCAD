//  SALOME Utils : general SALOME's definitions and tools
//
//  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
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
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//
//
//  File   : utilities.h
//  Author : Antoine YESSAYAN, Paul RASCLE, EDF
//  Module : SALOME
//  $Header: /home/server/cvs/KERNEL/KERNEL_SRC/src/SALOMELocalTrace/utilities.h,v 1.6.2.1 2007/01/22 13:51:27 prascle Exp $

// switch off massaging in release:
#define _NOMSG_

/* ---  Definition macros file to print informations if _DEBUG_ or _DEBUG is defined --- */

#ifndef UTILITIES_H
#define UTILITIES_H

#include <iostream>
#include <sstream>
#include <cstdlib>

//Can be redefined to reflect another buffer such as log, cerr or a custom one
#define DEF_MSG_BUFFER std::cout

/*!
 * For each message to put in the trace, a specific ostingstream object is
 * created and destroyed automatically at the end of the message macro.
 * This message is automatically passed to std::cout
 */

#define MESS_INIT(deb) std::ostringstream os; os<<deb
#define MESS_BEGIN(deb) MESS_INIT(deb)<<__FILE__<<" ["<<__LINE__<<"] : "

#define MESS_END std::endl; \
DEF_MSG_BUFFER << os.str() << std::endl;

#define MESS_ABORT std::endl; \
DEF_MSG_BUFFER << os.str() << std::endl;

// --- Some macros are always defined (without _DEBUG_): for use with release version

#define INFOS(msg) {MESS_BEGIN("- Trace ") << msg << MESS_END}
#define PYSCRIPT(msg) {MESS_INIT("---PYSCRIPT--- ") << msg << MESS_END}
#define INTERRUPTION(msg) {MESS_BEGIN("- INTERRUPTION: ")<< msg << MESS_ABORT}
#ifdef WNT
#define IMMEDIATE_ABORT(code) {std::cout <<std::flush; \
                               std::cerr << "- ABORT " << __FILE__ << " [" <<__LINE__<< "] : " << flush; \
                               std::cerr << "ABORT return code= "<< code << std::endl; \
                               exit(code);}
#else
#define IMMEDIATE_ABORT(code) {std::cout <<std::flush; \
                               std::cerr << "- ABORT " << __FILE__ << " [" <<__LINE__<< "] : " << flush; \
                               std::cerr << "ABORT return code= "<< code << std::endl; \
                               std::exit(code);}
#endif

/* --- To print date and time of compilation of current source --- */

#ifndef COMPILER
#if defined ( __GNUC__ )
#define COMPILER		"g++" 
#elif defined ( __sun )
#define COMPILER		"CC" 
#elif defined ( __KCC )
#define COMPILER		"KCC" 
#elif defined ( __PGI )
#define COMPILER		"pgCC" 
#elif defined ( __alpha )
#define COMPILER		"cxx"
#elif defined ( __BORLAND__ )
#define COMPILER		"bcc32"
#else
#define COMPILER		"undefined" 
#endif
#endif

#ifdef INFOS_COMPILATION
#error INFOS_COMPILATION already defined
#endif

#if defined(_DEBUG_) || defined (_DEBUG)

// --- the following MACROS are useful at debug time

#define INFOS_COMPILATION { MESS_BEGIN("COMPILED with ") << COMPILER \
				       << ", " << __DATE__ \
				       << " at " << __TIME__ << MESS_END }

#define MESSAGE(msg) {MESS_BEGIN("- Trace ") << msg << MESS_END}
#define SCRUTE(var)  {MESS_BEGIN("- Trace ") << #var << "=" << var <<MESS_END}

#define REPERE ("------- ")
#define BEGIN_OF(msg) {MESS_BEGIN(REPERE) << "Begin of: "      << msg << MESS_END} 
#define END_OF(msg)   {MESS_BEGIN(REPERE) << "Normal end of: " << msg << MESS_END} 

#ifndef ASSERT
#define ASSERT(condition) \
        if (!(condition)){INTERRUPTION("CONDITION "<<#condition<<" NOT VERIFIED")}
#endif /* ASSERT */


#elif !defined(_NOMSG_) /* ifdef _DEBUG_*/

#define INFOS_COMPILATION
#define MESSAGE(msg) {MESS_BEGIN("MSG:") << msg << MESS_END}
#define SCRUTE(var)  {MESS_BEGIN("SCRUTE:") << #var << " = " << var << MESS_END}
#define REPERE ("-------")
#define BEGIN_OF(msg) {MESS_BEGIN("MSG BEGIN:") << msg << MESS_END}
#define END_OF(msg) {MESS_BEGIN("MSG END:") << msg << MESS_END}

#ifndef ASSERT
#define ASSERT(condition) {}
#endif /* ASSERT */

#else  /* ifdef _DEBUG_*/

#define INFOS_COMPILATION
#define MESSAGE(msg) {}
#define SCRUTE(var) {}
#define REPERE
#define BEGIN_OF(msg)
#define END_OF(msg)

#ifndef ASSERT
#define ASSERT(condition) {}
#endif /* ASSERT */

#endif /* ifdef _DEBUG_*/

#endif /* ifndef UTILITIES_H */
