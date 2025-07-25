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

#include "FCGlobal.h"

#include <boost/regex.hpp>
#include <string>
#include <Base/FileInfo.h>

namespace App
{
// Helper class to handle different backup policies: originally a private class in Document.cpp,
// and extracted for public access to enable direct testing since the logic involved is quite
// complex.
class AppExport BackupPolicy
{
public:
    enum Policy
    {
        Standard,
        TimeStamp
    };
    void setPolicy(const Policy p);
    void setNumberOfFiles(const int count);
    void useBackupExtension(const bool on);
    void setDateFormat(const std::string& fmt);
    void apply(const std::string& sourcename, const std::string& targetname);

private:
    void applyStandard(const std::string& sourcename, const std::string& targetname) const;
    void applyTimeStamp(const std::string& sourcename, const std::string& targetname);
    static bool fileComparisonByDate(const Base::FileInfo& i, const Base::FileInfo& j);
    bool startsWith(const std::string& st1, const std::string& st2) const;
    bool checkValidString(const std::string& cmpl, const boost::regex& e) const;
    bool checkValidComplement(const std::string& file,
                              const std::string& pbn,
                              const std::string& ext) const;
    bool checkDigits(const std::string& cmpl) const;
    bool renameFileNoErase(Base::FileInfo fi, const std::string& newName);

private:
    Policy policy {Standard};
    int numberOfFiles {1};
    bool useFCBakExtension {true};
    std::string saveBackupDateFormat {"%Y%m%d-%H%M%S"};
};
}  // namespace App