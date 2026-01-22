// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Base/Type.h>
#include <zipios++/zipfile.h>
#include <sstream>
#include <list>
#include <map>
#include <string>
#include <xercesc/util/XercesDefs.hpp>

namespace XERCES_CPP_NAMESPACE
{
class DOMNode;
class DOMElement;
class DOMDocument;
}  // namespace XERCES_CPP_NAMESPACE

namespace App
{

class PropertyContainer;
class Property;

/**
 * @code
 * ProjectFile proj(project.fcstd);
 * if (proj.loadDocument()) {
 *     std::list<std::string> names = proj.getObjects();
 *     std::list<std::string> files = proj.getInputFiles(names.front().name);
 *     std::string meshFile = proj.extractInputFile(files.front());
 *     std::ifstream str(meshFile.c_str(), std::ios::in | std::ios::binary);
 *     Mesh::MeshObject mesh;
 *     mesh.load(str);
 * }
 * @endcode
 */
class AppExport ProjectFile
{
public:
    struct Object
    {
        std::string name;
        Base::Type type;
    };
    struct PropertyFile
    {
        std::string file;
        std::string name;
        Base::Type type;
    };
    struct Metadata
    {
        std::string comment;
        std::string company;
        std::string createdBy;
        std::string creationDate;
        std::string label;
        std::string lastModifiedBy;
        std::string lastModifiedDate;
        std::string license;
        std::string licenseURL;
        std::string programVersion;
        std::string uuid;
    };
    /**
     * Default constructor
     */
    ProjectFile();
    /**
     * Creates an instance of ProjectFile pointing to a project file.
     * You have to call @ref loadDocument() afterwards.
     */
    explicit ProjectFile(std::string zipArchive);
    /**
     * Destructor
     */
    ~ProjectFile();
    /**
     * Copy/move constructor & assignment
     */
    ProjectFile(const ProjectFile&) = delete;
    ProjectFile(ProjectFile&&) = delete;
    ProjectFile& operator=(const ProjectFile&) = delete;
    ProjectFile& operator=(ProjectFile&&) = delete;
    /**
     * Assigns the file name of another project file. This clears any internal data, thus
     * @ref loadDocument() needs to be called afterwards.
     */
    void setProjectFile(const std::string& zipArchive);
    /**
     * Loads the embedded document file of the project file. This is needed to query any information
     * about objects, their type ids or any referenced input files.
     */
    bool loadDocument();
    /**
     * Return the meta data of the loaded document.
     */
    Metadata getMetadata() const;
    /**
     * Returns a list of all object names with their type ids which are part of the project file.
     */
    std::list<Object> getObjects() const;
    /**
     * Returns a list of object names of a given type id which are part of the project file.
     */
    std::list<std::string> getObjectsOfType(const Base::Type&) const;
    /**
     * Restores the object identified by its @a name and loads all properties of @a prop
     * that match with the saved properties.
     */
    bool restoreObject(const std::string& name, App::PropertyContainer*, bool verbose = true);
    /**
     * Retrieves the type id of a given object name.
     */
    Base::Type getTypeId(const std::string& name) const;
    /**
     * Retrieves a list of input file names and their associated property names and type ids
     * to the given object name.
     * @see getInputFiles().
     */
    std::list<PropertyFile> getPropertyFiles(const std::string& name) const;
    /**
     * If the project file contains the file \a name true is returned and
     * false otherwise
     */
    bool containsFile(const std::string& name) const;
    /**
     * Retrieves a list of input file names referenced to the given object name.
     * This method does the same as @ref getPropertyFiles() unless that it only
     * returns the file names.
     */
    std::list<std::string> getInputFiles(const std::string& name) const;
    /**
     * Extracts an input file of @a name and saves it as file to the disk.
     * It's up to the client code to delete the file if needed.
     */
    std::string extractInputFile(const std::string& name);
    /**
     * Extracts, via a temporary file the content of an input file of @a name.
     */
    void readInputFile(const std::string& name, std::ostream& str);
    /**
     * Directly extracts the content of an input file of @a name.
     */
    void readInputFileDirect(const std::string& name, std::ostream& str);
    /**
     * Replaces the input file @a name with the content of the given @a stream.
     * The method returns the file name of the newly created project file.
     */
    std::string replaceInputFile(const std::string& name, std::istream& inp);
    /**
     * Replaces the input files with the streams of @a inp.
     * The method returns the file name of the newly created project file.
     * If you want to replace the original project file you must call the
     * method @ref replaceProjectFile() with the returned file name as argument.
     */
    std::string replaceInputFiles(const std::map<std::string, std::istream*>& inp);
    /**
     * Replaces the property files with the content of the properties in @a props.
     * The method returns the file name of the newly created project file.
     * If you want to replace the original project file you must call the
     * method @ref replaceProjectFile() with the returned file name as argument.
     */
    std::string replacePropertyFiles(const std::map<std::string, App::Property*>& props);
    /**
     * Replaces the project file with the given @a name. If @a keepfile is true
     * then the renamed original file won't be deleted.
     * Returns true if the replacement succeeded and false otherwise.
     * If you want to replace the original project file you must call the
     * method @ref replaceProjectFile() with the returned file name as argument.
     */
    bool replaceProjectFile(const std::string& name, bool keepfile = false);

private:
    void findFiles(XERCES_CPP_NAMESPACE::DOMNode*, std::list<std::string>&) const;
    void findFiles(XERCES_CPP_NAMESPACE::DOMNode*, std::list<PropertyFile>&) const;

private:
    std::string stdFile;
    XERCES_CPP_NAMESPACE::DOMDocument* xmlDocument;
};


}  // namespace App
