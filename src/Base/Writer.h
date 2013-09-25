/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de)          *
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

#ifndef BASE_WRITER_H
#define BASE_WRITER_H


#include <string>
#include <sstream>
#include <vector>
#include <cassert>

#include <zipios++/zipios-config.h>
#include <zipios++/zipfile.h>
#include <zipios++/zipinputstream.h>
#include <zipios++/zipoutputstream.h>
#include <zipios++/meta-iostreams.h>

#include "FileInfo.h"



namespace Base
{

class Persistence;


/** The Writer class 
 * This is an important helper class for the store and retrieval system
 * of persistent objects in FreeCAD.
 * \see Base::Persistence
 * \author Juergen Riegel
 */
class BaseExport Writer
{

public:
    Writer(void);
    virtual ~Writer();

    /// switch the writer in XML only mode (no files allowed)
    void setForceXML(bool on);
    /// check on state
    bool isForceXML(void);
    void setFileVersion(int);
    int getFileVersion() const;

    /// insert a file as CDATA section in the XML file
    void insertAsciiFile(const char* FileName);
    /// insert a binary file BASE64 coded as CDATA section in the XML file
    void insertBinFile(const char* FileName);

    /** @name additional file writing */
    //@{
    /// add a write request of a persistent object
    std::string addFile(const char* Name, const Base::Persistence *Object);
    /// process the requested file storing
    virtual void writeFiles(void)=0;
    /// get all registered file names
    const std::vector<std::string>& getFilenames() const;
    //@}

    /** @name pretty formating for XML */
    //@{
    /// get the current indention
    const char* ind(void) const {return indBuf;}
    /// increase indention by one tab
    void incInd(void);
    /// decrease indention by one tab
    void decInd(void);
    //@}

    virtual std::ostream &Stream(void)=0;

    /// name for underlying file saves
    std::string ObjectName;

protected:
    std::string getUniqueFileName(const char *Name);
    struct FileEntry {
        std::string FileName;
        const Base::Persistence *Object;
    };
    std::vector<FileEntry> FileList;
    std::vector<std::string> FileNames;

    short indent;
    char indBuf[256];

    bool forceXML;
    int fileVersion;
};


/** The ZipWriter class 
 * This is an important helper class implementation for the store and retrieval system
 * of persistent objects in FreeCAD. 
 * \see Base::Persistence
 * \author Juergen Riegel
 */
class BaseExport ZipWriter : public Writer
{

public:
    ZipWriter(const char* FileName);
    ZipWriter(std::ostream&);
    ~ZipWriter();

    virtual void writeFiles(void);

    virtual std::ostream &Stream(void){return ZipStream;}

    void setComment(const char* str){ZipStream.setComment(str);}
    void setLevel(int level){ZipStream.setLevel( level );}
    void putNextEntry(const char* str){ZipStream.putNextEntry(str);}

private:
    zipios::ZipOutputStream ZipStream;
};

/** The StringWriter class 
 * This is an important helper class implementation for the store and retrieval system
 * of objects in FreeCAD. 
 * \see Base::Persistence
 * \author Juergen Riegel
 */
class BaseExport StringWriter : public Writer
{

public:
    virtual std::ostream &Stream(void){return StrStream;}
    std::string getString(void){return StrStream.str();}
    virtual void writeFiles(void){assert(0);}

private:
    std::stringstream StrStream;
};


}  //namespace Base


#endif // BASE_WRITER_H
