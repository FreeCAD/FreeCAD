/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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


#include <set>
#include <unordered_set>
#include <string>
#include <sstream>
#include <vector>
#include <cassert>
#include <memory>

#ifdef _MSC_VER
#include <zipios++/zipios-config.h>
#endif
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
    Writer(short indent_size=2);
    virtual ~Writer();

    /// switch the writer in XML only mode (no files allowed)
    void setForceXML(int on);
    /// check on state
    int isForceXML();

    /// split xml among each object
    void setSplitXML(bool on);
    /// check whether to split xml among each object
    bool isSplitXML();

    /// set preference of binary output
    void setPreferBinary(bool on);
    bool isPreferBinary() const;

    void setFileVersion(int);
    int getFileVersion() const;

    /// put the next entry with a give name
    virtual void putNextEntry(const char *filename, const char *objName=0);

    /// insert a file as CDATA section in the XML file
    void insertAsciiFile(const char* FileName);
    /// insert a binary file BASE64 coded as CDATA section in the XML file
    void insertBinFile(const char* FileName, unsigned base64_line_size=80);
    /// insert text string as CDATA 
    void insertText(const std::string &s);

    /** @name additional file writing */
    //@{
    /// add a write request of a persistent object
    const std::string &addFile(const char* Name, const Base::Persistence *Object);
    /// add a write request of a persistent object
    const std::string &addFile(const std::string &Name, const Base::Persistence *Object) {
        return addFile(Name.c_str(),Object);
    }
    /// process the requested file storing
    virtual void writeFiles()=0;
    /// get all registered file names
    const std::vector<std::string>& getFilenames() const;
    /// Set mode
    void setMode(const std::string& mode);
    /// Set modes
    void setModes(const std::set<std::string>& modes);
    /// Get mode
    bool getMode(const std::string& mode) const;
    /// Get modes
    std::set<std::string> getModes() const;
    /// Clear mode
    void clearMode(const std::string& mode);
    /// Clear modes
    void clearModes();
    //@}

    /// Obtain the current file name
    const char *getCurrentFileName() const {
        return ObjectName.c_str();
    }

    /** @name Error handling */
    //@{
    void addError(const std::string&);
    bool hasErrors() const;
    void clearErrors();
    std::vector<std::string> getErrors() const;
    //@}

    /** @name pretty formatting for XML */
    //@{
    /// get the current indentation
    const char* ind() const {return indBuf;}
    /// increase indentation by one tab
    void incInd();
    /// decrease indentation by one tab
    void decInd();
    //@}

    virtual std::ostream &Stream()=0;

    /** Create an output stream for storing character content
     * @param base64: If true, the input will be base64 encoded before storing.
     *                If false, the input is assumed to be valid character with
     *                the current XML encoding, and will be enclosed inside
     *                CDATA section.  The stream will scan the input and
     *                properly escape any CDATA ending inside.
     * @param line_size: specifies the line size of the base64 output
     * @return Returns an output stream.
     *
     * You must call endCharStream() to end the current character stream.
     */
    std::ostream &beginCharStream(bool base64, unsigned line_size=80);
    /** End the current character output stream
     * @return Returns the normal writer stream for convenience
     */
    std::ostream &endCharStream();
    /// Return the current character output stream
    std::ostream &charStream();

protected:
    std::string getUniqueFileName(const char *Name);
    struct FileEntry {
        std::string FileName;
        const Base::Persistence *Object;
    };
    std::vector<FileEntry> FileList;
    std::vector<std::string> FileNames;
    std::unordered_set<std::string> FileNameSet;
    std::vector<std::string> Errors;
    std::set<std::string> Modes;

    int indent;
    short indent_size;
    char indBuf[65];

    int forceXML;
    bool splitXML;
    bool preferBinary;

    int fileVersion;

private:
    Writer(const Writer&);
    Writer& operator=(const Writer&);

private:
    std::string ObjectName;
    std::unique_ptr<std::ostream> CharStream;
    bool CharBase64 = false;
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
    virtual ~ZipWriter();

    virtual void writeFiles();

    virtual std::ostream &Stream(){return ZipStream;}

    void setComment(const char* str){ZipStream.setComment(str);}
    void setLevel(int level){ZipStream.setLevel( level );}
    virtual void putNextEntry(const char *filename, const char *objName=0);

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
    StringWriter();

    virtual std::ostream &Stream(){return StrStream;}
    std::string getString() const {return StrStream.str();}
    virtual void writeFiles();
    void clear() { StrStream.str(""); }

private:
    std::ostringstream StrStream;
};

/*! The FileWriter class
  This class writes out the data into files into a given directory name.
  \see Base::Persistence
  \author Werner Mayer
 */
class BaseExport FileWriter : public Writer
{
public:
    FileWriter(const char* DirName);
    virtual ~FileWriter();

    virtual void putNextEntry(const char *filename, const char *objName=nullptr);
    virtual void writeFiles();

    virtual std::ostream &Stream(){return FileStream;}
    void close() {FileStream.close();}
    /*!
     This method can be re-implemented in sub-classes to avoid
     to write out certain objects. The default implementation
     always returns true.
     */
    virtual bool shouldWrite(const std::string& name, const Base::Persistence *Object) const;

protected:
    std::string DirName;
    std::ofstream FileStream;
};


}  //namespace Base


#endif // BASE_WRITER_H
