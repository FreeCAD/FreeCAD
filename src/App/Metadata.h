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
 *   License along with this library; see the file LICENSE.html. If not,   *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef BASE_METADATAREADER_H 
#define BASE_METADATAREADER_H 

#include "FCConfig.h"

#include <boost/filesystem.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>


namespace App {

    namespace Meta {

        /**
         * \struct Contact
         * \brief A person or company representing a point of contact for the package (either author or maintainer).
         */
        struct AppExport Contact {
            Contact() = default;
            Contact(const std::string& name, const std::string& email);
            explicit Contact(const XERCES_CPP_NAMESPACE::DOMElement* e);
            std::string name; //< Contact name - required
            std::string email; //< Contact email - may be optional
        };

        /**
         * \struct License
         * \brief A license that covers some or all of this package.
         *
         * Many licenses also require the inclusion of the complete license text, specified in this struct
         * using the "file" member.
         */
        struct AppExport License {
            License() = default;
            License(const std::string& name, boost::filesystem::path file);
            explicit License(const XERCES_CPP_NAMESPACE::DOMElement* e);
            std::string name; //< Short name of license, e.g. "LGPL2", "MIT", "Mozilla Public License", etc.
            boost::filesystem::path file; //< Optional path to the license file, relative to the XML file's location
        };

        enum class UrlType {
            website,
            repository,
            bugtracker,
            readme,
            documentation
        };

        /**
         * \struct Url
         * \brief A URL, including type information (e.g. website, repository, or bugtracker, in package.xml)
         */
        struct AppExport Url {
            Url() = default;
            Url(const std::string& location, UrlType type);
            explicit Url(const XERCES_CPP_NAMESPACE::DOMElement* e);
            std::string location; //< The actual URL, including protocol
            UrlType type; //< What kind of URL this is
            std::string branch; //< If it's a repository, which branch to use
        };

        /**
         * \struct Version
         * A semantic version structure providing comparison operators and conversion to and from std::string
         */
        struct AppExport Version {
            Version();
            Version(int major, int minor = 0, int patch = 0, const std::string& suffix = std::string());
            explicit Version(const std::string& semanticString);

            int major;
            int minor;
            int patch;
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
         * \struct Dependency
         * \brief Another package that this package depends on, conflicts with, or replaces
         */
        struct AppExport Dependency {
            Dependency() = default;
            explicit Dependency(const XERCES_CPP_NAMESPACE::DOMElement* e);
            std::string package; //< Required: must exactly match the contents of the "name" element in the referenced package's package.xml file.
            std::string version_lt; //< Optional: The dependency to the package is restricted to versions less than the stated version number.
            std::string version_lte; //< Optional: The dependency to the package is restricted to versions less or equal than the stated version number.
            std::string version_eq; //< Optional: The dependency to the package is restricted to a version equal than the stated version number.
            std::string version_gte; //< Optional: The dependency to the package is restricted to versions greater or equal than the stated version number.
            std::string version_gt; //< Optional: The dependency to the package is restricted to versions greater than the stated version number.
            std::string condition; //< Optional: Conditional expression as documented in REP149.
        };

        /**
         * \struct GenericMetadata
         * A structure to hold unrecognized single-level metadata.
         *
         * Most unrecognized metadata is simple: when parsing the XML, if the parser finds a tag it
         * does not recognize, and that tag has no children, it is parsed into this data structure
         * for convenient access by client code.
         */
        struct AppExport GenericMetadata {
            GenericMetadata() = default;
            explicit GenericMetadata(const XERCES_CPP_NAMESPACE::DOMElement* e);
            explicit GenericMetadata(const std::string &contents);
            std::string contents; //< The contents of the tag
            std::map<std::string, std::string> attributes; //< The XML attributes of the tag
        };

    }

    /**
     * \class Metadata
     * \brief Reads data from a metadata file.
     *
     * The metadata format is based on https://ros.org/reps/rep-0149.html, modified for FreeCAD
     * use. Full format documentation is available at the FreeCAD Wiki:
     * https://wiki.freecadweb.org/Package_Metadata
     */
    class AppExport Metadata {
    public:

        Metadata();

        /**
         * Read the data from a file on disk
         *
         * This constructor takes a path to an XML file and loads the XML from that file as
         * metadata.
         */
        explicit Metadata(const boost::filesystem::path& metadataFile);

        /**
         * Construct a Metadata object from a DOM node.
         *
         * This node may have any tag name: it is only accessed via its children, which are
         * expected to follow the standard Metadata format for the contents of the <package> element.
         */
        Metadata(const XERCES_CPP_NAMESPACE::DOMNode* domNode, int format);

        ~Metadata();


