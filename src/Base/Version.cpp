// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Base/Version.h>
#include <string>

// If you stumble here, run the target "BuildExtractRevision" on Windows systems
// or the Python script "SubWCRev.py" on Linux based systems which builds
// src/Build/Version.h. Or create your own from src/Build/Version.h.in!
#include <Build/Version.h>

namespace Base
{

const char* FCVersionInfo::VersionMajor()
{
    return FCVersionMajor;
}
const char* FCVersionInfo::VersionMinor()
{
    return FCVersionMinor;
}
const char* FCVersionInfo::VersionName()
{
    return FCVersionName;
}
const char* FCVersionInfo::VersionPoint()
{
    return FCVersionPoint;
}
const char* FCVersionInfo::VersionSuffix()
{
    return FCVersionSuffix;
}
bool FCVersionInfo::IsDevelopmentVersion()
{
    static bool isDev = []() constexpr {
        return std::string(VersionSuffix()) == "dev";
    }();
    return isDev;
}

int FCVersionInfo::CopyrightYear()
{
    return FCCopyrightYear;
}

const char* FCVersionInfo::Revision()
{
    return FCRevision;
}
const char* FCVersionInfo::RevisionDate()
{
    return FCRevisionDate;
}
const char* FCVersionInfo::RepositoryURL()
{
    return FCRepositoryURL;
}

const char* FCVersionInfo::RepositoryHash()
{
#if defined(FCRepositoryHash)
    return FCRepositoryHash;
#else
    return "";
#endif
}
const char* FCVersionInfo::RepositoryBranch()
{
#if defined(FCRepositoryBranch)
    return FCRepositoryBranch;
#else
    return "";
#endif
}

}  // namespace Base
