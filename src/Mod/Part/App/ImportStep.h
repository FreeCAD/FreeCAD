/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef _ImportStep_h_
#define _ImportStep_h_

#include <Mod/Part/PartGlobal.h>

namespace App {
class Document;
}

namespace Part
{

/** The part shape property
 */
PartExport int ImportStepParts(App::Document *pcDoc, const char* Name);

inline std::list<std::string> supportedSTEPSchemes() {
    std::list<std::string> schemes;
    schemes.emplace_back("AP203");
    schemes.emplace_back("AP214CD");
    schemes.emplace_back("AP214DIS");
    schemes.emplace_back("AP214IS");
    schemes.emplace_back("AP242DIS");
    return schemes;
}

} //namespace Part



#endif
