/***************************************************************************
 *   Copyright (c) Riegel         <juergen.riegel@web.de>                  *
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
#include "Writer.h"
#include "Reader.h"
#include "PyObjectBase.h"

#ifndef _PreComp_
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "Persistence.h"

using namespace Base;

TYPESYSTEM_SOURCE_ABSTRACT(Base::Persistence,Base::BaseClass);


//**************************************************************************
// Construction/Destruction



//**************************************************************************
// separator for other implementation aspects

unsigned int Persistence::getMemSize (void) const
{
    // you have to implement this method in all descending classes!
    assert(0);
    return 0;
}

void Persistence::Save (Writer &/*writer*/) const
{
    // you have to implement this method in all descending classes!
    assert(0);
}

void Persistence::Restore(XMLReader &/*reader*/)
{
    // you have to implement this method in all descending classes!
    assert(0);
}

void Persistence::SaveDocFile (Writer &/*writer*/) const
{
}

void Persistence::RestoreDocFile(Reader &/*reader*/)
{
}

std::string Persistence::encodeAttribute(const std::string& str)
{
    std::string tmp;
    for (std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
        if (*it == '<')
            tmp += "&lt;";
        else if (*it == '\"')
            tmp += "&quot;";
        else if (*it == '\'')
            tmp += "&apos;";
        else if (*it == '&')
            tmp += "&amp;";
        else if (*it == '>')
            tmp += "&gt;";
        else if (*it == '\r')
            tmp += "&#13;";
        else if (*it == '\n')
            tmp += "&#10;";
        else if (*it == '\t')
            tmp += "&#9;";
        else
            tmp += *it;
    }

    return tmp;
}

void Persistence::dumpToStream(std::ostream& stream, int compression)
{
    //we need to close the zipstream to get a good result, the only way to do this is to delete the ZipWriter. 
    //Hence the scope...
    {
        //create the writer
        Base::ZipWriter writer(stream);
        writer.setLevel(compression);
        writer.putNextEntry("Persistence.xml");
        writer.setMode("BinaryBrep");

        //save the content (we need to encapsulte it with xml tags to be able to read single element xmls like happen for properties)
        writer.Stream() << "<Content>" << std::endl;
        Save(writer);
        writer.Stream() << "</Content>";
        writer.writeFiles();
    }
}

void Persistence::restoreFromStream(std::istream& stream)
{
    zipios::ZipInputStream zipstream(stream);
    Base::XMLReader reader("", zipstream);

    if (!reader.isValid())
        throw Base::ValueError("Unable to construct reader");

    reader.readElement("Content");
    Restore(reader);
    reader.readFiles(zipstream);
    restoreFinished();
}
