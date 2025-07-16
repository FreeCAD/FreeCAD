// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>      *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
#include <boost/algorithm/string/replace.hpp>
#include <boost/regex.hpp>
#include <string>
#endif

#include <Base/TimeInfo.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>
#include <Base/Tools.h>
#include <Base/Writer.h>

#include "BackupPolicy.h"

using namespace App;

void BackupPolicy::setPolicy(const Policy p)
{
    policy = p;
}
void BackupPolicy::setNumberOfFiles(const int count)
{
    numberOfFiles = count;
}
void BackupPolicy::useBackupExtension(const bool on)
{
    useFCBakExtension = on;
}
void BackupPolicy::setDateFormat(const std::string& fmt)
{
    saveBackupDateFormat = fmt;
}
void BackupPolicy::apply(const std::string& sourcename, const std::string& targetname)
{
    switch (policy) {
        case Standard:
            applyStandard(sourcename, targetname);
            break;
        case TimeStamp:
            applyTimeStamp(sourcename, targetname);
            break;
    }
}

void BackupPolicy::applyStandard(const std::string& sourcename, const std::string& targetname) const
{
    // if saving the project data succeeded rename to the actual file name
    if (Base::FileInfo fi(targetname); fi.exists()) {
        if (numberOfFiles > 0) {
            int nSuff = 0;
            std::string fn = fi.fileName();
            Base::FileInfo di(fi.dirPath());
            std::vector<Base::FileInfo> backup;
            std::vector<Base::FileInfo> files = di.getDirectoryContent();
            for (const Base::FileInfo& it : files) {
                if (std::string file = it.fileName(); file.substr(0, fn.length()) == fn) {
                    // starts with the same file name
                    std::string suf(file.substr(fn.length()));
                    if (!suf.empty()) {
                        std::string::size_type nPos = suf.find_first_not_of("0123456789");
                        if (nPos == std::string::npos) {
                            // store all backup files
                            backup.push_back(it);
                            nSuff =
                                std::max<int>(nSuff, static_cast<int>(std::atol(suf.c_str())));
                        }
                    }
                }
            }

            if (!backup.empty() && static_cast<int>(backup.size()) >= numberOfFiles) {
                // delete the oldest backup file we found
                Base::FileInfo del = backup.front();
                for (const Base::FileInfo& it : backup) {
                    if (it.lastModified() < del.lastModified()) {
                        del = it;
                    }
                }

                del.deleteFile();
                fn = del.filePath();
            }
            else {
                // create a new backup file
                std::stringstream str;
                str << fi.filePath() << (nSuff + 1);
                fn = str.str();
            }

            if (!fi.renameFile(fn.c_str())) {
                Base::Console().warning("Cannot rename project file to backup file\n");
            }
        }
        else {
            fi.deleteFile();
        }
    }

    if (Base::FileInfo tmp(sourcename); !tmp.renameFile(targetname.c_str())) {
        throw Base::FileException("Cannot rename tmp save file to project file",
                                  Base::FileInfo(targetname));
    }
}

