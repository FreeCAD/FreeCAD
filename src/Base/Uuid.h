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


#ifndef BASE_UUID_H
#define BASE_UUID_H

// Std. configurations

#include <string>
#ifndef FC_GLOBAL_H
#include <FCGlobal.h>
#endif

namespace Base
{

/** Creates a Uuid
 * \author Jürgen Riegel
 */
class BaseExport Uuid
{
public:
    /// Construction
    Uuid();
    Uuid(const Uuid&) = default;
    Uuid& operator=(const Uuid&) = default;
    /// Destruction
    virtual ~Uuid();

    void setValue(const char* sString);
    void setValue(const std::string &sString);
    const std::string& getValue() const;
    static std::string createUuid();

    bool operator==(const Uuid &other) const {return _uuid == other._uuid;}
    bool operator<(const Uuid &other) const {return _uuid < other._uuid;}

private:
    std::string _uuid;
};

} //namespace Base

#endif // BASE_UUID_H

