/***************************************************************************
 *   Copyright (c) 2022 FreeCAD Project Association                        *
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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <memory>
# include <sstream>
#endif

#include "Metadata.h"

#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>

#include "App/Application.h"
#include "App/Expression.h"
#include "Base/XMLTools.h"

/*
*** From GCC: ***
In the GNU C Library, "major" and "minor" are defined
by <sys/sysmacros.h>. For historical compatibility, it is
currently defined by <sys/types.h> as well, but we plan to
remove this soon. To use "major", include <sys/sysmacros.h>
directly. If you did not intend to use a system-defined macro
"major", you should undefine it after including <sys/types.h>.
*/
#ifdef major
#undef major
#endif
#ifdef minor
#undef minor
#endif

using namespace App;
namespace fs = boost::filesystem;
XERCES_CPP_NAMESPACE_USE

namespace MetadataInternal {
    class XMLErrorHandler : public HandlerBase {
        void warning(const SAXParseException& toCatch)
        {
            // Don't deal with warnings at all
            std::ignore = toCatch;
        }

        void error(const SAXParseException& toCatch)
        {
            std::stringstream message;
            message << "Error at file \"" << StrX(toCatch.getSystemId())
                << "\", line " << toCatch.getLineNumber()
                << ", column " << toCatch.getColumnNumber()
                << "\n   Message: " << StrX(toCatch.getMessage()) << std::endl;
            throw Base::XMLBaseException(message.str());
        }

        void fatalError(const SAXParseException& toCatch)
        {
            std::stringstream message;
            message << "Fatal error at file \"" << StrX(toCatch.getSystemId())
                << "\", line " << toCatch.getLineNumber()
                << ", column " << toCatch.getColumnNumber()
                << "\n   Message: " << StrX(toCatch.getMessage()) << std::endl;
            throw Base::XMLBaseException(message.str());
        }
    };
}

Metadata::Metadata(const fs::path& metadataFile)
{
    // Any exception thrown by the XML code propagates out and prevents object creation
    XMLPlatformUtils::Initialize();

    _parser = std::make_shared<XercesDOMParser>();
    _parser->setValidationScheme(XercesDOMParser::Val_Never);
    _parser->setDoNamespaces(true);

    auto errHandler = std::make_unique<MetadataInternal::XMLErrorHandler>();
    _parser->setErrorHandler(errHandler.get());

    _parser->parse(metadataFile.string().c_str());

    auto doc = _parser->getDocument();
    _dom = doc->getDocumentElement();

    auto rootTagName = StrXUTF8(_dom->getTagName()).str;
    if (rootTagName != "package")
        throw Base::XMLBaseException("Malformed package.xml document: Root <package> group not found");

    auto formatVersion = XMLString::parseInt(_dom->getAttribute(XUTF8Str("format").unicodeForm()));
    switch (formatVersion) {
    case 1:
        parseVersion1(_dom);
        break;
    default:
        throw Base::XMLBaseException("package.xml format version is not supported by this version of FreeCAD");
    }
}

Metadata::Metadata() : _dom(nullptr)
{
}

Metadata::Metadata(const DOMNode* domNode, int format) : _dom(nullptr)
{
    auto element = dynamic_cast<const DOMElement*>(domNode);
    if (element) {
        switch (format) {
        case 1:
            parseVersion1(element);
            break;
        default:
            throw Base::XMLBaseException("package.xml format version is not supported by this version of FreeCAD");
        }
    }
}

Metadata::~Metadata()
{
}

std::string Metadata::name() const
{
    return _name;
}

Meta::Version Metadata::version() const
{
    return _version;
}

std::string Metadata::description() const
{
    return _description;
}

std::vector<Meta::Contact> Metadata::maintainer() const
{
    return _maintainer;
}

std::vector<Meta::License> Metadata::license() const
{
    return _license;
}

std::vector<Meta::Url> Metadata::url() const
{
    return _url;
}

std::vector<Meta::Contact> Metadata::author() const
{
    return _author;
}

std::vector<Meta::Dependency> Metadata::depend() const
{
    return _depend;
}

std::vector<Meta::Dependency> Metadata::conflict() const
{
    return _conflict;
}

std::vector<Meta::Dependency> Metadata::replace() const
{
    return _replace;
}

