/***************************************************************************
 *   (c) Jürgen Riegel (juergen.riegel@web.de) 2005                        *
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
 *   Juergen Riegel 2002                                                   *
 ***************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
# include <algorithm>
# include <cassert>
# include <cstdio>
# include <cstdlib>
# include <fstream>
# include <climits>
# include <cstring>
# if defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
# include <dirent.h>
# include <unistd.h>
# include <sys/stat.h>
# elif defined (FC_OS_WIN32)
# include <direct.h>
# include <io.h>
# include <windows.h>
# endif
#endif


#include "FileInfo.h"
#include "Exception.h"
#include "Stream.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdio>
#include <cerrno>
#include <cstring>

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
// helper

#ifdef FC_OS_WIN32
std::string ConvertFromWideString(const std::wstring& string)
{
    int neededSize = WideCharToMultiByte(CP_UTF8, 0, string.c_str(), -1, 0, 0,0,0);
    char * CharString = new char[neededSize];
    WideCharToMultiByte(CP_UTF8, 0, string.c_str(), -1, CharString, neededSize,0,0);
    std::string String((char*)CharString);
    delete [] CharString;
    CharString = NULL;
    return String;
}

std::wstring ConvertToWideString(const std::string& string)
{
    int neededSize = MultiByteToWideChar(CP_UTF8, 0, string.c_str(), -1, 0, 0);
    wchar_t* wideCharString = new wchar_t[neededSize];
    MultiByteToWideChar(CP_UTF8, 0, string.c_str(), -1, wideCharString, neededSize);
    std::wstring wideString(wideCharString);
    delete [] wideCharString;
    wideCharString = NULL;
    return wideString;
}
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

const std::string &FileInfo::getTempPath(void)
{
    static std::string tempPath;

    if (tempPath == "") {
#ifdef FC_OS_WIN32
        wchar_t buf[MAX_PATH + 2];
        GetTempPathW(MAX_PATH + 1,buf);
        int neededSize = WideCharToMultiByte(CP_UTF8, 0, buf, -1, 0, 0, 0, 0);
        char* dest = new char[neededSize];
        WideCharToMultiByte(CP_UTF8, 0, buf, -1,dest, neededSize, 0, 0);
        tempPath = dest;
        delete [] dest;
#else
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
#endif
    }

    return tempPath;
}

std::string FileInfo::getTempFileName(const char* FileName, const char* Path)
{
    //FIXME: To avoid race conditions we should rather return a file pointer
    //than a file name.
#ifdef FC_OS_WIN32
    wchar_t buf[MAX_PATH + 2];

    // Path where the file is located
    std::wstring path; 
    if (Path)
        path = ConvertToWideString(std::string(Path));
    else
        path = ConvertToWideString(getTempPath());

    // File name in the path 
    std::wstring file; 
    if (FileName)
        file = ConvertToWideString(std::string(FileName));
    else
        file = L"FCTempFile";


    // this already creates the file
    GetTempFileNameW(path.c_str(),file.c_str(),0,buf);
    DeleteFileW(buf);

    return std::string(ConvertFromWideString(std::wstring(buf)));
#else
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

    int id = mkstemp(const_cast<char*>(buf.c_str()));
    if (id > -1) {
        FILE* file = fdopen(id, "w");
        fclose(file);
        unlink(buf.c_str());
    }
    return buf;
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
#ifdef FC_OS_WIN32
        wchar_t buf[MAX_PATH+1];
        GetCurrentDirectoryW(MAX_PATH, buf);
        retval = std::string(ConvertFromWideString(std::wstring(buf)));
#else
        char buf[PATH_MAX+1];
        const char* cwd = getcwd(buf, PATH_MAX);
        retval = std::string(cwd ? cwd : ".");
#endif
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
    // As FileName is UTF-8 is encoded we have to convert it
    // for Windows because the path names are UCS-2 encoded.
#ifdef FC_OS_WIN32
    return ConvertToWideString(FileName);
#else
    // FIXME: For MacOS the path names are UCS-4 encoded.
    // For the moment we cannot handle path names containing
    // non-ASCII characters.
    // For Linux the paths names are encoded in UTF-8 so we actually
    // don't need this method therefore.
    return std::wstring();
#endif
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
#if defined (FC_OS_WIN32)
    return _stricmp(Ext,extension().c_str()) == 0;
#elif defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    return strcasecmp(Ext,extension().c_str()) == 0;
#endif
}

bool FileInfo::exists () const
{
#if defined (FC_OS_WIN32)
    std::wstring wstr = toStdWString();
    return _waccess(wstr.c_str(),F_OK) == 0;
#elif defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    return access(FileName.c_str(),F_OK) == 0;
#endif
}

bool FileInfo::isReadable () const
{
#if defined (FC_OS_WIN32)
    std::wstring wstr = toStdWString();
    return _waccess(wstr.c_str(),R_OK) == 0;
#elif defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    return access(FileName.c_str(),R_OK) == 0;
#endif
}

bool FileInfo::isWritable () const
{
#if defined (FC_OS_WIN32)
    std::wstring wstr = toStdWString();
    return _waccess(wstr.c_str(),W_OK) == 0;
#elif defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
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
#if defined (FC_OS_WIN32)
    std::wstring wstr = toStdWString();
    return _wchmod(wstr.c_str(),mode) == 0;
#elif defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    return chmod(FileName.c_str(),mode) == 0;
#endif
}

bool FileInfo::isFile () const
{
#ifdef FC_OS_WIN32
    if (exists()) {
        std::wstring wstr = toStdWString();
        FILE* fd = _wfopen(wstr.c_str(), L"rb");
        bool ok = (fd != 0);
        if (fd) fclose(fd);
        return ok;
    }
#else
    if (exists()) {
        // If we can open it must be an existing file, otherwise we assume it
        // is a directory (which doesn't need to be true for any cases)
        std::ifstream str(FileName.c_str(), std::ios::in | std::ios::binary);
        if (!str) return false;
        str.close();
        return true;
    }
#endif

    // TODO: Check for valid file name
    return true;
}

bool FileInfo::isDir () const
{
    if (exists()) {
        // if we can chdir then it must be a directory, otherwise we assume it
        // is a file (which doesn't need to be true for any cases)
#if defined (FC_OS_WIN32)
        std::wstring wstr = toStdWString();
        struct _stat st;

        if (_wstat(wstr.c_str(), &st) != 0)
            return false;
        return ((st.st_mode & _S_IFDIR) != 0);

#elif defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
        struct stat st;
        if (stat(FileName.c_str(), &st) != 0) {
            return false;
        }
        return S_ISDIR(st.st_mode);
#endif
        return false;
    }
    else
        return false;

    // TODO: Check for valid path name
    return true;
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

#if defined (FC_OS_WIN32)
        std::wstring wstr = toStdWString();
        struct _stat st;
        if (_wstat(wstr.c_str(), &st) == 0) {
            ti.setTime_t(st.st_mtime);
        }

#elif defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
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

#if defined (FC_OS_WIN32)
        std::wstring wstr = toStdWString();
        struct _stat st;
        if (_wstat(wstr.c_str(), &st) == 0) {
            ti.setTime_t(st.st_atime);
        }

#elif defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
        struct stat st;
        if (stat(FileName.c_str(), &st) == 0) {
            ti.setTime_t(st.st_atime);
        }
#endif

    }
    return ti;
}

bool FileInfo::deleteFile(void) const
{
#if defined (FC_OS_WIN32)
    std::wstring wstr = toStdWString();
    return ::_wremove(wstr.c_str()) == 0;
#elif defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    return (::remove(FileName.c_str())==0);
#else
#   error "FileInfo::deleteFile() not implemented for this platform!"
#endif
}

bool FileInfo::renameFile(const char* NewName)
{
    bool res;
#if defined (FC_OS_WIN32)
    std::wstring oldname = toStdWString();
    std::wstring newname = ConvertToWideString(NewName);
    res = ::_wrename(oldname.c_str(),newname.c_str()) == 0;
#elif defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    res = ::rename(FileName.c_str(),NewName) == 0;
#else
#   error "FileInfo::renameFile() not implemented for this platform!"
#endif
    if (!res) {
        int code = errno;
        std::clog << "Error in renameFile: " << strerror(code) << " (" << code << ")" << std::endl;
    }

    return res;
}

bool FileInfo::copyTo(const char* NewName) const
{
#if defined (FC_OS_WIN32)
    std::wstring oldname = toStdWString();
    std::wstring newname = ConvertToWideString(NewName);
    return CopyFileW(oldname.c_str(),newname.c_str(),true) != 0;
#elif defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    FileInfo fi1(FileName);
    FileInfo fi2(NewName);
    Base::ifstream file(fi1, std::ios::in | std::ios::binary);
    Base::ofstream copy(fi2, std::ios::out | std::ios::binary);
    file >> copy.rdbuf();
    return file.is_open() && copy.is_open();
#else
#   error "FileInfo::copyTo() not implemented for this platform!"
#endif
}

bool FileInfo::createDirectory(void) const
{
#if defined (FC_OS_WIN32)
    std::wstring wstr = toStdWString();
    return _wmkdir(wstr.c_str()) == 0;
#elif defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    return mkdir(FileName.c_str(), 0777) == 0;
#else
#   error "FileInfo::createDirectory() not implemented for this platform!"
#endif
}

bool FileInfo::deleteDirectory(void) const
{
    if (isDir() == false ) return false;
#if defined (FC_OS_WIN32)
    std::wstring wstr = toStdWString();
    return _wrmdir(wstr.c_str()) == 0;
#elif defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    return rmdir(FileName.c_str()) == 0;
#else
#   error "FileInfo::rmdir() not implemented for this platform!"
#endif
}

bool FileInfo::deleteDirectoryRecursive(void) const
{
    if (isDir() == false ) return false;
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

std::vector<Base::FileInfo> FileInfo::getDirectoryContent(void) const
{
    std::vector<Base::FileInfo> List;
#if defined (FC_OS_WIN32)
    struct _wfinddata_t dentry;

    intptr_t hFile;

    // Find first directory entry
    std::wstring wstr = toStdWString();
    wstr += L"/*";

    if ((hFile = _wfindfirst( wstr.c_str(), &dentry)) == -1L)
        return List;

    while (_wfindnext(hFile, &dentry) == 0)
        if (wcscmp(dentry.name,L"..") != 0)
            List.push_back(FileInfo(FileName + "/" +ConvertFromWideString(std::wstring(dentry.name))));

    _findclose(hFile);

#elif defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    DIR* dp(0);
    struct dirent* dentry(0);
    if ((dp = opendir(FileName.c_str())) == NULL)
    {
        return List;
    }

    while ((dentry = readdir(dp)) != NULL)
    {
        std::string dir = dentry->d_name;
        if (dir != "." && dir != "..")
            List.push_back(FileInfo(FileName + "/" + dir));
    }
    closedir(dp);
#else
#   error "FileInfo::getDirectoryContent() not implemented for this platform!"
#endif
    return List;
}
