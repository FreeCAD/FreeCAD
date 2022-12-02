/***************************************************************************
 *   Copyright (c) 2021 Chris Hennes <chennes@pioneerlibrarysystem.org>    *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef BASE_PREFERENCEPACKMANAGER_H
#define BASE_PREFERENCEPACKMANAGER_H

#include <vector>
#include <string>
#include <mutex>

#include "App/Metadata.h"

namespace Gui {

    /**
     * \class PreferencePack A collection of user preferences stored in files on disk
     */
    class PreferencePack {

    public:

        /**
         * Construct a preferencePack from a directory
         *
         * \param path A path to a mod directory that contains a preferencePack
         * \param metadata The metadata from the package.xml file describing this preferencePack
         */
        PreferencePack(const boost::filesystem::path& path, const App::Metadata& metadata);

        ~PreferencePack() = default;

        /**
         * Get the name of the PreferencePack
         */
        std::string name() const;

        /**
         * Apply the PreferencePack over the top of the current preferences set
         * \returns True if the preferencePack was applied, or false if not
         */
        bool apply() const;

        /**
         * Get the complete metadata object for this preference pack
         */
        App::Metadata metadata() const;

    private:

        void applyConfigChanges() const;

        boost::filesystem::path _path;
        App::Metadata _metadata;

    };




    /**
     * \class PreferencePackManager handles storable and loadable collections of user preferences
     *
     * This class provides some additional utility functions for allowing users to save their current
     * preferences as a PreferencePack based on a set of template files provided either in the main
     * FreeCAD distribution, or inside various installed mods.
     */
    class PreferencePackManager {
    public:
        PreferencePackManager();
        ~PreferencePackManager() = default;

        /**
         * Rescan the preferencePack directory and update the available PreferencePacks
         */
        void rescan();

        /**
         * Get an alphabetical list of names of all visible PreferencePacks
         */
        std::vector<std::string> preferencePackNames() const;

        /**
         * Get a map of all visible PreferencePack names and their associated packs
         */
        std::map<std::string, PreferencePack> preferencePacks() const;

        /**
         * Apply the named preferencePack
         * \return True if the preferencePack was applied, or false if it was not
         */
        bool apply(const std::string& preferencePackName) const;

        /**
         * Check the visibility of the specified pack
         * \return True if the preferencePack is visible, or false if not. All packs are visible by default,
         * but can be marked as "invisible" (i.e. not returned by the manager in lists of packs) by using the
         * toggleVisibility function.
         */
        bool isVisible(const std::string& addonName, const std::string& preferencePackName) const;

        /**
         * Toggle the visibility of the named preference pack in a named addon
         */
        void toggleVisibility(const std::string& addonName, const std::string& preferencePackName);

        /**
         * Deletes the user-saved pack specified by name
         */
        void deleteUserPack(const std::string& name);


        /**
         * \struct TemplateFile A file containing a set of preferences that can be saved into
         * a PreferencePack
         *
         * PreferencePacks can contain any parameters at all, but inside FreeCAD there is no
         * centralized list of all of these parameters, and at any given time the user.cfg file
         * usually does not store a value for all possible parameters. Instead, it simply allows
         * calling code to use whatever default values that code sets. This is all completely
         * hidden from the users: FreeCAD behaves as though those values exist in the config file.
         *
         * When a user saves their current configuration as a pack, they likely expect that saved
         * configuration to include those default values, so that if they later apply their saved
         * configuration those defaults are restored. To enable this a set of template files
         * listing default values for various types of parameters can be used. Each file is
         * presented to the user as a checkable box when saving a new preferences pack, and the
         * intention is that these files will list out the various user-facing parameters that
         * someone might want to save into a preferences pack.
         *
         * These files are themselves valid user.cfg files, that if loaded and applied will result
         * in the default values of their contained variables being set. For this to work, these
         * files should be kept up-to-date with the default values set in the code. They are not
         * required to contain an exhaustive listing of all possible parameters: in most cases it
         * is enough that they list the variables that a user would expect for a given name. For
         * example, "Sketcher Colors.cfg" should list out all of the default colors used in
         * Sketcher that a user can set in Preferences, but it is not necessary that it contain any
         * color that is only used internally, and it should not include things like fonts, or
         * behavior information.
         *
         * The base FreeCAD installation includes default templates in:
         *    $INSTALL_DIR/data/Gui/PreferencePackTemplates/
         *
         * External add-ons are also searched for any directory called PreferencePackTemplates or
         * preference_pack_templates -- either of which is expected to contain appearance and/or
         * behavior subdirectories. In this way external add-ons can allow a user to easily save
         * their preferences to a PreferencePack, or even to add additional templates representing
         * sets of core FreeCAD preferences.
         */
        struct TemplateFile {
            std::string group; // Generally the Add-On/Mod/Package name
            std::string name;
            boost::filesystem::path path;
        };

        /**
         * Save current settings as a (possibly new) preferencePack
         *
         * If the named preferencePack does not exist, this creates it on disk. If it does exist, this overwrites the original.
         */
        void save(const std::string& name, const std::vector<TemplateFile>& templates);


        std::vector<TemplateFile> templateFiles(bool rescan = false);

        /**
         * Get a list of all available config file backups. Backups are currently stored for one week.
         */
        std::vector<boost::filesystem::path> configBackups() const;

        /**
         * Import an existing config file as a preference pack with a given name.
         */
        void importConfig(const std::string &packName, const boost::filesystem::path &path);

    private:

        void FindPreferencePacksInPackage(const boost::filesystem::path& mod);

        void BackupCurrentConfig() const;

        void DeleteOldBackups() const;

        void AddPackToMetadata(const std::string &packName) const;

        std::vector<boost::filesystem::path> _preferencePackPaths;
        std::vector<TemplateFile> _templateFiles;
        std::map<std::string, PreferencePack> _preferencePacks;
        mutable std::mutex _mutex;

    };

}

Q_DECLARE_METATYPE(Gui::PreferencePackManager::TemplateFile) // So it can be used with QVariant


#endif