std::vector<std::string> Metadata::tag() const
{
    return _tag;
}

fs::path Metadata::icon() const
{
    return _icon;
}

std::string Metadata::classname() const
{
    return _classname;
}

boost::filesystem::path App::Metadata::subdirectory() const
{
    return _subdirectory;
}

std::vector<fs::path> Metadata::file() const
{
    return _file;
}

Meta::Version App::Metadata::freecadmin() const
{
    return _freecadmin;
}

Meta::Version App::Metadata::freecadmax() const
{
    return _freecadmax;
}

std::multimap<std::string, Metadata> Metadata::content() const
{
    return _content;
}

std::vector<Meta::GenericMetadata> Metadata::operator[](const std::string& tag) const
{
    std::vector<Meta::GenericMetadata> returnValue;
    auto range = _genericMetadata.equal_range(tag);
    for (auto item = range.first; item != range.second; ++item)
        returnValue.push_back(item->second);
    return returnValue;
}

XERCES_CPP_NAMESPACE::DOMElement* Metadata::dom() const
{
    return _dom;
}

void Metadata::setName(const std::string& name)
{
    std::string invalidCharacters = "/\\?%*:|\"<>"; // Should cover all OSes
    if (_name.find_first_of(invalidCharacters) != std::string::npos)
        throw Base::RuntimeError("Name cannot contain any of: " + invalidCharacters);

    _name = name;
}

void Metadata::setVersion(const Meta::Version& version)
{
    _version = version;
}

void Metadata::setDescription(const std::string& description)
{
    _description = description;
}

void Metadata::addMaintainer(const Meta::Contact& maintainer)
{
    _maintainer.push_back(maintainer);
}

void Metadata::addLicense(const Meta::License& license)
{
    _license.push_back(license);
}

void Metadata::addUrl(const Meta::Url& url)
{
    _url.push_back(url);
}

void Metadata::addAuthor(const Meta::Contact& author)
{
    _author.push_back(author);
}

void Metadata::addDepend(const Meta::Dependency& dep)
{
    _depend.push_back(dep);
}

void Metadata::addConflict(const Meta::Dependency& dep)
{
    _conflict.push_back(dep);
}

void Metadata::addReplace(const Meta::Dependency& dep)
{
    _replace.push_back(dep);
}

void Metadata::addTag(const std::string& tag)
{
    _tag.push_back(tag);
}

void Metadata::setIcon(const fs::path& path)
{
    _icon = path;
}

void Metadata::setClassname(const std::string& name)
{
    _classname = name;
}

void App::Metadata::setSubdirectory(const boost::filesystem::path& path)
{
    _subdirectory = path;
}

void Metadata::addFile(const fs::path& path)
{
    _file.push_back(path);
}

void Metadata::addContentItem(const std::string& tag, const Metadata& item)
{
    _content.insert(std::make_pair(tag, item));
}

void App::Metadata::setFreeCADMin(const Meta::Version& version)
{
    _freecadmin = version;
}

void App::Metadata::setFreeCADMax(const Meta::Version& version)
{
    _freecadmax = version;
}

void App::Metadata::addGenericMetadata(const std::string& tag, const Meta::GenericMetadata& genericMetadata)
{
    _genericMetadata.insert(std::make_pair(tag, genericMetadata));
}

void App::Metadata::removeContentItem(const std::string& tag, const std::string& itemName)
{
    auto tagRange = _content.equal_range(tag);
    auto foundItem = std::find_if(tagRange.first, tagRange.second, [&itemName](const auto& check) -> bool { return itemName == check.second.name(); });
    if (foundItem != tagRange.second)
        _content.erase(foundItem);
}


DOMElement* appendSimpleXMLNode(DOMElement* baseNode, const std::string& nodeName, const std::string& nodeContents)
{
    // For convenience (and brevity of final output) don't create nodes that don't have contents
    if (nodeContents.empty())
        return nullptr;

    auto doc = baseNode->getOwnerDocument();
    DOMElement* namedElement = doc->createElement(XUTF8Str(nodeName.c_str()).unicodeForm());
    baseNode->appendChild(namedElement);
    DOMText* namedNode = doc->createTextNode(XUTF8Str(nodeContents.c_str()).unicodeForm());
    namedElement->appendChild(namedNode);
    return namedElement;
}

