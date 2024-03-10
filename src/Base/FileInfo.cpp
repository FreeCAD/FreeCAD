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
#include <algorithm>
#include <cassert>
#include <codecvt>
#include <cstring>
#include <locale>
#if defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
#include <dirent.h>
#include <unistd.h>
#elif defined(FC_OS_WIN32)
#include <io.h>
#include <Windows.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "FileInfo.h"
#include "Exception.h"
#include "Stream.h"


using namespace Base;

#ifndef R_OK
#define R_OK 4 /* Test for read permission    */
#endif
#ifndef W_OK
#define W_OK 2 /* Test for write permission   */
#endif
#ifndef X_OK
#define X_OK 1 /* Test for execute permission */
#endif
#ifndef F_OK
#define F_OK 0 /* Test for existence          */
#endif

//**********************************************************************************
// helper

#ifdef FC_OS_WIN32
std::string ConvertFromWideString(const std::wstring& string)
{
    int neededSize = WideCharToMultiByte(CP_UTF8, 0, string.c_str(), -1, 0, 0, 0, 0);
    char* CharString = new char[static_cast<size_t>(neededSize)];
    WideCharToMultiByte(CP_UTF8, 0, string.c_str(), -1, CharString, neededSize, 0, 0);
    std::string String(CharString);
    delete[] CharString;
    CharString = NULL;
    return String;
}

std::wstring ConvertToWideString(const std::string& string)
{
    int neededSize = MultiByteToWideChar(CP_UTF8, 0, string.c_str(), -1, 0, 0);
    wchar_t* wideCharString = new wchar_t[static_cast<size_t>(neededSize)];
    MultiByteToWideChar(CP_UTF8, 0, string.c_str(), -1, wideCharString, neededSize);
    std::wstring wideString(wideCharString);
    delete[] wideCharString;
    wideCharString = NULL;
    return wideString;
}
#endif


//**********************************************************************************
// FileInfo


FileInfo::FileInfo(const char* fileName)
{
    setFile(fileName);
}

FileInfo::FileInfo(const std::string& fileName)
{
    setFile(fileName.c_str());
}

const std::string& FileInfo::getTempPath()
{
    static std::string tempPath;

    if (tempPath.empty()) {
#ifdef FC_OS_WIN32
        wchar_t buf[MAX_PATH + 2];
        GetTempPathW(MAX_PATH + 1, buf);
        int neededSize = WideCharToMultiByte(CP_UTF8, 0, buf, -1, 0, 0, 0, 0);
        char* dest = new char[static_cast<size_t>(neededSize)];
        WideCharToMultiByte(CP_UTF8, 0, buf, -1, dest, neededSize, 0, 0);
        tempPath = dest;
        delete[] dest;
#else
        const char* tmp = getenv("TMPDIR");
        if (tmp && tmp[0] != '\0') {
            tempPath = tmp;
            FileInfo fi(tempPath);
            if (tempPath.empty() || !fi.isDir()) {  // still empty or non-existent
                tempPath = "/tmp/";
            }
            else if (tempPath.at(tempPath.size() - 1) != '/') {
                tempPath.append("/");
            }
        }
        else {
            tempPath = "/tmp/";
        }
#endif
    }

    return tempPath;
}

std::string FileInfo::getTempFileName(const char* FileName, const char* Path)
{
    // FIXME: To avoid race conditions we should rather return a file pointer
    // than a file name.
#ifdef FC_OS_WIN32
    wchar_t buf[MAX_PATH + 2];

    // Path where the file is located
    std::wstring path;
    if (Path) {
        path = ConvertToWideString(std::string(Path));
    }
    else {
        path = ConvertToWideString(getTempPath());
    }

    // File name in the path
    std::wstring file;
    if (FileName) {
        file = ConvertToWideString(std::string(FileName));
    }
    else {
        file = L"FCTempFile";
    }


    // this already creates the file
    GetTempFileNameW(path.c_str(), file.c_str(), 0, buf);
    DeleteFileW(buf);

    return std::string(ConvertFromWideString(std::wstring(buf)));
#else
    std::string buf;

    // Path where the file is located
    if (Path) {
        buf = Path;
    }
    else {
        buf = getTempPath();
    }

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
        vec.pop_back();  // remove '\0'
        std::string str(vec.begin(), vec.end());
        buf.swap(str);
        unlink(buf.c_str());
    }
    return buf;
