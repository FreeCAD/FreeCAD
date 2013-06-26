/***************************************************************************
 *   (c) Jürgen Riegel (juergen.riegel@web.de) 2008                        *
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

#ifndef _PreComp_
# include <QUuid>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "Uuid.h"
#include "Exception.h"
#include "Interpreter.h"
#include <stdexcept>
#include <CXX/Objects.hxx>


using namespace Base;


//**************************************************************************
// Construction/Destruction

/**
 * A constructor.
 * A more elaborate description of the constructor.
 */
Uuid::Uuid()
{
    _uuid = createUuid();
}

/**
 * A destructor.
 * A more elaborate description of the destructor.
 */
Uuid::~Uuid()
{
}

//**************************************************************************
//**************************************************************************
// Get the UUID
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
std::string Uuid::createUuid(void)
{
    std::string Uuid;
    QString uuid = QUuid::createUuid().toString();
    uuid = uuid.mid(1);
    uuid.chop(1);
    Uuid = (const char*)uuid.toAscii();
    return Uuid;
}

void Uuid::setValue(const char* sString) 
{ 
    if (sString) { 
        QUuid uuid(QString::fromAscii(sString)); 
        if (uuid.isNull()) 
            throw std::runtime_error("invalid uuid"); 
        // remove curly braces 
        QString id = uuid.toString(); 
        id = id.mid(1); 
        id.chop(1); 
        _uuid = (const char*)id.toAscii(); 
    } 
} 

void Uuid::setValue(const std::string &sString)
{
    setValue(sString.c_str());
}

const std::string& Uuid::getValue(void) const
{
    return _uuid;
}