void addAttribute(DOMElement* node, const std::string& key, const std::string& value)
{
    if (value.empty())
        return;

    node->setAttribute(XUTF8Str(key.c_str()).unicodeForm(), XUTF8Str(value.c_str()).unicodeForm());
}

void addDependencyNode(DOMElement* root, const std::string& name, const Meta::Dependency& depend)
{
    auto element = appendSimpleXMLNode(root, name, depend.package);
    if (element) {
        addAttribute(element, "version_lt", depend.version_lt);
        addAttribute(element, "version_lte", depend.version_lte);
        addAttribute(element, "version_eq", depend.version_eq);
        addAttribute(element, "version_gte", depend.version_gte);
        addAttribute(element, "version_gt", depend.version_gt);
        addAttribute(element, "condition", depend.condition);
    }
}

void Metadata::write(const fs::path& file) const
{
    DOMImplementation* impl = DOMImplementationRegistry::getDOMImplementation(XUTF8Str("Core LS").unicodeForm());

    DOMDocument* doc = impl->createDocument(nullptr, XUTF8Str("package").unicodeForm(), nullptr);
    DOMElement* root = doc->getDocumentElement();
    root->setAttribute(XUTF8Str("format").unicodeForm(), XUTF8Str("1").unicodeForm());

    appendToElement(root);

    DOMLSSerializer* theSerializer = ((DOMImplementationLS*)impl)->createLSSerializer();
    DOMConfiguration* config = theSerializer->getDomConfig();
    if (config->canSetParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true))
        config->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true);

    // set feature if the serializer supports the feature/mode
    if (config->canSetParameter(XMLUni::fgDOMWRTSplitCdataSections, true))
        config->setParameter(XMLUni::fgDOMWRTSplitCdataSections, true);

    if (config->canSetParameter(XMLUni::fgDOMWRTDiscardDefaultContent, true))
        config->setParameter(XMLUni::fgDOMWRTDiscardDefaultContent, true);

    try {
        XMLFormatTarget* myFormTarget = new LocalFileFormatTarget(file.string().c_str());
        DOMLSOutput* theOutput = ((DOMImplementationLS*)impl)->createLSOutput();

        theOutput->setByteStream(myFormTarget);
        theSerializer->write(doc, theOutput);

        theOutput->release();
        theSerializer->release();
        delete myFormTarget;
    }
    catch (const XMLException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        std::string what = message;
        XMLString::release(&message);
        throw Base::XMLBaseException(what);
    }
    catch (const DOMException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        std::string what = message;
        XMLString::release(&message);
        throw Base::XMLBaseException(what);
    }

    doc->release();
}

bool Metadata::satisfies(const Meta::Dependency& dep)
{
    if (dep.package != _name)
        return false;

    // The "condition" attribute allows an expression to enable or disable this dependency check: it must contain a valid
    // FreeCAD Expression. If it evaluates to false, this dependency is bypassed (e.g. this function returns false).
    if (!dep.condition.empty()) {
        auto injectedString = dep.condition;
        std::map<std::string, std::string> replacements;
        std::map<std::string, std::string>& config = App::Application::Config();
        replacements.insert(std::make_pair("$BuildVersionMajor", config["BuildVersionMajor"]));
        replacements.insert(std::make_pair("$BuildVersionMinor", config["BuildVersionMinor"]));
        replacements.insert(std::make_pair("$BuildRevision", config["BuildRevision"]));
        for (const auto& replacement : replacements) {
            auto pos = injectedString.find(replacement.first);
            while (pos != std::string::npos) {
                injectedString.replace(pos, replacement.first.length(), replacement.second);
                pos = injectedString.find(replacement.first);
            }
        }
        auto parsedExpression = App::Expression::parse(nullptr, dep.condition);
        auto result = parsedExpression->eval();
        if (!boost::any_cast<bool> (result->getValueAsAny()))
            return false;
    }

    if (!dep.version_eq.empty())
        return _version == Meta::Version(dep.version_eq);

    // Any of the others might be specified in pairs, so only return the "false" case

    if (!dep.version_lt.empty())
        if (!(_version < Meta::Version(dep.version_lt)))
            return false;

    if (!dep.version_lte.empty())
        if (!(_version <= Meta::Version(dep.version_lt)))
            return false;

    if (!dep.version_gt.empty())
        if (!(_version > Meta::Version(dep.version_lt)))
            return false;

    if (!dep.version_gte.empty())
        if (!(_version >= Meta::Version(dep.version_lt)))
            return false;

    return true;
}

