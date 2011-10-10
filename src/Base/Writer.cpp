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

#ifndef _PreComp_
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "Writer.h"
#include "Persistence.h"
#include "Exception.h"
#include "Base64.h"
#include "FileInfo.h"
#include "Tools.h"

#include <algorithm>
#include <locale>

using namespace Base;
using namespace std;
using namespace zipios;



// ---------------------------------------------------------------------------
//  Writer: Constructors and Destructor
// ---------------------------------------------------------------------------

Writer::Writer(void)
  : indent(0),forceXML(false)
{
    indBuf[0] = '\0';
}

Writer::~Writer()
{
}

void Writer::insertAsciiFile(const char* FileName)
{
    std::ifstream from(FileName);
    if (!from) throw Base::Exception("Writer::insertAsciiFile() Could not open file!");

    Stream() << "<![CDATA[" << endl;
    char ch;
    while (from.get(ch)) Stream().put(ch);
    Stream() << endl << "]]>" << endl;
}

void Writer::insertBinFile(const char* FileName)
{
    std::ifstream from(FileName);
    if (!from) throw Base::Exception("Writer::insertAsciiFile() Could not open file!");

    Stream() << "<![CDATA[" << endl;

    unsigned char buf[60];
    std::string encoded;
    unsigned int i;

    while (from){
        for(i=0 ; i<60 && from;i++)
            buf[i] = from.get();
        Stream() << Base::base64_encode(buf,i) << endl;
    }

    Stream() << "]]>" << endl;
}

void Writer::setForceXML(bool on)
{
    forceXML = on;
}

bool Writer::isForceXML(void)
{
    return forceXML;
}

std::string Writer::addFile(const char* Name,const Base::Persistence *Object)
{
    // always check isForceXML() before requesting a file!
    assert(isForceXML()==false);

    FileEntry temp;
    temp.FileName = getUniqueFileName(Name);
    temp.Object = Object;
  
    FileList.push_back(temp);

    FileNames.push_back( temp.FileName );

    // return the unique file name
    return temp.FileName;
}

std::string Writer::getUniqueFileName(const char *Name)
{
    // name in use?
    std::string CleanName = (Name ? Name : "");
    std::vector<std::string>::const_iterator pos;
    pos = find(FileNames.begin(),FileNames.end(),CleanName);

    if (pos == FileNames.end()) {
        // if not, name is OK
        return CleanName;
    }
    else {
        std::vector<std::string> names;
        names.reserve(FileNames.size());
        FileInfo fi(CleanName);
        CleanName = fi.fileNamePure();
        std::string ext = fi.extension();
        for (pos = FileNames.begin();pos != FileNames.end();++pos) {
            fi.setFile(*pos);
            std::string FileName = fi.fileNamePure();
            if (fi.extension() == ext)
                names.push_back(FileName);
        }
        std::stringstream str;
        str << Base::Tools::getUniqueName(CleanName, names);
        if (!ext.empty())
            str << "." << ext;
        return str.str();
    }
}

const std::vector<std::string>& Writer::getFilenames() const
{
    return FileNames;
}

void Writer::incInd(void)
{
    if (indent < 255) {
        indBuf[indent] = '\t';
        indBuf[indent+1] = '\0';
        indent++;
    }
}

void Writer::decInd(void)
{
    if (indent > 0) {
        indent--;
        indBuf[indent] = '\0';
    }
}

ZipWriter::ZipWriter(const char* FileName) 
  : ZipStream(FileName)
{
#ifdef _MSC_VER
    ZipStream.imbue(std::locale::empty());
#else
    //FIXME: Check whether this is correct
    ZipStream.imbue(std::locale::classic());
#endif
}

ZipWriter::ZipWriter(std::ostream& os) 
  : ZipStream(os)
{
#ifdef _MSC_VER
    ZipStream.imbue(std::locale::empty());
#else
    //FIXME: Check whether this is correct
    ZipStream.imbue(std::locale::classic());
#endif
}

void ZipWriter::writeFiles(void)
{
    // use a while loop because it is possible that while
    // processing the files new ones can be added
    size_t index = 0;
    while (index < FileList.size()) {
        FileEntry entry = FileList.begin()[index];
        ZipStream.putNextEntry(entry.FileName);
        entry.Object->SaveDocFile(*this);
        index++;
    }
}

ZipWriter::~ZipWriter()
{
    ZipStream.close();
}