void BackupPolicy::applyTimeStamp(const std::string& sourcename, const std::string& targetname)
{
    Base::FileInfo fi(targetname);

    std::string fn = sourcename;
    std::string ext = fi.extension();
    std::string bn;   // full path with no extension but with "."
    std::string pbn;  // base name of the project + "."
    if (!ext.empty()) {
        bn = fi.filePath().substr(0, fi.filePath().length() - ext.length());
        pbn = fi.fileName().substr(0, fi.fileName().length() - ext.length());
    }
    else {
        bn = fi.filePath() + ".";
        pbn = fi.fileName() + ".";
    }

    bool backupManagementError = false;  // Note error and report at the end
    if (fi.exists()) {
        if (numberOfFiles > 0) {
            // replace . by - in format to avoid . between base name and extension
            boost::replace_all(saveBackupDateFormat, ".", "-");
            {
                // Remove all extra backups
                std::string filename = fi.fileName();
                Base::FileInfo di(fi.dirPath());
                std::vector<Base::FileInfo> backup;
                std::vector<Base::FileInfo> files = di.getDirectoryContent();
                for (const Base::FileInfo& it : files) {
                    if (it.isFile()) {
                        std::string file = it.fileName();
                        std::string fext = it.extension();
                        std::string fextUp = fext;
                        std::transform(fextUp.begin(),
                                       fextUp.end(),
                                       fextUp.begin(),
                                       static_cast<int (*)(int)>(toupper));
                        // re-enforcing identification of the backup file


                        // old case : the name starts with the full name of the project and
                        // follows with numbers
                        if ((startsWith(file, filename) && (file.length() > filename.length())
                             && checkDigits(file.substr(filename.length())))
                            ||
                            // .FCBak case : The bame starts with the base name of the project +
                            // "."
                            // + complement with no "." + ".FCBak"
                            ((fextUp == "FCBAK") && startsWith(file, pbn)
                             && (checkValidComplement(file, pbn, fext)))) {
                            backup.push_back(it);
                        }
                    }
                }

                if (!backup.empty() && static_cast<int>(backup.size()) >= numberOfFiles) {
                    std::sort(backup.begin(), backup.end(), fileComparisonByDate);
                    // delete the oldest backup file we found
                    // Base::FileInfo del = backup.front();
                    int nb = 0;
                    for (Base::FileInfo& it : backup) {
                        nb++;
                        if (nb >= numberOfFiles) {
                            try {
                                if (!it.deleteFile()) {
                                    backupManagementError = true;
                                    Base::Console().warning("Cannot remove backup file : %s\n",
                                                            it.fileName().c_str());
                                }
                            }
                            catch (...) {
                                backupManagementError = true;
                                Base::Console().warning("Cannot remove backup file : %s\n",
                                                        it.fileName().c_str());
                            }
                        }
                    }
                }
            }  // end remove backup

            // create a new backup file
            {
                int ext2 = 1;
                if (useFCBakExtension) {
                    std::stringstream str;
                    Base::TimeInfo ti = fi.lastModified();
                    time_t s = ti.getTime_t();
                    struct tm* timeinfo = localtime(&s);
                    char buffer[100];

                    strftime(buffer, sizeof(buffer), saveBackupDateFormat.c_str(), timeinfo);
                    str << bn << buffer;

                    fn = str.str();
                    bool done = false;

                    if ((fn.empty()) || (fn[fn.length() - 1] == ' ')
                        || (fn[fn.length() - 1] == '-')) {
                        if (fn[fn.length() - 1] == ' ') {
                            fn = fn.substr(0, fn.length() - 1);
                        }
                    }
                    else {
                        if (!renameFileNoErase(fi, fn + ".FCBak")) {
                            fn = fn + "-";
                        }
                        else {
                            done = true;
                        }
                    }

                    if (!done) {
                        while (ext2 < numberOfFiles + 10) {
                            if (renameFileNoErase(fi, fn + std::to_string(ext2) + ".FCBak")) {
                                break;
                            }
                            ext2++;
                        }
                    }
                }
                else {
                    // changed but simpler and solves also the delay sometimes introduced by
                    // google drive
                    while (ext2 < numberOfFiles + 10) {
                        // linux just replace the file if exists, and then the existence is to
                        // be tested before rename
                        if (renameFileNoErase(fi, fi.filePath() + std::to_string(ext2))) {
                            break;
                        }
                        ext2++;
                    }
                }

                if (ext2 >= numberOfFiles + 10) {
                    Base::Console().error(
                        "File not saved: Cannot rename project file to backup file\n");
                    // throw Base::FileException("File not saved: Cannot rename project file to
                    // backup file", fi);
                }
            }
        }
        else {
            try {
                fi.deleteFile();
            }
            catch (...) {
                Base::Console().warning("Cannot remove backup file: %s\n",
                                        fi.fileName().c_str());
                backupManagementError = true;
            }
        }
    }

    Base::FileInfo tmp(sourcename);
    if (!tmp.renameFile(targetname.c_str())) {
        throw Base::FileException(
            "Save interrupted: Cannot rename temporary file to project file",
            tmp);
    }

    if (backupManagementError) {
        throw Base::FileException(
            "Warning: Save complete, but error while managing backup history.",
            fi);
    }
}

bool BackupPolicy::fileComparisonByDate(const Base::FileInfo& i, const Base::FileInfo& j)
{
    return (i.lastModified() > j.lastModified());
}

bool BackupPolicy::startsWith(const std::string& st1, const std::string& st2) const
{
    return st1.substr(0, st2.length()) == st2;
}

bool BackupPolicy::checkValidString(const std::string& cmpl, const boost::regex& e) const
{
    boost::smatch what;
    const bool res = boost::regex_search(cmpl, what, e);
    return res;
}

bool BackupPolicy::checkValidComplement(const std::string& file,
                          const std::string& pbn,
                          const std::string& ext) const
{
    const std::string cmpl =
        file.substr(pbn.length(), file.length() - pbn.length() - ext.length() - 1);
    const boost::regex e(R"(^[^.]*$)");
    return checkValidString(cmpl, e);
}

bool BackupPolicy::checkDigits(const std::string& cmpl) const
{
    const boost::regex e(R"(^[0-9]*$)");
    return checkValidString(cmpl, e);
}

bool BackupPolicy::renameFileNoErase(Base::FileInfo fi, const std::string& newName)
{
    // linux just replaces the file if it exists, so the existence is to be tested before rename
    const Base::FileInfo nf(newName);
    if (!nf.exists()) {
        return fi.renameFile(newName.c_str());
    }
    return false;
}