#endif
}

boost::filesystem::path FileInfo::stringToPath(const std::string& str)
{
#if defined(FC_OS_WIN32)
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    boost::filesystem::path path(converter.from_bytes(str));
#else
    boost::filesystem::path path(str);
#endif
    return path;
}

std::string FileInfo::pathToString(const boost::filesystem::path& path)
{
#if defined(FC_OS_WIN32)
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(path.wstring());
#else
    return path.string();
#endif
}

void FileInfo::setFile(const char* name)
{
    if (!name) {
        FileName.clear();
        return;
    }

    FileName = name;

    // keep the UNC paths intact
    if (FileName.substr(0, 2) == std::string("\\\\")) {
        std::replace(FileName.begin() + 2, FileName.end(), '\\', '/');
    }
    else {
        std::replace(FileName.begin(), FileName.end(), '\\', '/');
    }
}

std::string FileInfo::filePath() const
{
    return FileName;
}

std::string FileInfo::fileName() const
{
    return FileName.substr(FileName.find_last_of('/') + 1);
}

std::string FileInfo::dirPath() const
{
    std::size_t last_pos {};
    std::string retval;
    last_pos = FileName.find_last_of('/');
    if (last_pos != std::string::npos) {
        retval = FileName.substr(0, last_pos);
    }
    else {
#ifdef FC_OS_WIN32
        wchar_t buf[MAX_PATH + 1];
        GetCurrentDirectoryW(MAX_PATH, buf);
        retval = std::string(ConvertFromWideString(std::wstring(buf)));
#else
        char buf[PATH_MAX + 1];
        const char* cwd = getcwd(buf, PATH_MAX);
        retval = std::string(cwd ? cwd : ".");
#endif
    }
    return retval;
}

std::string FileInfo::fileNamePure() const
{
    std::string temp = fileName();
    std::string::size_type pos = temp.find_last_of('.');

    if (pos != std::string::npos) {
        return temp.substr(0, pos);
    }

    return temp;
}

std::wstring FileInfo::toStdWString() const
{
    // As FileName is UTF-8 is encoded we have to convert it
    // for Windows because the path names are UTF-16 encoded.
#ifdef FC_OS_WIN32
    return ConvertToWideString(FileName);
#else
    // On other platforms it's discouraged to use wchar_t for file names
    throw Base::FileException("Cannot use FileInfo::toStdWString() on this platform");
#endif
}

std::string FileInfo::extension() const
{
    std::string::size_type pos = FileName.find_last_of('.');
    if (pos == std::string::npos) {
        return {};
    }
    return FileName.substr(pos + 1);
}

std::string FileInfo::completeExtension() const
{
    std::string::size_type pos = FileName.find_first_of('.');
    if (pos == std::string::npos) {
        return {};
    }
    return FileName.substr(pos + 1);
}

bool FileInfo::hasExtension(const char* Ext) const
{
#if defined(FC_OS_WIN32)
    return _stricmp(Ext, extension().c_str()) == 0;
#elif defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    return strcasecmp(Ext, extension().c_str()) == 0;
#endif
}

bool FileInfo::hasExtension(std::initializer_list<const char*> Exts) const
{
    return std::any_of(Exts.begin(), Exts.end(), [this](const char* ext) {
        return hasExtension(ext);
    });
}

bool FileInfo::exists() const
{
#if defined(FC_OS_WIN32)
    std::wstring wstr = toStdWString();
    return _waccess(wstr.c_str(), F_OK) == 0;
#elif defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    return access(FileName.c_str(), F_OK) == 0;
#endif
}

bool FileInfo::isReadable() const
{
#if defined(FC_OS_WIN32)
    std::wstring wstr = toStdWString();
    return _waccess(wstr.c_str(), R_OK) == 0;
#elif defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    return access(FileName.c_str(), R_OK) == 0;
#endif
}