bool App::Metadata::supportsCurrentFreeCAD() const
{
    static auto fcVersion = Meta::Version();
    if (fcVersion == Meta::Version()) {
        std::map<std::string, std::string>& config = App::Application::Config();
        std::stringstream ss;
        ss << config["BuildVersionMajor"] << "." << config["BuildVersionMinor"] << "." << (config["BuildRevision"].empty() ? "0" : config["BuildRevision"]);
        fcVersion = Meta::Version(ss.str());
    }

    if (_freecadmin != Meta::Version() && _freecadmin > fcVersion)
        return false;
    else if (_freecadmax != Meta::Version() && _freecadmax < fcVersion)
        return false;
    return true;
}

void Metadata::appendToElement(DOMElement* root) const
{
    appendSimpleXMLNode(root, "name", _name);
    appendSimpleXMLNode(root, "description", _description);
    appendSimpleXMLNode(root, "version", _version.str());

    for (const auto& maintainer : _maintainer) {
        auto element = appendSimpleXMLNode(root, "maintainer", maintainer.name);
        if (element)
            addAttribute(element, "email", maintainer.email);
    }

    for (const auto& license : _license) {
        auto element = appendSimpleXMLNode(root, "license", license.name);
        if (element)
            addAttribute(element, "file", license.file.string());
    }

    if (_freecadmin != Meta::Version())
        appendSimpleXMLNode(root, "freecadmin", _freecadmin.str());

    if (_freecadmax != Meta::Version())
        appendSimpleXMLNode(root, "freecadmax", _freecadmin.str());

    for (const auto& url : _url) {
        auto element = appendSimpleXMLNode(root, "url", url.location);
        if (element) {
            std::string typeAsString("website");
            switch (url.type) {
            case Meta::UrlType::website:       typeAsString = "website";       break;
            case Meta::UrlType::repository:    typeAsString = "repository";    break;
            case Meta::UrlType::bugtracker:    typeAsString = "bugtracker";    break;
            case Meta::UrlType::readme:        typeAsString = "readme";        break;
            case Meta::UrlType::documentation: typeAsString = "documentation"; break;
            }
            addAttribute(element, "type", typeAsString);
            if (url.type == Meta::UrlType::repository) {
                addAttribute(element, "branch", url.branch);
            }
        }
    }

    for (const auto& author : _author) {
        auto element = appendSimpleXMLNode(root, "author", author.name);
        if (element)
            addAttribute(element, "email", author.email);
    }

    for (const auto& depend : _depend)
        addDependencyNode(root, "depend", depend);

    for (const auto& conflict : _conflict)
        addDependencyNode(root, "conflict", conflict);

    for (const auto& replace : _replace)
        addDependencyNode(root, "replace", replace);

    for (const auto& tag : _tag)
        appendSimpleXMLNode(root, "tag", tag);

    appendSimpleXMLNode(root, "icon", _icon.string());

    appendSimpleXMLNode(root, "classname", _classname);

    appendSimpleXMLNode(root, "subdirectory", _subdirectory.string());

    for (const auto& file : _file)
        appendSimpleXMLNode(root, "file", file.string());

    for (const auto& md : _genericMetadata) {
        auto element = appendSimpleXMLNode(root, md.first, md.second.contents);
        for (const auto& attr : md.second.attributes)
            addAttribute(element, attr.first, attr.second);
    }

    if (!_content.empty()) {
        auto doc = root->getOwnerDocument();
        DOMElement* contentRootElement = doc->createElement(XUTF8Str("content").unicodeForm());
        root->appendChild(contentRootElement);
        for (const auto& content : _content) {
            DOMElement* contentElement = doc->createElement(XUTF8Str(content.first.c_str()).unicodeForm());
            contentRootElement->appendChild(contentElement);
            content.second.appendToElement(contentElement);
        }
    }
}


