// SPDX-License-Identifier: LGPL-2.1-or-later

#include "Version.h"
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <boost/algorithm/string/trim.hpp>

#define BOOST_DLL_USE_STD_FS
#include <boost/dll/runtime_symbol_info.hpp>

namespace
{

const char* versionValue(const char* key, const char* fallback = "")
{
    // Keep strings alive for callers and avoid repeated I/O during document operations.
    static const auto values = [] {
        std::map<std::string, std::string> result;
        const auto library = std::filesystem::canonical(boost::dll::this_line_location());
        std::ifstream file(library.parent_path() / "FreeCADVersion.txt");
        std::string line;
        while (std::getline(file, line)) {
            boost::algorithm::trim_right(line);

            const auto separator = line.find('=');
            if (separator == std::string::npos) {
                continue;
            }
            result.insert_or_assign(line.substr(0, separator), line.substr(separator + 1));
        }
        return result;
    }();

    const auto value = values.find(key);
    return value == values.end() ? fallback : value->second.c_str();
}

}  // namespace

namespace Base
{

const char* FCVersionInfo::VersionMajor()
{
    return versionValue("FCVersionMajor", "0");
}
const char* FCVersionInfo::VersionMinor()
{
    return versionValue("FCVersionMinor", "0");
}
const char* FCVersionInfo::VersionName()
{
    return versionValue("FCVersionName");
}
const char* FCVersionInfo::VersionPoint()
{
    return versionValue("FCVersionPoint", "0");
}
const char* FCVersionInfo::VersionSuffix()
{
    return versionValue("FCVersionSuffix");
}
bool FCVersionInfo::IsDevelopmentVersion()
{
    return std::strcmp(VersionSuffix(), "dev") == 0;
}

int FCVersionInfo::CopyrightYear()
{
    return std::atoi(versionValue("FCCopyrightYear"));
}

const char* FCVersionInfo::Revision()
{
    return versionValue("FCRevision", "Unknown");
}
const char* FCVersionInfo::RevisionDate()
{
    return versionValue("FCRevisionDate", "Unknown");
}
const char* FCVersionInfo::RepositoryURL()
{
    return versionValue("FCRepositoryURL", "Unknown");
}

const std::string_view FCVersionInfo::RepositoryHash()
{
    return versionValue("FCRepositoryHash");
}
const std::string_view FCVersionInfo::RepositoryBranch()
{
    return versionValue("FCRepositoryBranch");
}
const std::string_view FCVersionInfo::BuildType()
{
#ifdef FC_BUILD_TYPE
    return FC_BUILD_TYPE;
#else
    return "";
#endif
}

}  // namespace Base
