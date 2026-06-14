// SPDX-License-Identifier: LGPL-2.1-or-later

/**************************************************************************
 *                                                                         *
 *   Copyright (c) 2021-2023 FreeCAD Project Association                   *
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
 ***************************************************************************/

#pragma once

#include "FCConfig.h"

#include <filesystem>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>


namespace App
{

/**
 * @brief A namespace for metadata for Python external addons.
 *
 * This namespace contains functionality to define the contents of a
 * package.xml file, such as Contact, License, Url, and Dependency.
 */
namespace Meta
{

/**
 * \struct Contact
 * \brief A person or company representing a point of contact for the package (either author or
 * maintainer).
 */
struct AppExport Contact
{
    Contact() = default;
    Contact(std::string name, std::string email);
    explicit Contact(const XERCES_CPP_NAMESPACE::DOMElement* elem);
    std::string name;   //< Contact name - required
    std::string email;  //< Contact email - may be optional
    bool operator==(const Contact& rhs) const;
};

/**
 * \struct License
 * \brief A license that covers some or all of this package.
 *
 * Many licenses also require the inclusion of the complete license text, specified in this struct
 * using the "file" member.
 */
struct AppExport License
{
    License() = default;
    License(std::string name, std::filesystem::path file);
    explicit License(const XERCES_CPP_NAMESPACE::DOMElement* elem);
    std::string
        name;  //< Short name of license, e.g. "LGPL2", "MIT", "Mozilla Public License", etc.
    std::filesystem::path
        file;  //< Optional path to the license file, relative to the XML file's location
    bool operator==(const License& rhs) const;
};

enum class UrlType
{
    website,
    repository,
    bugtracker,
    readme,
    documentation,
    discussion
};

/**
 * \struct Url
 * \brief A URL, including type information (e.g. website, repository, or bugtracker, in
 * package.xml)
 */
struct AppExport Url
{
    Url();
    Url(std::string location, UrlType type);
    explicit Url(const XERCES_CPP_NAMESPACE::DOMElement* elem);
    std::string location;  //< The actual URL, including protocol
    UrlType type;          //< What kind of URL this is
    std::string branch;    //< If it's a repository, which branch to use
    bool operator==(const Url& rhs) const;
};

/**
 * \struct Version
 * A semantic version structure providing comparison operators and conversion to and from
 * std::string
 */
struct AppExport Version
{
    Version();
    explicit Version(int major, int minor = 0, int patch = 0, std::string suffix = std::string());
    explicit Version(const std::string& semanticString);

    int major {};
    int minor {};
    int patch {};
    std::string suffix;

    std::string str() const;

    bool operator<(const Version&) const;
    bool operator>(const Version&) const;
    bool operator<=(const Version&) const;
    bool operator>=(const Version&) const;
    bool operator==(const Version&) const;
    bool operator!=(const Version&) const;
};

/**
 * \enum DependencyType
 * The type of dependency.
 */
enum class DependencyType
{
    automatic,
    internal,
    addon,
    python
};

/**
 * \struct Dependency
 * \brief Another package that this package depends on, conflicts with, or replaces
 */
struct AppExport Dependency
{
    Dependency();
    explicit Dependency(std::string pkg);
    explicit Dependency(const XERCES_CPP_NAMESPACE::DOMElement* elem);
    std::string package;  //< Required: must exactly match the contents of the "name" element in the
                          // referenced package's package.xml file.
    std::string version_lt;   //< Optional: The dependency to the package is restricted to versions
                              // less than the stated version number.
    std::string version_lte;  //< Optional: The dependency to the package is restricted to versions
                              // less or equal than the stated version number.
    std::string version_eq;   //< Optional: The dependency to the package is restricted to a version
                              // equal than the stated version number.
    std::string version_gte;  //< Optional: The dependency to the package is restricted to versions
                              // greater or equal than the stated version number.
    std::string version_gt;   //< Optional: The dependency to the package is restricted to versions
                              // greater than the stated version number.
    std::string condition;    //< Optional: Conditional expression as documented in REP149.
    bool optional;            //< Optional: Whether this dependency is considered "optional"
    DependencyType dependencyType;  //< Optional: defaults to "automatic"
    bool operator==(const Dependency& rhs) const;
};

/**
 * \struct GenericMetadata
 * A structure to hold unrecognized single-level metadata.
 *
 * Most unrecognized metadata is simple: when parsing the XML, if the parser finds a tag it
 * does not recognize, and that tag has no children, it is parsed into this data structure
 * for convenient access by client code.
 */
struct AppExport GenericMetadata
{
    GenericMetadata() = default;
    explicit GenericMetadata(const XERCES_CPP_NAMESPACE::DOMElement* elem);
    explicit GenericMetadata(std::string contents);
    std::string contents;                           //< The contents of the tag
    std::map<std::string, std::string> attributes;  //< The XML attributes of the tag
};

}  // namespace Meta

/**
 * \class Metadata
 * \brief Reads data from a metadata file.
 *
 * The metadata format is based on https://ros.org/reps/rep-0149.html, modified for FreeCAD
 * use. Full format documentation is available at the FreeCAD Wiki:
 * https://wiki.freecad.org/Package_Metadata
 */
class AppExport Metadata
{
public:
    Metadata();