void Metadata::parseVersion1(const DOMNode* startNode)
{
    auto children = startNode->getChildNodes();

    for (XMLSize_t i = 0; i < children->getLength(); ++i) {
        auto child = children->item(i);
        auto element = dynamic_cast<const DOMElement*>(child);
        if (!element)
            continue;

        auto tag = element->getNodeName();
        auto tagString = StrXUTF8(tag).str;

        if (tagString == "name")
            _name = StrXUTF8(element->getTextContent()).str;
        else if (tagString == "version")
            _version = Meta::Version(StrXUTF8(element->getTextContent()).str);
        else if (tagString == "description")
            _description = StrXUTF8(element->getTextContent()).str;
        else if (tagString == "maintainer")
            _maintainer.emplace_back(element);
        else if (tagString == "license")
            _license.emplace_back(element);
        else if (tagString == "freecadmin")
            _freecadmin = Meta::Version(StrXUTF8(element->getTextContent()).str);
        else if (tagString == "freecadmax")
            _freecadmax = Meta::Version(StrXUTF8(element->getTextContent()).str);
        else if (tagString == "url")
            _url.emplace_back(element);
        else if (tagString == "author")
            _author.emplace_back(element);
        else if (tagString == "depend")
            _depend.emplace_back(element);
        else if (tagString == "conflict")
            _conflict.emplace_back(element);
        else if (tagString == "replace")
            _replace.emplace_back(element);
        else if (tagString == "tag")
            _tag.emplace_back(StrXUTF8(element->getTextContent()).str);
        else if (tagString == "file")
            _file.emplace_back(StrXUTF8(element->getTextContent()).str);
        else if (tagString == "classname")
            _classname = StrXUTF8(element->getTextContent()).str;
        else if (tagString == "subdirectory")
            _subdirectory = StrXUTF8(element->getTextContent()).str;
        else if (tagString == "icon")
            _icon = fs::path(StrXUTF8(element->getTextContent()).str);
        else if (tagString == "content")
            parseContentNodeVersion1(element); // Recursive call
        else {
            // If none of this node's children have children of their own, it is a simple element and we
            // can handle it as a GenericMetadata object
            auto children = element->getChildNodes();
            bool hasGrandchildren = false;
            for (XMLSize_t i = 0; i < children->getLength() && !hasGrandchildren; ++i)
                if (children->item(i)->getChildNodes()->getLength() > 0)
                    hasGrandchildren = true;
            if (!hasGrandchildren)
                _genericMetadata.insert(std::make_pair(tagString, Meta::GenericMetadata(element)));
        }
    }
}

void Metadata::parseContentNodeVersion1(const DOMElement* contentNode)
{
    auto children = contentNode->getChildNodes();
    for (XMLSize_t i = 0; i < children->getLength(); ++i) {
        auto child = dynamic_cast<const DOMElement*>(children->item(i));
        if (child) {
            auto tag = StrXUTF8(child->getTagName()).str;
            _content.insert(std::make_pair(tag, Metadata(child, 1)));
        }
    }
}

Meta::Contact::Contact(const std::string& name, const std::string& email) :
    name(name),
    email(email)
{
    // This has to be provided manually since we have another constructor
}

Meta::Contact::Contact(const XERCES_CPP_NAMESPACE::DOMElement* e)
{
    auto emailAttribute = e->getAttribute(XUTF8Str("email").unicodeForm());
    name = StrXUTF8(e->getTextContent()).str;
    email = StrXUTF8(emailAttribute).str;
}

Meta::License::License(const std::string& name, fs::path file) :
    name(name),
    file(file)
{
    // This has to be provided manually since we have another constructor
}

Meta::License::License(const XERCES_CPP_NAMESPACE::DOMElement* e)
{
    auto fileAttribute = e->getAttribute(XUTF8Str("file").unicodeForm());
    if (XMLString::stringLen(fileAttribute) > 0) {
        file = fs::path(StrXUTF8(fileAttribute).str);
    }
    name = StrXUTF8(e->getTextContent()).str;
}

Meta::Url::Url(const std::string& location, UrlType type) :
    location(location),
    type(type)
{
    // This has to be provided manually since we have another constructor
}

