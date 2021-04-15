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


#include "PreCompiled.h"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <locale>


#include "Writer.h"
#include "Base64.h"
#include "Exception.h"
#include "FileInfo.h"
#include "Stream.h"
#include "Tools.h"
#include "Persistence.h"

using namespace Base;
using namespace std;
using namespace zipios;

// boost iostream filter to escape ']]>' in text file saved into CDATA section.
// It does not check if the character is valid utf8 or not.
struct cdata_filter {

    typedef char char_type;
    typedef bio::output_filter_tag category;

    template<typename Device>
    inline bool put(Device& dev, char c) {
        switch(state) {
        case 0:
        case 1:
            if(c == ']')
                ++state;
            else
                state = 0;
            break;
        case 2:
            if(c == '>') {
                static const char escape[] = "]]><![CDATA[";
                bio::write(dev,escape,sizeof(escape)-1);
            }
            state = 0;
            break;
        }
        return bio::put(dev,c);
    }

    int state = 0;
};

// ---------------------------------------------------------------------------
//  Writer: Constructors and Destructor
// ---------------------------------------------------------------------------

Writer::Writer(short indent_size)
  : indent(0)
  , indent_size(indent_size)
  , forceXML(0)
  , splitXML(false)
  , preferBinary(true)
  , fileVersion(1)
{
    indBuf[0] = '\0';
}

Writer::~Writer()
{
}

std::ostream &Writer::beginCharStream(bool base64, unsigned line_size) {
    if(CharStream)
        throw Base::RuntimeError("Writer::beginCharStream(): invalid state");
    CharBase64 = base64;
    if(base64) {
        CharStream = create_base64_encoder(Stream(),line_size);
    } else {
        Stream() << "<![CDATA[";
        CharStream.reset(new bio::filtering_ostream);
        auto f = static_cast<bio::filtering_ostream*>(CharStream.get());
        f->push(cdata_filter());
        f->push(Stream());
        *f << std::setprecision(std::numeric_limits<double>::digits10 + 1);
    }
    return *CharStream;
}

std::ostream &Writer::endCharStream() {
    if(CharStream) {
        CharStream.reset();
        if(!CharBase64)
            Stream() << "]]>";
    }
    return Stream();
}

std::ostream &Writer::charStream() {
    if(!CharStream)
        throw Base::RuntimeError("Writer::endCharStream(): no current character stream");
    return *CharStream;
}

void Writer::insertText(const std::string &s) {
    beginCharStream(false) << s;
    endCharStream();
}

void Writer::insertAsciiFile(const char* FileName)
{
    Base::FileInfo fi(FileName);
    Base::ifstream from(fi, std::ios::in | std::ios::binary);
    if (!from)
        throw Base::FileException("Writer::insertAsciiFile() Could not open file!");

    beginCharStream(false) << from.rdbuf();
    endCharStream();
}

void Writer::insertBinFile(const char* FileName, unsigned line_size)
{
    Base::FileInfo fi(FileName);
    Base::ifstream from(fi, std::ios::in | std::ios::binary);
    if (!from)
        throw Base::FileException("Writer::insertBinaryFile() Could not open file!");

    beginCharStream(true,line_size) << from.rdbuf();
    endCharStream();
}

void Writer::setForceXML(int on)
{
    forceXML = on;
}

int Writer::isForceXML()
{
    return forceXML;
}

void Writer::setSplitXML(bool on)
{
    splitXML = on;
}

bool Writer::isSplitXML()
{
    return splitXML;
}

void Writer::setPreferBinary(bool on)
{
    preferBinary = on;
}

bool Writer::isPreferBinary() const
{
    return preferBinary;
}

void Writer::setFileVersion(int v)
{
    fileVersion = v;
}

int Writer::getFileVersion() const
{
    return fileVersion;
}

void Writer::setMode(const std::string& mode)
{
    Modes.insert(mode);
}

void Writer::setModes(const std::set<std::string>& modes)
{
    Modes = modes;
}

bool Writer::getMode(const std::string& mode) const
{
    std::set<std::string>::const_iterator it = Modes.find(mode);
    return (it != Modes.end());
}

std::set<std::string> Writer::getModes() const
{
    return Modes;
}

void Writer::clearMode(const std::string& mode)
{
    std::set<std::string>::iterator it = Modes.find(mode);
    if (it != Modes.end())
        Modes.erase(it);
}

void Writer::clearModes()
{
    Modes.clear();
}

void Writer::addError(const std::string& msg)
{
    Errors.push_back(msg);
}

bool Writer::hasErrors() const
{
    return (!Errors.empty());
}

void Writer::clearErrors()
{
    Errors.clear();
}

std::vector<std::string> Writer::getErrors() const
{
    return Errors;
}