    /**
     * Read the data from a file on disk
     *
     * This constructor takes a path to an XML file and loads the XML from that file as
     * metadata.
     */
    explicit Metadata(const std::filesystem::path& metadataFile);

    /**
     * Construct a Metadata object from a DOM node.
     *
     * This node may have any tag name: it is only accessed via its children, which are
     * expected to follow the standard Metadata format for the contents of the <package> element.
     */
    Metadata(const XERCES_CPP_NAMESPACE::DOMNode* domNode, int format);

    /**
     * Treat the incoming rawData as metadata to be parsed.
     */
    explicit Metadata(const std::string& rawData);

    ~Metadata();


    //////////////////////////////////////////////////////////////
    // Recognized Metadata
    //////////////////////////////////////////////////////////////

    std::string name() const;       //< A short name for this package, often used as a menu entry.
    std::string type() const;       //< The type for this package.
    Meta::Version version() const;  //< Version string in semantic triplet format, e.g. "1.2.3".
    std::string date()
        const;  //< Date string -- currently arbitrary (when C++20 is well-supported we can revisit)
    std::string description() const;  //< Text-only description of the package. No markup.
    std::vector<Meta::Contact>
    maintainer() const;  //< Must be at least one, and must specify an email address.
    std::vector<Meta::License>
    license() const;  //< Must be at least one, and most licenses require including a license file.
    std::vector<Meta::Url> url() const;  //< Any number of URLs may be specified, but at least one
                                         // repository URL must be included at the package level.
    std::vector<Meta::Contact>
    author() const;  //< Any number of authors may be specified, and email addresses are optional.
    std::vector<Meta::Dependency>
    depend() const;  //< Zero or more packages this package requires prior to use.
    std::vector<Meta::Dependency>
    conflict() const;  //< Zero of more packages this package conflicts with.
    std::vector<Meta::Dependency>
    replace() const;  //< Zero or more packages this package is intended to replace.
    std::vector<std::string> tag() const;  //< Zero or more text tags related to this package.
    std::filesystem::path icon() const;  //< Path to an icon file.
    std::string
    classname() const;  //< Recognized for convenience -- generally only used by Workbenches.
    std::filesystem::path
    subdirectory() const;  //< Optional, override the default subdirectory name for this item.
    std::vector<std::filesystem::path>
    file() const;  //< Arbitrary files associated with this package or content item.
    Meta::Version freecadmin() const;  //< The minimum FreeCAD version.
    Meta::Version freecadmax() const;  //< The maximum FreeCAD version.
    Meta::Version pythonmin() const;   //< The minimum Python version.

    /**
     * Access the metadata for the content elements of this package
     *
     * In addition to the overall package metadata, this class reads in metadata contained in a
     * <content> element. Each entry in the content element is an element representing some
     * type of package content (e.g. add-on, macro, theme, etc.). This class places no restriction
     * on the types, it is up to client code to place requirements on the metadata included
     * here.
     *
     * For example, themes might be specified:
     * <content>
     *   <theme>
     *     <name>High Contrast</name>
     *   </theme>
     * </content>
     */
    std::multimap<std::string, Metadata> content() const;

