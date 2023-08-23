/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#include "FileTemplate.h"


using namespace Base;


//**************************************************************************
// Construction/Destruction

/**
 * A constructor.
 * A more elaborate description of the constructor.
 */
ClassTemplate::ClassTemplate() = default;

/**
 * A destructor.
 * A more elaborate description of the destructor.
 */
ClassTemplate::~ClassTemplate() = default;


//**************************************************************************
// separator for other implementation aspects

/**
 * a normal member taking two arguments and returning an integer value.
 * \par
 * You can use a printf like interface like:
 * \code
 * GetConsole().Warning("Some defects in %s, loading anyway\n",str);
 * \endcode
 * @param a an integer argument.
 * @param s a constant character pointer.
 * @see ClassTemplate()
 * @see ~ClassTemplate()
 * @see testMeToo()
 * @see publicVar()
 * @return The test results
 */
int ClassTemplate::testMe(int /*a*/,const char* /*s*/)
{
    return 0;
}


//**************************************************************************
//**************************************************************************
// Separator for additional classes
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



