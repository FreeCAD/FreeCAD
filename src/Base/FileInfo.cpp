/***************************************************************************
 *   Copyright (c) 2005 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with FreeCAD; if not, write to the Free Software        *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
# include <algorithm>
# include <cassert>
# include <codecvt>
# include <cstring>
# include <locale>
# if defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
# include <dirent.h>
# include <unistd.h>
# endif
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "FileInfo.h"
#include "Exception.h"
#include "Stream.h"


using namespace Base;

#ifndef R_OK
#define R_OK    4   /* Test for read permission    */
#endif
#ifndef W_OK
#define W_OK    2   /* Test for write permission   */
#endif
#ifndef X_OK
#define X_OK    1   /* Test for execute permission */
#endif
#ifndef F_OK
#define F_OK    0   /* Test for existence          */
#endif



//**********************************************************************************
// FileInfo


FileInfo::FileInfo (const char* _FileName)
{
    setFile(_FileName);
}

FileInfo::FileInfo (const std::string &_FileName)
{
    setFile(_FileName.c_str());
}

const std::string &FileInfo::getTempPath()
{
    static std::string tempPath;

    if (tempPath == "") {
        const char* tmp = getenv("TMPDIR");
        if (tmp && tmp[0] != '\0') {
            tempPath = tmp;
            FileInfo fi(tempPath);
            if (tempPath.empty() || !fi.isDir()) // still empty or non-existent
                tempPath = "/tmp/";
            else if (tempPath.at(tempPath.size()-1)!='/')
                tempPath.append("/");
        }
        else {
            tempPath = "/tmp/";
        }
    }

    return tempPath;
}

std::string FileInfo::getTempFileName(const char* FileName, const char* Path)
{

    std::string buf;

    // Path where the file is located
    if (Path)
        buf = Path;
    else
        buf = getTempPath();

    // File name in the path
    if (FileName) {
        buf += "/";
        buf += FileName;
        buf += "XXXXXX";
    }
    else {
        buf += "/fileXXXXXX";
    }

    std::vector<char> vec;
    std::copy(buf.begin(), buf.end(), std::back_inserter(vec));
    vec.push_back('\0');

    /* coverity[secure_temp] mkstemp uses 0600 as the mode and is safe */
    int id = mkstemp(vec.data());
    if (id > -1) {
        FILE* file = fdopen(id, "w");
        fclose(file);
        vec.pop_back(); // remove '\0'
        std::string str(vec.begin(), vec.end());
        buf.swap(str);
        unlink(buf.c_str());
    }
    return buf;
}

boost::filesystem::path FileInfo::stringToPath(const std::string& str)
{
    boost::filesystem::path path(str);
    return path;
}

std::string FileInfo::pathToString(const boost::filesystem::path& p)
{
    return p.string();
}

void FileInfo::setFile(const char* name)
{
    if (!name) {
        FileName.clear();
        return;
    }

    FileName = name;

    // keep the UNC paths intact
    if (FileName.substr(0,2) == std::string("\\\\"))
        std::replace(FileName.begin()+2, FileName.end(), '\\', '/');
    else
        std::replace(FileName.begin(), FileName.end(), '\\', '/');
}

std::string FileInfo::filePath () const
{
    return FileName;
}

std::string FileInfo::fileName () const
{
    return FileName.substr(FileName.find_last_of('/')+1);
}

std::string FileInfo::dirPath () const
{
    std::size_t last_pos;
    std::string retval;
    last_pos = FileName.find_last_of('/');
    if (last_pos != std::string::npos) {
        retval = FileName.substr(0, last_pos);
    }
    else {
        char buf[PATH_MAX+1];
        const char* cwd = getcwd(buf, PATH_MAX);
        retval = std::string(cwd ? cwd : ".");
    }
    return retval;
}

std::string FileInfo::fileNamePure () const
{
    std::string temp = fileName();
    std::string::size_type pos = temp.find_last_of('.');

    if (pos != std::string::npos)
        return temp.substr(0,pos);
    else
        return temp;
}

std::wstring FileInfo::toStdWString() const
{
    // On other platforms it's discouraged to use wchar_t for file names
    throw Base::FileException("Cannot use FileInfo::toStdWString() on this platform");
}

std::string FileInfo::extension () const
{
    std::string::size_type pos = FileName.find_last_of('.');
    if (pos == std::string::npos)
        return std::string();
    return FileName.substr(pos+1);
}

std::string FileInfo::completeExtension () const
{
    std::string::size_type pos = FileName.find_first_of('.');
    if (pos == std::string::npos)
        return std::string();
    return FileName.substr(pos+1);
}

bool FileInfo::hasExtension (const char* Ext) const
{
#if defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    return strcasecmp(Ext,extension().c_str()) == 0;
#endif
}

bool FileInfo::exists () const
{
#if defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    return access(FileName.c_str(),F_OK) == 0;
#endif
}

bool FileInfo::isReadable () const
{
#if defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    return access(FileName.c_str(),R_OK) == 0;
#endif
}

bool FileInfo::isWritable () const
{
#if defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    return access(FileName.c_str(),W_OK) == 0;
#endif
}