    /**
     * Convenience accessor for unrecognized simple metadata.
     *
     * If the XML parser encounters tags that it does not recognize, and those tags have
     * no children, a GenericMetadata object is created. Those objects can be accessed using
     * operator[], which returns a (potentially empty) vector containing all instances of the
     * given tag. It cannot be used to *create* a new tag, however. See addGenericMetadata().
     */
    std::vector<Meta::GenericMetadata> operator[](const std::string& tag) const;

    /**
     * Directly access the DOM tree to support unrecognized multi-level metadata
     */
    XERCES_CPP_NAMESPACE::DOMElement* dom() const;


    // Setters
    void setName(const std::string& name);
    void setType(const std::string& type);
    void setVersion(const Meta::Version& version);
    void setDate(const std::string& date);
    void setDescription(const std::string& description);
    void addMaintainer(const Meta::Contact& maintainer);
    void addLicense(const Meta::License& license);
    void addUrl(const Meta::Url& url);
    void addAuthor(const Meta::Contact& author);
    void addDepend(const Meta::Dependency& dep);
    void addConflict(const Meta::Dependency& dep);
    void addReplace(const Meta::Dependency& dep);
    void addTag(const std::string& tag);
    void setIcon(const std::filesystem::path& path);
    void setClassname(const std::string& name);
    void setSubdirectory(const std::filesystem::path& path);
    void addFile(const std::filesystem::path& path);
    void addContentItem(const std::string& tag, const Metadata& item);
    void setFreeCADMin(const Meta::Version& version);
    void setFreeCADMax(const Meta::Version& version);
    void setPythonMin(const Meta::Version& version);
    void addGenericMetadata(const std::string& tag, const Meta::GenericMetadata& genericMetadata);

    // Deleters
    void removeContentItem(const std::string& tag, const std::string& itemName);
    void removeMaintainer(const Meta::Contact& maintainer);
    void removeLicense(const Meta::License& license);
    void removeUrl(const Meta::Url& url);
    void removeAuthor(const Meta::Contact& author);
    void removeDepend(const Meta::Dependency& dep);
    void removeConflict(const Meta::Dependency& dep);
    void removeReplace(const Meta::Dependency& dep);
    void removeTag(const std::string& tag);
    void removeFile(const std::filesystem::path& path);

    // Utility functions to clear lists
    void clearContent();
    void clearMaintainer();
    void clearLicense();
    void clearUrl();
    void clearAuthor();
    void clearDepend();
    void clearConflict();
    void clearReplace();
    void clearTag();
    void clearFile();

    /**
     * Write the metadata to an XML file
     */
    void write(const std::filesystem::path& file) const;

    /**
     * Determine whether this package satisfies the given dependency
     */
    bool satisfies(const Meta::Dependency&);

    /**
     * Determine whether the current metadata specifies support for the currently-running version of
     * FreeCAD. Does not interrogate content items, which must be queried individually.
     */
    bool supportsCurrentFreeCAD() const;

private:
    std::string _name;
    std::string _type;
    Meta::Version _version;
    std::string _date;
    std::string _description;
    std::vector<Meta::Contact> _maintainer;
    std::vector<Meta::License> _license;
    std::vector<Meta::Url> _url;
    std::vector<Meta::Contact> _author;
    std::vector<Meta::Dependency> _depend;
    std::vector<Meta::Dependency> _conflict;
    std::vector<Meta::Dependency> _replace;
    std::vector<std::string> _tag;
    std::filesystem::path _icon;
    std::string _classname;
    std::filesystem::path _subdirectory;
    std::vector<std::filesystem::path> _file;
    Meta::Version _freecadmin;
    Meta::Version _freecadmax;
    Meta::Version _pythonmin;

    std::multimap<std::string, Metadata> _content;

    std::multimap<std::string, Meta::GenericMetadata> _genericMetadata;

    XERCES_CPP_NAMESPACE::DOMElement* _dom;
    std::shared_ptr<XERCES_CPP_NAMESPACE::XercesDOMParser> _parser;

    void loadFromInputSource(const XERCES_CPP_NAMESPACE::InputSource& source);
    void parseVersion1(const XERCES_CPP_NAMESPACE::DOMNode* startNode);
    void parseContentNodeVersion1(const XERCES_CPP_NAMESPACE::DOMElement* contentNode);

    void appendToElement(XERCES_CPP_NAMESPACE::DOMElement* root) const;
};

}  // namespace App
