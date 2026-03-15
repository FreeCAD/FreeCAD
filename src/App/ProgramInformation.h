// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2025 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#pragma once

#include <FCGlobal.h>
#include <map>
#include <string>
#include <sstream>

namespace App
{

class AppExport ProgramInformation
{
public:
    /**
     * @name Program Information
     *
     * @{
     */

    /// Get a pretty formatted product information string.
    static std::string prettyProductInfoWrapper();

    /**
     * @brief Add module info to the verbose output.
     *
     * This function is used to add information about a single add-on.
     *
     * @param[in,out] str The text stream to write the information to.
     * @param[in] modPath The path of the module.
     */
    static void addModuleInfo(std::stringstream& str, const std::string& path);

    /**
     * @brief Get verbose information about the application.
     *
     * @param[in,out] str The text stream to write the information to.
     * @param[in] mConfig The application configuration.
     */
    static void getVerboseCommonInfo(
        std::stringstream& str,
        const std::map<std::string, std::string>& mConfig);

    /**
     * @brief Get verbose information about add-ons.
     *
     * @copydetails getVerboseCommonInfo
     */
    static void getVerboseAddOnsInfo(
        std::stringstream& str,
        const std::map<std::string, std::string>& mConfig);

    /**
     * Constant that request verbose version information to be printed.
     *
     * If an exception has this message, it means that we will print verbose
     * version information.
     */
    static constexpr const char* verboseVersionEmitMessage{"verbose_version"};

private:
    static void getSystemInformation(std::stringstream& str);
    static void getVersionInformation(
        const std::map<std::string, std::string>& mConfig,
        std::stringstream& str);
    static void getBuildInformation(
        const std::map<std::string, std::string>& mConfig,
        std::stringstream& str);
    static void getPackageInformation(std::stringstream& str);
    static void getLibraryVersions(std::stringstream& str);
    static void getIfcInfo(std::stringstream& str);
    static void getLocale(std::stringstream& str);

    /**
     * @brief Get a value from a map or an empty string.
     *
     * @param[in] map The map to search.
     * @param[in] key The key to search for.
     * @return Returns the value if found, or an empty string otherwise.
     */
    static std::string getValueOrEmpty(
        const std::map<std::string, std::string>& map,
        const std::string& key);

    /// @}
};

}