Meta::Url::Url(const XERCES_CPP_NAMESPACE::DOMElement* e)
{
    auto typeAttribute = StrXUTF8(e->getAttribute(XUTF8Str("type").unicodeForm())).str;
    if (typeAttribute.empty() || typeAttribute == "website")
        type = UrlType::website;
    else if (typeAttribute == "bugtracker")
        type = UrlType::bugtracker;
    else if (typeAttribute == "repository")
        type = UrlType::repository;
    else if (typeAttribute == "readme")
        type = UrlType::readme;
    else if (typeAttribute == "documentation")
        type = UrlType::documentation;
    else
        type = UrlType::website;

    if (type == UrlType::repository) 
        branch = StrXUTF8(e->getAttribute(XUTF8Str("branch").unicodeForm())).str;

    location = StrXUTF8(e->getTextContent()).str;
}

Meta::Dependency::Dependency(const XERCES_CPP_NAMESPACE::DOMElement* e)
{
    version_lt = StrXUTF8(e->getAttribute(XUTF8Str("version_lt").unicodeForm())).str;
    version_lte = StrXUTF8(e->getAttribute(XUTF8Str("version_lte").unicodeForm())).str;
    version_eq = StrXUTF8(e->getAttribute(XUTF8Str("version_eq").unicodeForm())).str;
    version_gte = StrXUTF8(e->getAttribute(XUTF8Str("version_gte").unicodeForm())).str;
    version_gt = StrXUTF8(e->getAttribute(XUTF8Str("version_gt").unicodeForm())).str;
    condition = StrXUTF8(e->getAttribute(XUTF8Str("condition").unicodeForm())).str;

    package = StrXUTF8(e->getTextContent()).str;
}

Meta::Version::Version() :
    major(0),
    minor(0),
    patch(0)
{
}

Meta::Version::Version(int major, int minor, int patch, const std::string& suffix) :
    major(major),
    minor(minor),
    patch(patch),
    suffix(suffix)
{

}

Meta::Version::Version(const std::string& versionString) :
    minor(0),
    patch(0)
{
    std::istringstream stream(versionString);
    char separator;
    stream >> major;
    if (stream) stream >> separator;
    if (stream) stream >> minor;
    if (stream) stream >> separator;
    if (stream) stream >> patch;
    if (stream) stream >> suffix;
}

std::string Meta::Version::str() const
{
    std::ostringstream stream;
    stream << major << "." << minor << "." << patch << suffix;
    return stream.str();
}

bool Meta::Version::operator<(const Version& rhs) const
{
    return std::tie(major, minor, patch, suffix) < std::tie(rhs.major, rhs.minor, rhs.patch, rhs.suffix);
}

bool Meta::Version::operator>(const Version& rhs) const
{
    return std::tie(major, minor, patch, suffix) > std::tie(rhs.major, rhs.minor, rhs.patch, rhs.suffix);
}

bool Meta::Version::operator<=(const Version& rhs) const
{
    return std::tie(major, minor, patch, suffix) <= std::tie(rhs.major, rhs.minor, rhs.patch, rhs.suffix);
}

bool Meta::Version::operator>=(const Version& rhs) const
{
    return std::tie(major, minor, patch, suffix) >= std::tie(rhs.major, rhs.minor, rhs.patch, rhs.suffix);
}

bool Meta::Version::operator==(const Version& rhs) const
{
    return std::tie(major, minor, patch, suffix) == std::tie(rhs.major, rhs.minor, rhs.patch, rhs.suffix);
}

bool Meta::Version::operator!=(const Version& rhs) const
{
    return std::tie(major, minor, patch, suffix) != std::tie(rhs.major, rhs.minor, rhs.patch, rhs.suffix);
}

Meta::GenericMetadata::GenericMetadata(const XERCES_CPP_NAMESPACE::DOMElement* e)
{
    contents = StrXUTF8(e->getTextContent()).str;
    for (XMLSize_t i = 0; i < e->getAttributes()->getLength(); ++i) {
        auto a = e->getAttributes()->item(i);
        attributes.insert(std::make_pair(StrXUTF8(a->getNodeName()).str,
            StrXUTF8(a->getTextContent()).str));
    }
}

App::Meta::GenericMetadata::GenericMetadata(const std::string& contents) :
    contents(contents)
{
}