bool FileInfo::isWritable() const
{
#if defined(FC_OS_WIN32)
    std::wstring wstr = toStdWString();
    return _waccess(wstr.c_str(), W_OK) == 0;
#elif defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    return access(FileName.c_str(), W_OK) == 0;
#endif
}

bool FileInfo::setPermissions(Permissions perms)
{
    int mode = 0;

    if (perms & FileInfo::ReadOnly) {
        mode |= S_IREAD;
    }
    if (perms & FileInfo::WriteOnly) {
        mode |= S_IWRITE;
    }

    if (mode == 0) {  // bad argument
        return false;
    }
#if defined(FC_OS_WIN32)
    std::wstring wstr = toStdWString();
    return _wchmod(wstr.c_str(), mode) == 0;
#elif defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    return chmod(FileName.c_str(), mode) == 0;
#endif
}

bool FileInfo::isFile() const
{
    if (exists()) {
#ifdef FC_OS_WIN32

        std::wstring wstr = toStdWString();
        FILE* fd = _wfopen(wstr.c_str(), L"rb");
        bool ok = (fd != 0);
        if (fd) {
            fclose(fd);
        }
        return ok;
#else
        // clang-format off
        struct stat st {};
        // clang-format on
        if (stat(FileName.c_str(), &st) != 0) {
            return false;
        }
        return S_ISREG(st.st_mode);
#endif
    }

    // TODO: Check for valid file name
    return true;
}

bool FileInfo::isDir() const
{
    if (exists()) {
        // if we can chdir then it must be a directory, otherwise we assume it
        // is a file (which doesn't need to be true for any cases)
#if defined(FC_OS_WIN32)
        std::wstring wstr = toStdWString();
        struct _stat st;

        if (_wstat(wstr.c_str(), &st) != 0) {
            return false;
        }
        return ((st.st_mode & _S_IFDIR) != 0);

#elif defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
        struct stat st
        {
        };
        if (stat(FileName.c_str(), &st) != 0) {
            return false;
        }
        return S_ISDIR(st.st_mode);
#else
        return false;
#endif
    }
    else {
        return false;
    }

    // TODO: Check for valid path name
    // return true;
}

unsigned int FileInfo::size() const
{
    unsigned int bytes {};
    if (exists()) {

#if defined(FC_OS_WIN32)
        std::wstring wstr = toStdWString();
        struct _stat st;
        if (_wstat(wstr.c_str(), &st) == 0) {
            bytes = st.st_size;
        }

#elif defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
        struct stat st
        {
        };
        if (stat(FileName.c_str(), &st) == 0) {
            bytes = st.st_size;
        }
#endif
    }
    return bytes;
}

TimeInfo FileInfo::lastModified() const
{
    TimeInfo ti = TimeInfo::null();
    if (exists()) {

#if defined(FC_OS_WIN32)
        std::wstring wstr = toStdWString();
        struct _stat st;
        if (_wstat(wstr.c_str(), &st) == 0) {
            ti.setTime_t(st.st_mtime);
        }

#elif defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
        struct stat st
        {
        };
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

#if defined(FC_OS_WIN32)
        std::wstring wstr = toStdWString();
        struct _stat st;
        if (_wstat(wstr.c_str(), &st) == 0) {
            ti.setTime_t(st.st_atime);
        }

#elif defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
        struct stat st
        {
        };
        if (stat(FileName.c_str(), &st) == 0) {
            ti.setTime_t(st.st_atime);
        }
#endif
    }
    return ti;
}

bool FileInfo::deleteFile() const
{
#if defined(FC_OS_WIN32)
    std::wstring wstr = toStdWString();
    return ::_wremove(wstr.c_str()) == 0;
#elif defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    return (::remove(FileName.c_str()) == 0);
#else
#error "FileInfo::deleteFile() not implemented for this platform!"
#endif
}