        //////////////////////////////////////////////////////////////
        // Recognized Metadata
        //////////////////////////////////////////////////////////////

        std::string name() const; //< A short name for this package, often used as a menu entry.
        Meta::Version version() const; //< Version string in symantic triplet format, e.g. "1.2.3".
        std::string description() const; //< Text-only description of the package. No markup.
        std::vector<Meta::Contact> maintainer() const; //< Must be at least one, and must specify an email address.
        std::vector<Meta::License> license() const; //< Must be at least one, and most licenses require including a license file.
        std::vector<Meta::Url> url() const; //< Any number of URLs may be specified, but at least one repository URL must be included at the package level.
        std::vector<Meta::Contact> author() const; //< Any number of authors may be specified, and email addresses are optional.
        std::vector<Meta::Dependency> depend() const; //< Zero or more packages this package requires prior to use.
        std::vector<Meta::Dependency> conflict() const; //< Zero of more packages this package conflicts with.
        std::vector<Meta::Dependency> replace() const; //< Zero or more packages this package is intended to replace.
        std::vector<std::string> tag() const; //< Zero or more text tags related to this package.
        boost::filesystem::path icon() const; //< Path to an icon file.
        std::string classname() const; //< Recognized for convenience -- generally only used by Workbenches.
        boost::filesystem::path subdirectory() const; //< Optional, override the default subdirectory name for this item.
        std::vector<boost::filesystem::path> file() const; //< Arbitrary files associated with this package or content item.
        Meta::Version freecadmin() const; //< The minimum FreeCAD version.
        Meta::Version freecadmax() const; //< The maximum FreeCAD version.

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
        std::vector<Meta::GenericMetadata> operator[] (const std::string& tag) const;

        /**
         * Directly access the DOM tree to support unrecognized multi-level metadata
         */
        XERCES_CPP_NAMESPACE::DOMElement* dom() const;


        // Setters
        void setName(const std::string& name);
        void setVersion(const Meta::Version& version);
        void setDescription(const std::string& description);
        void addMaintainer(const Meta::Contact& maintainer);
        void addLicense(const Meta::License& license);
        void addUrl(const Meta::Url& url);
        void addAuthor(const Meta::Contact& author);
        void addDepend(const Meta::Dependency& dep);
        void addConflict(const Meta::Dependency& dep);
        void addReplace(const Meta::Dependency& dep);
        void addTag(const std::string& tag);
        void setIcon(const boost::filesystem::path& path);
        void setClassname(const std::string& name);
        void setSubdirectory(const boost::filesystem::path& path);
        void addFile(const boost::filesystem::path& path);
        void addContentItem(const std::string& tag, const Metadata& item);
        void setFreeCADMin(const Meta::Version& version);
        void setFreeCADMax(const Meta::Version& version);
        void addGenericMetadata(const std::string& tag, const Meta::GenericMetadata& genericMetadata);

        // Deleters (work in progress...)
        void removeContentItem(const std::string& tag, const std::string& itemName);

        /**
         * Write the metadata to an XML file
         */
        void write(const boost::filesystem::path& file) const;

        /**
         * Determine whether this package satisfies the given dependency
         */
        bool satisfies(const Meta::Dependency&);

        /**
         * Determine whether the current metadata specifies support for the currently-running version of FreeCAD.
         * Does not interrogate content items, which must be querried individually.
         */
        bool supportsCurrentFreeCAD() const;

    private:

        std::string _name;
        Meta::Version _version;
        std::string _description;
        std::vector<Meta::Contact> _maintainer;
        std::vector<Meta::License> _license;
        std::vector<Meta::Url> _url;
        std::vector<Meta::Contact> _author;
        std::vector<Meta::Dependency> _depend;
        std::vector<Meta::Dependency> _conflict;
        std::vector<Meta::Dependency> _replace;
        std::vector<std::string> _tag;
        boost::filesystem::path _icon;
        std::string _classname;
        boost::filesystem::path _subdirectory;
        std::vector<boost::filesystem::path> _file;
        Meta::Version _freecadmin;
        Meta::Version _freecadmax;

        std::multimap<std::string, Metadata> _content;

        std::multimap<std::string, Meta::GenericMetadata> _genericMetadata;

        XERCES_CPP_NAMESPACE::DOMElement* _dom;
        std::shared_ptr<XERCES_CPP_NAMESPACE::XercesDOMParser> _parser;

        void parseVersion1(const XERCES_CPP_NAMESPACE::DOMNode* startNode);
        void parseContentNodeVersion1(const XERCES_CPP_NAMESPACE::DOMElement* contentNode);

        void appendToElement(XERCES_CPP_NAMESPACE::DOMElement* root) const;

    };

}

#endif
