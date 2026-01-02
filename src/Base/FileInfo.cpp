// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <FCConfig.h>

#include <algorithm>
#include <codecvt>
#include <cstring>
#include <fstream>
#include <iostream>
#include <system_error>
#ifdef FC_OS_WIN32
# include <Windows.h>
#else
# include <unistd.h>
#endif

#include "FileInfo.h"
#include "Exception.h"
#include "TimeInfo.h"

using namespace Base;
namespace fs = std::filesystem;

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
        fs::path tmp = fs::temp_directory_path();
        tmp += fs::path::preferred_separator;
        tempPath = pathToString(tmp);
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

fs::path FileInfo::stringToPath(const std::string& str)
{
#ifdef FC_OS_WIN32
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    fs::path path(converter.from_bytes(str));
#else
    fs::path path(str);
#endif
    return path;
}

std::string FileInfo::pathToString(const fs::path& path)
{
#ifdef FC_OS_WIN32
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
        retval = pathToString(fs::current_path());
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
#ifdef FC_OS_WIN32
    return _stricmp(Ext, extension().c_str()) == 0;
#else
    return strcasecmp(Ext, extension().c_str()) == 0;
#endif
}

bool FileInfo::hasExtension(std::initializer_list<const char*> Exts) const
{
    return std::ranges::any_of(Exts, [this](const char* ext) { return hasExtension(ext); });
}

bool FileInfo::exists() const
{
    fs::path path(stringToPath(FileName));
    return fs::exists(path);
}

bool FileInfo::isReadable() const
{
    fs::path path = stringToPath(FileName);
    if (!fs::exists(path)) {
        return false;
    }
    fs::file_status stat = fs::status(path);
    fs::perms perms = stat.permissions();
    return (perms & fs::perms::owner_read) == fs::perms::owner_read;
}

bool directoryIsWritable(const fs::path& dir)
{
    try {
        if (!fs::exists(dir) || !fs::is_directory(dir)) {
            return false;
        }
        fs::path test_file = dir / (".fs_perm_test_" + std::to_string(std::rand()) + ".tmp");
        {
            std::ofstream ofs(test_file);
            if (!ofs) {
                return false;
            }
        }
        std::error_code ec;
        fs::remove(test_file, ec);
        return true;
    }
    catch (...) {
        return false;
    }
}

bool FileInfo::isWritable() const
{
    fs::path path = stringToPath(FileName);
    if (!fs::exists(path)) {
        return false;
    }
    if (fs::is_directory(path)) {
        return directoryIsWritable(path);
    }
#ifdef FC_OS_WIN32
    // convert filename from UTF-8 to windows WSTRING
    std::wstring fileNameWstring = toStdWString();
    // requires import of <windows.h>
    DWORD attributes = GetFileAttributes(fileNameWstring.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        // Log the error?
        std::clog << "GetFileAttributes failed for file: " << FileName << '\n';
        // usually indicates some kind of network file issue, so the file is probably not writable
        return false;
    }
    if ((attributes & FILE_ATTRIBUTE_READONLY) != 0) {
        return false;
    }
    // TEST if file is truly writable, because windows ACL does not map well to POSIX perms,
    //  and there are other potential blockers (app or shared file locks, etc)
    HANDLE hFile = CreateFileW(
        fileNameWstring.c_str(),
        GENERIC_WRITE,
        0,  // ----> no sharing: fail if anyone else has it open
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        if (err == ERROR_SHARING_VIOLATION || err == ERROR_LOCK_VIOLATION) {
            return false;
        }
        return false;
    }
    if (!CloseHandle(hFile)) {
        std::clog << "CloseHandle failed for file: " << FileName
                  << " while checking for write access." << '\n';
    }
#endif
    fs::file_status stat = fs::status(path);
    fs::perms perms = stat.permissions();
    return (perms & fs::perms::owner_write) == fs::perms::owner_write;
}