const std::string &Writer::addFile(const char* Name,const Base::Persistence *Object)
{
    assert(Name);

    FileList.emplace_back();
    FileEntry &entry = FileList.back();

    if(!FileNameSet.insert(Name).second) {
        entry.FileName = getUniqueFileName(Name);
        FileNameSet.insert(entry.FileName);
    } else
        entry.FileName = Name;

    FileNames.push_back(entry.FileName);

    entry.Object = Object;
    return entry.FileName;
}

std::string Writer::getUniqueFileName(const char *Name)
{
    std::vector<std::string> names;
    names.reserve(FileNames.size());
    FileInfo fi(Name);
    std::string CleanName = fi.fileNamePure();
    std::string ext = fi.extension();
    for (auto pos = FileNames.begin();pos != FileNames.end();++pos) {
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

const std::vector<std::string>& Writer::getFilenames() const
{
    return FileNames;
}

void Writer::incInd()
{
    int pos = sizeof(indBuf)-1;
    if(indent < pos)
        pos = indent;
    for(int i=0;i<indent_size && pos-1<(int)sizeof(indBuf);++i)
        indBuf[pos++] = ' ';
    indBuf[pos] = 0;
    indent += indent_size;
}

void Writer::decInd()
{
    if (indent >= indent_size) {
        indent -= indent_size;
    }
    else {
        indent = 0;
    }
    if(indent < (int)sizeof(indBuf))
        indBuf[indent] = '\0';
}

void Writer::putNextEntry(const char *file, const char *obj) {
    ObjectName = obj?obj:file;
}

// ----------------------------------------------------------------------------

ZipWriter::ZipWriter(const char* FileName)
  : ZipStream(FileName)
{
#ifdef _MSC_VER
    ZipStream.imbue(std::locale::empty());
#else
    ZipStream.imbue(std::locale::classic());
#endif
    ZipStream.precision(std::numeric_limits<double>::digits10 + 1);
    ZipStream.setf(ios::fixed,ios::floatfield);
}

ZipWriter::ZipWriter(std::ostream& os)
  : ZipStream(os)
{
#ifdef _MSC_VER
    ZipStream.imbue(std::locale::empty());
#else
    ZipStream.imbue(std::locale::classic());
#endif
    ZipStream.precision(std::numeric_limits<double>::digits10 + 1);
    ZipStream.setf(ios::fixed,ios::floatfield);
}

void ZipWriter::putNextEntry(const char *file, const char *obj) {
    Writer::putNextEntry(file,obj);

    ZipStream.putNextEntry(file);
}

void ZipWriter::writeFiles()
{
    // use a while loop because it is possible that while
    // processing the files new ones can be added
    size_t index = 0;
    while (index < FileList.size()) {
        FileEntry entry = FileList.begin()[index];
        putNextEntry(entry.FileName.c_str());
        indent = 0;
        indBuf[0] = 0;
        entry.Object->SaveDocFile(*this);
        index++;
    }
}

ZipWriter::~ZipWriter()
{
    ZipStream.close();
}

// ----------------------------------------------------------------------------

StringWriter::StringWriter() {
    setFileVersion(2);
    setForceXML(9999);
    setSplitXML(false);
    setPreferBinary(false);
    this->StrStream << std::setprecision(std::numeric_limits<double>::digits10 + 1);
}

void StringWriter::writeFiles() {
    if(FileList.size())
        throw Base::FileException("StringWriter does not support saving into multiple files");
}

// ----------------------------------------------------------------------------

FileWriter::FileWriter(const char* DirName) : DirName(DirName)
{
}

FileWriter::~FileWriter()
{
}

void FileWriter::putNextEntry(const char* file, const char *obj)
{
    Writer::putNextEntry(file,obj);

    std::string fileName = DirName + "/" + file;
    this->FileStream.open(fileName.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    this->FileStream << std::setprecision(std::numeric_limits<double>::digits10 + 1);
}

bool FileWriter::shouldWrite(const std::string& , const Base::Persistence *) const
{
    return true;
}

void FileWriter::writeFiles()
{
    // use a while loop because it is possible that while
    // processing the files new ones can be added
    size_t index = 0;
    this->FileStream.close();
    while (index < FileList.size()) {
        FileEntry entry = FileList[index];

        if (shouldWrite(entry.FileName, entry.Object)) {
            std::string filePath = entry.FileName;
            std::string::size_type pos = 0;
            while ((pos = filePath.find('/', pos)) != std::string::npos) {
                std::string dirName = DirName + "/" + filePath.substr(0, pos);
                pos++;
                Base::FileInfo fi(dirName);
                fi.createDirectory();
            }

            putNextEntry(entry.FileName.c_str());
            indent = 0;
            indBuf[0] = 0;
            entry.Object->SaveDocFile(*this);
            this->FileStream.close();
        }

        index++;
    }
}
