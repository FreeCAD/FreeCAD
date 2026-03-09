// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************************************
 *                                                                                                 *
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>                                      *
 *   Copyright (c) 2025 The FreeCAD project association AISBL                                      *
 *                                                                                                 *
 *   This file is part of FreeCAD.                                                                 *
 *                                                                                                 *
 *   FreeCAD is free software: you can redistribute it and/or modify it under the terms of the     *
 *   GNU Lesser General Public License as published by the Free Software Foundation, either        *
 *   version 2.1 of the License, or (at your option) any later version.                            *
 *                                                                                                 *
 *   FreeCAD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without  *
 *   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the     *
 *   GNU Lesser General Public License for more details.                                           *
 *                                                                                                 *
 *   You should have received a copy of the GNU Lesser General Public License along with FreeCAD.  *
 *   If not, see <https://www.gnu.org/licenses/>.                                                  *
 *                                                                                                 *
 **************************************************************************************************/

#pragma once

#include "FCConfig.h"
#include "FCGlobal.h"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#ifdef FC_OS_WIN32
#include <QString>
#endif

namespace App {

    /// A helper class to handle application-wide directory management on behalf of the main
    /// App::Application class. Most of this class's methods were originally in Application, and
    /// were extracted here to be more easily testable (and to better follow the single-
    /// responsibility principle, though further work is required in that area).
    class AppExport ApplicationDirectories {

    public:

        /// Constructor
        /// \param config The base configuration dictionary. Used to create the appropriate
        /// directories, and updated to reflect their locations. New code should not directly access
        /// elements of this dictionary to determine directory locations, but should instead use
        /// the public methods of this class to determine the locations needed.
        explicit ApplicationDirectories(std::map<std::string,std::string> &config);

        /// Given argv[0], figure out the home path. Used to initialize the AppHomePath element of
        /// the configuration map prior to instantiating this class.
        static std::filesystem::path findHomePath(const char* sCall);

        /// "Home" here is the parent directory of the actual executable file being run. It is
        /// exposed here primarily for historical compatibility reasons, and new code should almost
        /// certainly NOT use this path for anything. See alternatives in the
        /// `App::ApplicationDirectories` class.
        const std::filesystem::path& getHomePath() const;

        /// Get the user's home directory. This should not be used for many things, use caution when
        /// deciding to use it for anything, there are usually better places.
        const std::filesystem::path& getUserHomePath() const;

        /// Temp path is the location of all temporary files: it is not guaranteed to preserve
        /// information between runs of the program, but *is* guaranteed to exist for the duration
        /// of a single program execution (that is, files are not deleted from it *during* the run).
        const std::filesystem::path& getTempPath() const;

        /// Get a file in the temp directory. WARNING: NOT THREAD-SAFE! Currently just forwards to
        /// the FileInfo class.  TODO: Rewrite to be thread safe
        std::filesystem::path getTempFileName(const std::string & filename = "") const;

        /// The user cache path can be used to store files that the program *prefers* not be deleted
        /// between runs, but that will be recreated or otherwise handled if they do not exist due
        /// to the cache being cleared. There is no guarantee that the files will exist from
        /// run-to-run, but an effort is made to preserve them (unlike the temp directory, which
        /// should never be used to save data between runs).
        const std::filesystem::path& getUserCachePath() const;

        /// The primary directory used to store per-user application data. This is the parent
        /// directory of all installed addons, per-user configuration files, etc. Developers looking
        /// for a place to put per-user data should begin here. Common subdirectories include "Mod",
        /// "Macros", "Materials" and many others that don't begin with the letter "M". This is
        /// typically a versioned directory, though users may choose to use a single path for
        /// multiple versions of the software.
        const std::filesystem::path& getUserAppDataDir() const;

        /// Historically, a single directory was used to store user-created (or user-installed)
        /// macro files. This is the path to that directory. Note that going forward it should *not*
        /// be assumed that all installed macros are located in this directory. This is typically a
        /// versioned directory, though users may choose to use a single path for multiple versions
        /// of the software.
        const std::filesystem::path& getUserMacroDir() const;

        /// The "resource" directory should be used to store non-ephemeral resources such as icons,
        /// templates, hardware setup, etc. -- items that should be preserved from run-to-run of the
        /// program, and between versions. This is *not* a versioned directory, and multiple
        /// versions of the software may access the same data.
        const std::filesystem::path& getResourceDir() const;

        /// Nominally, this is the directory where "help" files are stored, though for historical
        /// reasons several other informational files are stored here as well. It should only be
        /// used for user-facing informational files.
        const std::filesystem::path& getHelpDir() const;

        /// The root path of user config files `user.cfg` and `system.cfg`.
        const std::filesystem::path& getUserConfigPath() const;

        /// The directory of all extension modules. Added to `sys.path` during Python
        /// initialization.
        const std::filesystem::path& getLibraryDir() const;

        /// Get the user's home directory
        static std::filesystem::path getUserHome();

        /// Returns true if the current active directory set is custom, and false if it's the
        /// standard system default.
        bool usingCustomDirectories() const;


        // Versioned-Path Management Methods:

        /// Determine if a given config path is for the current version of the program
        /// \param config The path to check
        bool usingCurrentVersionConfig(std::filesystem::path config) const;

        /// Migrate a set of versionable configuration directories from the given path to a new
        /// version. The new version's directories cannot exist yet, and the old ones *must* exist.
        /// If the old paths are themselves versioned, then the new paths will be placed at the same
        /// level in the directory structure (e.g., they will be siblings of each entry in paths).
        /// If paths are NOT versioned, the new (versioned) copies will be placed *inside* the
        /// original paths.
        void migrateAllPaths(const std::vector<std::filesystem::path> &paths) const;

