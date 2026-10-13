// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "FCGlobal.h"
#include <string_view>

namespace Base
{

class BaseExport FCVersionInfo
{
public:
    // Version Number
    static const char* VersionMajor();
    static const char* VersionMinor();
    static const char* VersionName();
    static const char* VersionPoint();
    static const char* VersionSuffix();
    static bool IsDevelopmentVersion();

    // Displayed Copyright Year (i.e. build year)
    static int CopyrightYear();

    static const char* Revision();       // Highest committed revision number
    static const char* RevisionDate();   // Date of highest committed revision
    static const char* RepositoryURL();  // Repository URL of the working copy

    // Git relevant stuff
    static const std::string_view RepositoryHash();
    static const std::string_view RepositoryBranch();
};

}  // namespace Base