bool FileInfo::renameFile(const char* NewName)
{
    bool res {};
#if defined(FC_OS_WIN32)
    std::wstring oldname = toStdWString();
    std::wstring newname = ConvertToWideString(NewName);
    res = ::_wrename(oldname.c_str(), newname.c_str()) == 0;
#elif defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    res = ::rename(FileName.c_str(), NewName) == 0;
#else
#error "FileInfo::renameFile() not implemented for this platform!"
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
#if defined(FC_OS_WIN32)
    std::wstring oldname = toStdWString();
    std::wstring newname = ConvertToWideString(NewName);
    return CopyFileW(oldname.c_str(), newname.c_str(), true) != 0;
#elif defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    FileInfo fi1(FileName);
    FileInfo fi2(NewName);
    Base::ifstream file(fi1, std::ios::in | std::ios::binary);
    file.unsetf(std::ios_base::skipws);
    Base::ofstream copy(fi2, std::ios::out | std::ios::binary);
    file >> copy.rdbuf();
    return file.is_open() && copy.is_open();
#else
#error "FileInfo::copyTo() not implemented for this platform!"
#endif
}

bool FileInfo::createDirectory() const
{
#if defined(FC_OS_WIN32)
    std::wstring wstr = toStdWString();
    return _wmkdir(wstr.c_str()) == 0;
#elif defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    return mkdir(FileName.c_str(), 0777) == 0;
#else
#error "FileInfo::createDirectory() not implemented for this platform!"
#endif
}

bool FileInfo::createDirectories() const
{
    try {
        boost::filesystem::path path(stringToPath(FileName));
        if (boost::filesystem::exists(path)) {
            return true;
        }
        boost::filesystem::create_directories(path);
        return true;
    }
    catch (const boost::filesystem::filesystem_error&) {
        return false;
    }
}

bool FileInfo::deleteDirectory() const
{
    if (!isDir()) {
        return false;
    }
#if defined(FC_OS_WIN32)
    std::wstring wstr = toStdWString();
    return _wrmdir(wstr.c_str()) == 0;
#elif defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    return rmdir(FileName.c_str()) == 0;
#else
#error "FileInfo::rmdir() not implemented for this platform!"
#endif
}

bool FileInfo::deleteDirectoryRecursive() const
{
    if (!isDir()) {
        return false;
    }
    std::vector<Base::FileInfo> List = getDirectoryContent();

    for (Base::FileInfo& fi : List) {
        if (fi.isDir()) {
            // At least on Linux, directory needs execute permission to be
            // deleted. We don't really need to set permission for directory
            // anyway, since FC code does not touch directory permission.
            //
            // It->setPermissions(FileInfo::ReadWrite);

            fi.deleteDirectoryRecursive();
        }
        else if (fi.isFile()) {
            fi.setPermissions(FileInfo::ReadWrite);
            fi.deleteFile();
        }
        else {
            throw Base::FileException(
                "FileInfo::deleteDirectoryRecursive(): Unknown object Type in directory!");
        }
    }
    return deleteDirectory();
}

std::vector<Base::FileInfo> FileInfo::getDirectoryContent() const
{
    std::vector<Base::FileInfo> List;
#if defined(FC_OS_WIN32)
    struct _wfinddata_t dentry;

    intptr_t hFile;

    // Find first directory entry
    std::wstring wstr = toStdWString();
    wstr += L"/*";

    if ((hFile = _wfindfirst(wstr.c_str(), &dentry)) == -1L) {
        return List;
    }

    while (_wfindnext(hFile, &dentry) == 0) {
        if (wcscmp(dentry.name, L"..") != 0) {
            List.push_back(
                FileInfo(FileName + "/" + ConvertFromWideString(std::wstring(dentry.name))));
        }
    }

    _findclose(hFile);

#elif defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    DIR* dp(nullptr);
    struct dirent* dentry(nullptr);
    if (!(dp = opendir(FileName.c_str()))) {
        return List;
    }

    while ((dentry = readdir(dp))) {
        std::string dir = dentry->d_name;
        if (dir != "." && dir != "..") {
            List.emplace_back(FileName + "/" + dir);
        }
    }
    closedir(dp);
#else
#error "FileInfo::getDirectoryContent() not implemented for this platform!"
#endif
    return List;
}