bool FileInfo::setPermissions(Permissions perms)
{
    fs::perms mode = fs::perms::none;

    if (perms & FileInfo::ReadOnly) {
        mode |= fs::perms::owner_read;
    }
    if (perms & FileInfo::WriteOnly) {
        mode |= fs::perms::owner_write;
    }

    if (mode == fs::perms::none) {  // bad argument
        return false;
    }

    fs::path file_path = stringToPath(FileName);
    if (!fs::exists(file_path)) {
        return false;
    }

    fs::permissions(file_path, mode);
    fs::file_status stat = fs::status(file_path);
    return stat.permissions() == mode;
}

bool FileInfo::isFile() const
{
    fs::path path = stringToPath(FileName);
    if (fs::exists(path)) {
        return fs::is_regular_file(path);
    }

    // TODO: Check for valid file name
    return true;
}

bool FileInfo::isDir() const
{
    fs::path path = stringToPath(FileName);
    if (fs::exists(path)) {
        return fs::is_directory(path);
    }

    return false;
}

unsigned int FileInfo::size() const
{
    unsigned int bytes {};
    fs::path path = stringToPath(FileName);
    if (fs::exists(path)) {
        bytes = fs::file_size(path);
    }

    return bytes;
}

template<typename TP>
std::time_t to_time_t(TP tp)
{
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now() + system_clock::now());
    return system_clock::to_time_t(sctp);
}

TimeInfo FileInfo::lastModified() const
{
    TimeInfo ti = TimeInfo::null();

    if (exists()) {
        fs::path path = stringToPath(FileName);
        ti.setTime_t(to_time_t(fs::last_write_time(path)));
    }

    return ti;
}

bool FileInfo::deleteFile() const
{
    try {
        fs::path path = stringToPath(FileName);
        return fs::remove(path);
    }
    catch (const fs::filesystem_error& e) {
        std::clog << e.what() << '\n';
        return false;
    }
}

bool FileInfo::renameFile(const char* NewName)
{
    try {
        fs::path old_path = stringToPath(FileName);
        fs::path new_path = stringToPath(NewName);
        fs::rename(old_path, new_path);
        FileName = NewName;
        return true;
    }
    catch (const fs::filesystem_error& e) {
        std::clog << "Error in renameFile: " << e.what() << '\n';
        return false;
    }
}

bool FileInfo::copyTo(const char* NewName) const
{
    try {
        fs::path old_path = stringToPath(FileName);
        fs::path new_path = stringToPath(NewName);
        fs::copy(old_path, new_path);
        return true;
    }
    catch (const fs::filesystem_error&) {
        return false;
    }
}

bool FileInfo::createDirectory() const
{
    try {
        fs::path path(stringToPath(FileName));
        return fs::create_directory(path);
    }
    catch (const fs::filesystem_error&) {
        return false;
    }
}

bool FileInfo::createDirectories() const
{
    try {
        fs::path path(stringToPath(FileName));
        if (fs::exists(path)) {
            return true;
        }

        return fs::create_directories(path);
    }
    catch (const fs::filesystem_error&) {
        return false;
    }
}

bool FileInfo::deleteDirectory() const
{
    if (!isDir()) {
        return false;
    }

    try {
        fs::path path = stringToPath(FileName);
        return fs::remove(path);
    }
    catch (const fs::filesystem_error& e) {
        std::clog << e.what() << '\n';
        return false;
    }
}

bool FileInfo::deleteDirectoryRecursive() const
{
    if (!isDir()) {
        return false;
    }

    try {
        fs::path path = stringToPath(FileName);
        return fs::remove_all(path) > 0;
    }
    catch (const fs::filesystem_error& e) {
        std::clog << e.what() << '\n';
        return false;
    }
}

std::vector<Base::FileInfo> FileInfo::getDirectoryContent() const
{
    std::error_code ec;
    std::vector<Base::FileInfo> list;
    fs::path path = stringToPath(FileName);

    for (const fs::directory_entry& f : fs::directory_iterator {path, ec}) {
        list.emplace_back(pathToString(f.path()));
    }

    return list;
}