        /// A utility method to generate the versioned directory name for a given version. This only
        /// returns the version string, not an entire path. As of FreeCAD 1.1, the string is of the
        /// form "vX-Y" where X is the major version and Y is the minor, but external code should
        /// not assume that is always the form, and should instead use this method, which is
        /// guaranteed stable (that is, given "1" and "1" as the major and minor, it will always
        /// return the string "v1-1", even if in later versions the format changes.
        static std::string versionStringForPath(int major, int minor);

        /// Given an arbitrary path, determine if it has the correct form to be a versioned path
        /// (e.g. is the last component a recognized version of the code). DOES NOT recognize
        /// versions *newer* than the current version, even if the directory name matches the path
        /// naming convention used by the previous version.
        bool isVersionedPath(const std::filesystem::path &startingPath) const;

        /// Given a base path that is expected to contained versioned subdirectories, locate the
        /// directory name (*not* the path, only the final component, the version string itself)
        /// corresponding to the most recent version of the software, up to and including the
        /// current version, but NOT exceeding it.
        std::string mostRecentAvailableConfigVersion(const std::filesystem::path &startingPath) const;

        /// Given a base path that is expected to contained versioned subdirectories, locate the
        /// directory corresponding to the most recent version of the software, up to and including
        /// the current version, but NOT exceeding it.
        std::filesystem::path mostRecentConfigFromBase(const std::filesystem::path &startingPath) const;

        /// A utility method to copy all files and directories from oldPath to newPath, handling the
        /// case where newPath might itself be a subdirectory of oldPath (and *not* attempting that
        /// otherwise-recursive copy).
        static void migrateConfig(const std::filesystem::path &oldPath, const std::filesystem::path &newPath);

#ifdef FC_OS_WIN32
        /// On Windows, gets the location of the user's "AppData" directory. Invalid on other OSes.
        QString getOldGenericDataLocation();
#endif
        /// Adds subdirectory information to the appData vector for use in constructing full paths to config files, etc.
        static void getSubDirectories(const std::map<std::string,std::string>& mConfig,
                                      std::vector<std::string>& appData);
        /// To a given path it adds the subdirectories where to store application-specific files.
        /// On Linux or BSD a hidden directory (i.e. starting with a dot) is added.
        static void getOldDataLocation(const std::map<std::string,std::string>& mConfig,
                                       std::vector<std::string>& appData);
        /// If the passed path name is not empty, it will be returned, otherwise the user home path of the system will
        /// be returned.
        static std::filesystem::path findUserHomePath(const std::filesystem::path& userHome);

    protected:

        /// Override all application directories with temp directories. Returns true on success and
        /// false if the temp directory creation failed.
        bool startSafeMode(std::map<std::string,std::string>& mConfig);

        /// Take a path and add a version to it, if it's possible to do so. A version can be
        /// appended only if a) the versioned subdirectory already exists, or b) pathToCheck/subdirs
        /// does NOT yet exist. This does not actually create any directories, just determines
        /// if we can append the versioned directory name to subdirs.
        void appendVersionIfPossible(const std::filesystem::path& basePath,
                                     std::vector<std::string> &subdirs) const;

        static std::filesystem::path findPath(
            const std::filesystem::path& stdHome,
            const std::filesystem::path& customHome,
            const std::vector<std::string>& paths,
            bool create);

        void configurePaths(std::map<std::string,std::string> &config);
        void configureResourceDirectory(const std::map<std::string,std::string>& mConfig);
        void configureLibraryDirectory(const std::map<std::string,std::string>& mConfig);
        void configureHelpDirectory(const std::map<std::string,std::string>& mConfig);

        /*!
         * Returns a tuple of path names where to store config, data, and temp. files.
         * The method therefore reads the environment variables:
         * \list
         * \li FREECAD_USER_HOME
         * \li FREECAD_USER_DATA
         * \li FREECAD_USER_TEMP
         * \endlist
         */
        static std::tuple<std::filesystem::path, std::filesystem::path, std::filesystem::path> getCustomPaths();

        /*!
         * Returns a tuple of XDG-compliant standard paths names where to store config, data and cached files.
         * The method therefore reads the environment variables:
         * \list
         * \li XDG_CONFIG_HOME
         * \li XDG_DATA_HOME
         * \li XDG_CACHE_HOME
         * \endlist
         */
        std::tuple<std::filesystem::path, std::filesystem::path, std::filesystem::path, std::filesystem::path> getStandardPaths();

        /// Find the BuildVersionMajor, BuildVersionMinor pair in the config map, convert them to an int tuple, and
        /// return it. If the pair is not found, or cannot be converted to integers, a RuntimeError is raised.
        /// \param config The config map to search.
        /// \return The version tuple.
        static std::tuple<int, int> extractVersionFromConfigMap(const std::map<std::string,std::string> &config);

        /// A utility method to remove any stray null characters from a path (Conda sometimes
        /// injects these for unknown reasons -- see #6892 in the bug tracker).
        /// \param pathAsString The std::string path to sanitize
        /// \returns A path with any stray nulls removed
        static std::filesystem::path sanitizePath(const std::string& pathAsString);

    private:
        std::tuple<int, int> _currentVersion;
        std::filesystem::path _home;
        std::filesystem::path _temp;
        std::filesystem::path _userCache;
        std::filesystem::path _userConfig;
        std::filesystem::path _userAppData;
        std::filesystem::path _userMacro;
        std::filesystem::path _userHome;
        std::filesystem::path _resource;
        std::filesystem::path _library;
        std::filesystem::path _help;

        bool _usingCustomDirectories {false};
    };

} // App
