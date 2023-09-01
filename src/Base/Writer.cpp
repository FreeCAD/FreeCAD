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

#include <limits>
#include <locale>
#include <iomanip>

#include "Writer.h"
#include "Base64.h"
#include "Exception.h"
#include "FileInfo.h"
#include "Persistence.h"
#include "Stream.h"
#include "Tools.h"

#include <boost/iostreams/filtering_stream.hpp>

using namespace Base;
using namespace std;
using namespace zipios;

// boost iostream filter to escape ']]>' in text file saved into CDATA section.
// It does not check if the character is valid utf8 or not.
struct cdata_filter {

    using char_type = char;
    using category = boost::iostreams::output_filter_tag;

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
                    boost::iostreams::write(dev,escape,sizeof(escape)-1);
                }
                state = 0;
                break;
        }
        return boost::iostreams::put(dev,c);
    }

    int state = 0;
};

// ---------------------------------------------------------------------------
//  Writer: Constructors and Destructor
// ---------------------------------------------------------------------------

Writer::Writer()
{
    indBuf[0] = '\0';
}

Writer::~Writer() = default;

std::ostream& Writer::beginCharStream()
{
    if (CharStream) {
        throw Base::RuntimeError("Writer::beginCharStream(): invalid state");
    }

    Stream() << "<![CDATA[";
    CharStream = std::make_unique<boost::iostreams::filtering_ostream>();
    auto* filteredStream = dynamic_cast<boost::iostreams::filtering_ostream*>(CharStream.get());
    filteredStream->push(cdata_filter());
    filteredStream->push(Stream());
    *filteredStream << std::setprecision(std::numeric_limits<double>::digits10 + 1);
    return *CharStream;
}

std::ostream& Writer::endCharStream()
{
    if (CharStream) {
        CharStream.reset();
        Stream() << "]]>";
    }
    return Stream();
}

std::ostream& Writer::charStream()
{
    if (!CharStream) {
        throw Base::RuntimeError("Writer::endCharStream(): no current character stream");
    }
    return *CharStream;
}

void Writer::insertText(const std::string& s)
{
    beginCharStream() << s;
    endCharStream();
}

void Writer::insertAsciiFile(const char* FileName)
{
    Base::FileInfo fi(FileName);
    Base::ifstream from(fi);
    if (!from)
        throw Base::FileException("Writer::insertAsciiFile() Could not open file!");

    Stream() << "<![CDATA[";
    char ch{};
    while (from.get(ch))
        Stream().put(ch);
    Stream() << "]]>" << endl;
}

void Writer::insertBinFile(const char* FileName)
{
    Base::FileInfo fi(FileName);
    Base::ifstream from(fi, std::ios::in | std::ios::binary | std::ios::ate);
    if (!from)
        throw Base::FileException("Writer::insertAsciiFile() Could not open file!");

    Stream() << "<![CDATA[";
    std::ifstream::pos_type fileSize = from.tellg();
    from.seekg(0, std::ios::beg);
    std::vector<unsigned char> bytes(static_cast<size_t>(fileSize));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    from.read(reinterpret_cast<char*>(&bytes[0]), fileSize);
    Stream() << Base::base64_encode(&bytes[0], static_cast<unsigned int>(fileSize));
    Stream() << "]]>" << endl;
}

void Writer::setForceXML(bool on)
{
    forceXML = on;
}

bool Writer::isForceXML()
{
    return forceXML;
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

std::string Writer::addFile(const char* Name,const Base::Persistence *Object)
{
    // always check isForceXML() before requesting a file!
    assert(!isForceXML());

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

void Writer::incInd()
{
    if (indent < 1020) {
        indBuf[indent  ] = ' ';
        indBuf[indent+1] = ' ';
        indBuf[indent+2] = ' ';
        indBuf[indent+3] = ' ';
        indBuf[indent+4] = '\0';
        indent += 4;
    }
}

void Writer::decInd()
{
    if (indent >= 4) {
        indent -= 4;
    }
    else {
        indent = 0;
    }
    indBuf[indent] = '\0';
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

void ZipWriter::writeFiles()
{
    // use a while loop because it is possible that while
    // processing the files new ones can be added
    size_t index = 0;
    while (index < FileList.size()) {
        FileEntry entry = FileList[index];
        ZipStream.putNextEntry(entry.FileName);
        entry.Object->SaveDocFile(*this);
        index++;
    }
}

ZipWriter::~ZipWriter()
{
    ZipStream.close();
}

// ----------------------------------------------------------------------------

FileWriter::FileWriter(const char* DirName) : DirName(DirName)
{
}

FileWriter::~FileWriter() = default;

void FileWriter::putNextEntry(const char* file)
{
    std::string fileName = DirName + "/" + file;
    this->FileStream.open(fileName.c_str(), std::ios::out | std::ios::binary);
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

            std::string fileName = DirName + "/" + entry.FileName;
            this->FileStream.open(fileName.c_str(), std::ios::out | std::ios::binary);
            entry.Object->SaveDocFile(*this);
            this->FileStream.close();
        }

        index++;
    }
}