bool FileInfo::setPermissions (Permissions perms)
{
    int mode = 0;

    if (perms & FileInfo::ReadOnly)
        mode |= S_IREAD;
    if (perms & FileInfo::WriteOnly)
        mode |= S_IWRITE;

    if (mode == 0) // bad argument
        return false;
#if defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    return chmod(FileName.c_str(),mode) == 0;
#endif
}

bool FileInfo::isFile () const
{
    if (exists()) {
        // If we can open it must be an existing file, otherwise we assume it
        // is a directory (which doesn't need to be true for any cases)
        std::ifstream str(FileName.c_str(), std::ios::in | std::ios::binary);
        if (!str)
            return false;
        str.close();
        return true;
    }

    // TODO: Check for valid file name
    return true;
}

bool FileInfo::isDir () const
{
    if (exists()) {
        // if we can chdir then it must be a directory, otherwise we assume it
        // is a file (which doesn't need to be true for any cases)
#if defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
        struct stat st;
        if (stat(FileName.c_str(), &st) != 0) {
            return false;
        }
        return S_ISDIR(st.st_mode);
#else
        return false;
#endif
    }
    else
        return false;

    // TODO: Check for valid path name
    //return true;
}

unsigned int FileInfo::size () const
{
    // not implemented
    assert(0);
    return 0;
}

TimeInfo FileInfo::lastModified() const
{
    TimeInfo ti = TimeInfo::null();
    if (exists()) {

#if defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
        struct stat st;
        if (stat(FileName.c_str(), &st) == 0) {
            ti.setTime_t(st.st_mtime);
        }
#endif

    }
    return ti;
}

TimeInfo FileInfo::lastRead() const
{
    TimeInfo ti = TimeInfo::null();
    if (exists()) {

#if defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
        struct stat st;
        if (stat(FileName.c_str(), &st) == 0) {
            ti.setTime_t(st.st_atime);
        }
#endif

    }
    return ti;
}

bool FileInfo::deleteFile() const
{
#if defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    return (::remove(FileName.c_str())==0);
#else
#   error "FileInfo::deleteFile() not implemented for this platform!"
#endif
}

bool FileInfo::renameFile(const char* NewName)
{
    bool res;
#if defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    res = ::rename(FileName.c_str(),NewName) == 0;
#else
#   error "FileInfo::renameFile() not implemented for this platform!"
#endif
    if (!res) {
        int code = errno;
        std::clog << "Error in renameFile: " << strerror(code) << " (" << code << ")" << std::endl;
    }
    else {
        FileName = NewName;
    }

    return res;
}

bool FileInfo::copyTo(const char* NewName) const
{
#if defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    FileInfo fi1(FileName);
    FileInfo fi2(NewName);
    Base::ifstream file(fi1, std::ios::in | std::ios::binary);
    file.unsetf(std::ios_base::skipws);
    Base::ofstream copy(fi2, std::ios::out | std::ios::binary);
    file >> copy.rdbuf();
    return file.is_open() && copy.is_open();
#else
#   error "FileInfo::copyTo() not implemented for this platform!"
#endif
}

bool FileInfo::createDirectory() const
{
#if defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    return mkdir(FileName.c_str(), 0777) == 0;
#else
#   error "FileInfo::createDirectory() not implemented for this platform!"
#endif
}

bool FileInfo::createDirectories() const
{
    try {
        boost::filesystem::path path(stringToPath(FileName));
        if (boost::filesystem::exists(path))
            return true;
        boost::filesystem::create_directories(path);
        return true;
    }
    catch (const boost::filesystem::filesystem_error&) {
        return false;
    }
}

bool FileInfo::deleteDirectory() const
{
    if (isDir() == false )
        return false;
#if defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    return rmdir(FileName.c_str()) == 0;
#else
#   error "FileInfo::rmdir() not implemented for this platform!"
#endif
}

bool FileInfo::deleteDirectoryRecursive() const
{
    if (isDir() == false )
        return false;
    std::vector<Base::FileInfo> List = getDirectoryContent();

    for (std::vector<Base::FileInfo>::iterator It = List.begin();It!=List.end();++It) {
        if (It->isDir()) {
            // At least on Linux, directory needs execute permission to be
            // deleted. We don't really need to set permission for directory
            // anyway, since FC code does not touch directory permission.
            //
            // It->setPermissions(FileInfo::ReadWrite);

            It->deleteDirectoryRecursive();
        }
        else if (It->isFile()) {
            It->setPermissions(FileInfo::ReadWrite);
            It->deleteFile();
        }
        else {
            throw Base::FileException("FileInfo::deleteDirectoryRecursive(): Unknown object Type in directory!");
        }
    }
    return deleteDirectory();
}

std::vector<Base::FileInfo> FileInfo::getDirectoryContent() const
{
    std::vector<Base::FileInfo> List;
#if defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    DIR* dp(nullptr);
    struct dirent* dentry(nullptr);
    if ((dp = opendir(FileName.c_str())) == nullptr)
    {
        return List;
    }

    while ((dentry = readdir(dp)) != nullptr)
    {
        std::string dir = dentry->d_name;
        if (dir != "." && dir != "..")
            List.emplace_back(FileName + "/" + dir);
    }
    closedir(dp);
#else
#   error "FileInfo::getDirectoryContent() not implemented for this platform!"
#endif
    return List;
}
