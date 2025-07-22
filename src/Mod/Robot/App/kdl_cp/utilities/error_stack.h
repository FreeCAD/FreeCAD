/***************************************************************************
  tag: Erwin Aertbelien  Mon Jan 10 16:38:39 CET 2005  error_stack.h 

                        error_stack.h -  description
                           -------------------
    begin                : Mon January 10 2005
    copyright            : (C) 2005 Erwin Aertbelien
    email                : erwin.aertbelien@mech.kuleuven.ac.be
 
 ***************************************************************************
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2.1 of the License, or (at your option) any later version.    *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/
 
 
/** 
 * \file 
 * \author Erwin Aertbelien, Div. PMA, Dep. of Mech. Eng., K.U.Leuven
 * \version 
 *      ORO_Geometry V0.2
 *   
 * \par history
 *   - changed layout of the comments to accommodate doxygen
 */
#ifndef ERROR_STACK_H
#define ERROR_STACK_H

#include "utility.h"
#include "utility_io.h"
#include <string>


namespace KDL {

/*
 * \todo
 *    IOTrace-routines store in static memory, should be in thread-local memory.
 * pushes a description of the current routine on the IO-stack trace
 */
void IOTrace(const std::string& description);

//! pops a description of the IO-stack
void IOTracePop();


//! outputs the IO-stack to a stream to provide a better errormessage.
void IOTraceOutput(std::ostream& os);

//! outputs one element of the IO-stack to the buffer (maximally size chars)
//! returns empty string if no elements on the stack.
void IOTracePopStr(char* buffer,int size);


}

#endif

