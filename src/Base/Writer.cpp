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
#ifndef _PreComp_
#include <memory>
#include <set>
#include <vector>
#include <string>
#endif

#include <limits>
#include <locale>
#include <iomanip>

#include "Writer.h"
#include "Base64.h"
#include "Base64Filter.h"
#include "Exception.h"
#include "FileInfo.h"
#include "Persistence.h"
#include "Stream.h"
#include "Tools.h"

#include <boost/iostreams/filtering_stream.hpp>
#include <zipios++/zipinputstream.h>

using namespace Base;

// boost iostream filter to escape ']]>' in text file saved into CDATA section.
// It does not check if the character is valid utf8 or not.
struct cdata_filter
{

    using char_type = char;
    using category = boost::iostreams::output_filter_tag;

    template<typename Device>
    inline bool put(Device& dev, char ch)
    {
        switch (state) {
            case 0:
            case 1:
                if (ch == ']') {
                    ++state;
                }
                else {
                    state = 0;
                }
                break;
            case 2:
                if (ch == '>') {
                    static const char escape[] = "]]><![CDATA[";
                    boost::iostreams::write(dev, escape, sizeof(escape) - 1);
                }
                state = 0;
                break;
        }
        return boost::iostreams::put(dev, ch);
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

std::ostream& Writer::beginCharStream(CharStreamFormat format)
{
    if (CharStream) {
        throw Base::RuntimeError("Writer::beginCharStream(): invalid state");
    }
    charStreamFormat = format;
    if (format == CharStreamFormat::Base64Encoded) {
        CharStream = create_base64_encoder(Stream(), Base::base64DefaultBufferSize);
    }
    else {
        Stream() << "<![CDATA[";
        CharStream = std::make_unique<boost::iostreams::filtering_ostream>();
        auto* filteredStream = dynamic_cast<boost::iostreams::filtering_ostream*>(CharStream.get());
        filteredStream->push(cdata_filter());
        filteredStream->push(Stream());
        *filteredStream << std::setprecision(std::numeric_limits<double>::digits10 + 1);
    }

    checkErrNo();

    return *CharStream;
}

std::ostream& Writer::endCharStream()
{
    if (CharStream) {
        CharStream.reset();
        if (charStreamFormat == CharStreamFormat::Raw) {
            Stream() << "]]>";
        }
    }

    checkErrNo();

    return Stream();
}

std::ostream& Writer::charStream()
{
    if (!CharStream) {
        throw Base::RuntimeError("Writer::endCharStream(): no current character stream");
    }
    return *CharStream;
}

void Writer::insertText(const std::string& str)
{
    beginCharStream() << str;
    endCharStream();
}

void Writer::insertAsciiFile(const char* FileName)
{
    Base::FileInfo fi(FileName);
    Base::ifstream from(fi);
    if (!from) {
        throw Base::FileException("Writer::insertAsciiFile() Could not open file!");
    }

    Stream() << "<![CDATA[";
    char ch {};
    while (from.get(ch)) {
        Stream().put(ch);
    }
    Stream() << "]]>" << std::endl;

    checkErrNo();
}

void Writer::insertBinFile(const char* FileName)
{
    Base::FileInfo fi(FileName);
    Base::ifstream from(fi, std::ios::in | std::ios::binary | std::ios::ate);
    if (!from) {
        throw Base::FileException("Writer::insertAsciiFile() Could not open file!");
    }

    Stream() << "<![CDATA[";
    std::ifstream::pos_type fileSize = from.tellg();
    from.seekg(0, std::ios::beg);
    std::vector<unsigned char> bytes(static_cast<size_t>(fileSize));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    from.read(reinterpret_cast<char*>(bytes.data()), fileSize);
    Stream() << Base::base64_encode(bytes.data(), static_cast<unsigned int>(fileSize));
    Stream() << "]]>" << std::endl;

    checkErrNo();
}

void Writer::setForceXML(bool on)
{
    forceXML = on;
}

bool Writer::isForceXML() const
{
    return forceXML;
}

void Writer::setFileVersion(int version)
{
    fileVersion = version;
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
    if (it != Modes.end()) {
        Modes.erase(it);
    }
}

void Writer::clearModes()
{
    Modes.clear();
}

void Writer::addError(const std::string& msg)
{
    Errors.push_back(msg);
}

void Writer::checkErrNo()
{
    switch (errno) {
        case ENOSPC:  // No space left
        case EROFS:   // Read only
        case ENODEV:  // No such device
        case EACCES:  // Access denied
            addError(strerror(errno));
    }
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

std::string Writer::addFile(const char* Name, const Base::Persistence* Object)
{
    // always check isForceXML() before requesting a file!
    assert(!isForceXML());

    FileEntry temp;
    temp.FileName = Name ? Name : "";
    if (FileNameManager.containsName(temp.FileName)) {
        temp.FileName = FileNameManager.makeUniqueName(temp.FileName);
    }
    temp.Object = Object;

    FileList.push_back(temp);
    FileNameManager.addExactName(temp.FileName);

    // return the unique file name
    return temp.FileName;
}

void Writer::incInd()
{
    if (indent < 1020) {
        indBuf[indent] = ' ';
        indBuf[indent + 1] = ' ';
        indBuf[indent + 2] = ' ';
        indBuf[indent + 3] = ' ';
        indBuf[indent + 4] = '\0';
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

void Writer::putNextEntry(const char* file, const char* obj)
{
    ObjectName = obj ? obj : file;
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
    ZipStream.setf(std::ios::fixed, std::ios::floatfield);
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
    ZipStream.setf(std::ios::fixed, std::ios::floatfield);
}

void ZipWriter::putNextEntry(const char* file, const char* obj)
{
    Writer::putNextEntry(file, obj);

    ZipStream.putNextEntry(file);

    Writer::checkErrNo();
}

void ZipWriter::writeFiles()
{
    // use a while loop because it is possible that while
    // processing the files new ones can be added
    size_t index = 0;
    while (index < FileList.size()) {
        FileEntry entry = FileList[index];
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

FileWriter::FileWriter(const char* DirName)
    : DirName(DirName)
{}

FileWriter::~FileWriter() = default;

void FileWriter::putNextEntry(const char* file, const char* obj)
{
    Writer::putNextEntry(file, obj);

    std::string fileName = DirName + "/" + file;
    this->FileStream.open(fileName.c_str(), std::ios::out | std::ios::binary);

    Writer::checkErrNo();
}

bool FileWriter::shouldWrite(const std::string& /*name*/, const Base::Persistence* /*obj*/) const
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

    Writer::checkErrNo();
}
