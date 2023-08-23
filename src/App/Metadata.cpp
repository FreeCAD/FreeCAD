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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <boost/core/ignore_unused.hpp>
# include <memory>
# include <sstream>
#endif

#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/sax/HandlerBase.hpp>

#include "App/Application.h"
#include "App/Expression.h"
#include "Base/XMLTools.h"

#include "Metadata.h"


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

namespace MetadataInternal
{
class XMLErrorHandler: public HandlerBase
{
    void warning(const SAXParseException& toCatch) override
    {
        // Don't deal with warnings at all
        boost::ignore_unused(toCatch);
    }

    void error(const SAXParseException& toCatch) override
    {
        std::stringstream message;
        message << "Error at file \"" << StrX(toCatch.getSystemId()) << "\", line "
                << toCatch.getLineNumber() << ", column " << toCatch.getColumnNumber()
                << "\n   Message: " << StrX(toCatch.getMessage()) << std::endl;
        throw Base::XMLBaseException(message.str());
    }

    void fatalError(const SAXParseException& toCatch) override
    {
        std::stringstream message;
        message << "Fatal error at file \"" << StrX(toCatch.getSystemId()) << "\", line "
                << toCatch.getLineNumber() << ", column " << toCatch.getColumnNumber()
                << "\n   Message: " << StrX(toCatch.getMessage()) << std::endl;
        throw Base::XMLBaseException(message.str());
    }
};
}// namespace MetadataInternal

Metadata::Metadata(const fs::path& metadataFile)
    : _dom(nullptr)
{
#if defined(FC_OS_WIN32)
    auto source =
        LocalFileInputSource(reinterpret_cast<const XMLCh*>(metadataFile.wstring().c_str()));
#else
    auto source = LocalFileInputSource(XUTF8Str(metadataFile.string().c_str()).unicodeForm());
#endif
    loadFromInputSource(source);
}

Metadata::Metadata()
    : _dom(nullptr)
{}

Metadata::Metadata(const DOMNode* domNode, int format)
    : _dom(nullptr)
{
    auto element = dynamic_cast<const DOMElement*>(domNode);
    if (element) {
        switch (format) {
            case 1:
                parseVersion1(element);
                break;
            default:
                throw Base::XMLBaseException(
                    "package.xml format version is not supported by this version of FreeCAD");
        }
    }
}

App::Metadata::Metadata(const std::string& rawData)
    : _dom(nullptr)
{
    MemBufInputSource buffer(
        reinterpret_cast<const XMLByte*>(rawData.c_str()),
        rawData.size(),
        "raw data (in memory)");
    loadFromInputSource(buffer);
}

void Metadata::loadFromInputSource(const InputSource& source)
{
    // Any exception thrown by the XML code propagates out and prevents object creation
    XMLPlatformUtils::Initialize();

    _parser = std::make_shared<XercesDOMParser>();
    _parser->setValidationScheme(XercesDOMParser::Val_Never);
    _parser->setDoNamespaces(true);

    auto errHandler = std::make_unique<MetadataInternal::XMLErrorHandler>();
    _parser->setErrorHandler(errHandler.get());

    _parser->parse(source);

    auto doc = _parser->getDocument();
    _dom = doc->getDocumentElement();

    auto rootTagName = StrXUTF8(_dom->getTagName()).str;
    if (rootTagName != "package") {
        throw Base::XMLBaseException(
            "Malformed package.xml document: Root <package> group not found");
    }
    auto formatVersion = XMLString::parseInt(_dom->getAttribute(XUTF8Str("format").unicodeForm()));
    switch (formatVersion) {
        case 1:
            parseVersion1(_dom);
            break;
        default:
            throw Base::XMLBaseException(
                "package.xml format version is not supported by this version of FreeCAD");
    }
}

Metadata::~Metadata() = default;

std::string Metadata::name() const
{
    return _name;
}

std::string Metadata::type() const
{
    return _type;
}

Meta::Version Metadata::version() const
{
    return _version;
}

std::string App::Metadata::date() const
{
    return _date;
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

boost::filesystem::path Metadata::subdirectory() const
{
    return _subdirectory;
}

std::vector<fs::path> Metadata::file() const
{
    return _file;
}

Meta::Version Metadata::freecadmin() const
{
    return _freecadmin;
}

Meta::Version Metadata::freecadmax() const
{
    return _freecadmax;
}

Meta::Version Metadata::pythonmin() const
{
    return _pythonmin;
}

std::multimap<std::string, Metadata> Metadata::content() const
{
    return _content;
}

std::vector<Meta::GenericMetadata> Metadata::operator[](const std::string& tag) const
{
    std::vector<Meta::GenericMetadata> returnValue;
    auto range = _genericMetadata.equal_range(tag);
    for (auto item = range.first; item != range.second; ++item) {
        returnValue.push_back(item->second);
    }
    return returnValue;
}

XERCES_CPP_NAMESPACE::DOMElement* Metadata::dom() const
{
    return _dom;
}

void Metadata::setName(const std::string& name)
{
    std::string invalidCharacters = "/\\?%*:|\"<>";// Should cover all OSes
    if (_name.find_first_of(invalidCharacters) != std::string::npos) {
        throw Base::RuntimeError("Name cannot contain any of: " + invalidCharacters);
    }
    _name = name;
}

void Metadata::setType(const std::string& type)
{
    _type = type;
}

void Metadata::setVersion(const Meta::Version& version)
{
    _version = version;
}

void App::Metadata::setDate(const std::string& date)
{
    _date = date;
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

void Metadata::setSubdirectory(const boost::filesystem::path& path)
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

void Metadata::setFreeCADMin(const Meta::Version& version)
{
    _freecadmin = version;
}

void Metadata::setPythonMin(const Meta::Version& version)
{
    _pythonmin = version;
}

void Metadata::setFreeCADMax(const Meta::Version& version)
{
    _freecadmax = version;
}

void Metadata::addGenericMetadata(const std::string& tag,
                                  const Meta::GenericMetadata& genericMetadata)
{
    _genericMetadata.insert(std::make_pair(tag, genericMetadata));
}

void Metadata::removeContentItem(const std::string& tag, const std::string& itemName)
{
    auto tagRange = _content.equal_range(tag);
    auto foundItem =
        std::find_if(tagRange.first, tagRange.second, [&itemName](const auto& check) -> bool {
            return itemName == check.second.name();
        });
    if (foundItem != tagRange.second) {
        _content.erase(foundItem);
    }
}

void Metadata::removeMaintainer(const Meta::Contact& maintainer)
{
    auto new_end = std::remove(_maintainer.begin(), _maintainer.end(), maintainer);
    _maintainer.erase(new_end, _maintainer.end());
}

void Metadata::removeLicense(const Meta::License& license)
{
    auto new_end = std::remove(_license.begin(), _license.end(), license);
    _license.erase(new_end, _license.end());
}

void Metadata::removeUrl(const Meta::Url& url)
{
    auto new_end = std::remove(_url.begin(), _url.end(), url);
    _url.erase(new_end, _url.end());
}

void Metadata::removeAuthor(const Meta::Contact& author)
{
    auto new_end = std::remove(_author.begin(), _author.end(), author);
    _author.erase(new_end, _author.end());
}

void Metadata::removeDepend(const Meta::Dependency& dep)
{
    bool found = false;
    for (const auto& check : _depend) {
        if (dep == check) {
            found = true;
        }
    }
    if (!found) {
        throw Base::RuntimeError("No match found for dependency to remove");
    }
    auto new_end = std::remove(_depend.begin(), _depend.end(), dep);
    _depend.erase(new_end, _depend.end());
}

void Metadata::removeConflict(const Meta::Dependency& dep)
{
    auto new_end = std::remove(_conflict.begin(), _conflict.end(), dep);
    _conflict.erase(new_end, _conflict.end());
}

void Metadata::removeReplace(const Meta::Dependency& dep)
{
    auto new_end = std::remove(_replace.begin(), _replace.end(), dep);
    _replace.erase(new_end, _replace.end());
}

void Metadata::removeTag(const std::string& tag)
{
    auto new_end = std::remove(_tag.begin(), _tag.end(), tag);
    _tag.erase(new_end, _tag.end());
}

void Metadata::removeFile(const boost::filesystem::path& path)
{
    auto new_end = std::remove(_file.begin(), _file.end(), path);
    _file.erase(new_end, _file.end());
}


void Metadata::clearContent()
{
    _content.clear();
}

void Metadata::clearMaintainer()
{
    _maintainer.clear();
}

void Metadata::clearLicense()
{
    _license.clear();
}

void Metadata::clearUrl()
{
    _url.clear();
}

void Metadata::clearAuthor()
{
    _author.clear();
}

void Metadata::clearDepend()
{
    _depend.clear();
}

void Metadata::clearConflict()
{
    _conflict.clear();
}

void Metadata::clearReplace()
{
    _replace.clear();
}

void Metadata::clearTag()
{
    _tag.clear();
}

void Metadata::clearFile()
{
    _file.clear();
}


DOMElement* appendSimpleXMLNode(DOMElement* baseNode, const std::string& nodeName,
                                const std::string& nodeContents)
{
    // For convenience (and brevity of final output) don't create nodes that don't have contents
    if (nodeContents.empty()) {
        return nullptr;
    }
    auto doc = baseNode->getOwnerDocument();
    DOMElement* namedElement = doc->createElement(XUTF8Str(nodeName.c_str()).unicodeForm());
    baseNode->appendChild(namedElement);
    DOMText* namedNode = doc->createTextNode(XUTF8Str(nodeContents.c_str()).unicodeForm());
    namedElement->appendChild(namedNode);
    return namedElement;
}

void addAttribute(DOMElement* node, const std::string& key, const std::string& value)
{
    if (value.empty()) {
        return;
    }
    node->setAttribute(XUTF8Str(key.c_str()).unicodeForm(), XUTF8Str(value.c_str()).unicodeForm());
}

void addAttribute(DOMElement* node, const std::string& key, bool value)
{
    if (value) {
        node->setAttribute(XUTF8Str(key.c_str()).unicodeForm(), XUTF8Str("True").unicodeForm());
    }
    else {
        node->setAttribute(XUTF8Str(key.c_str()).unicodeForm(), XUTF8Str("False").unicodeForm());
    }
}

void addAttribute(DOMElement* node, const std::string& key, Meta::DependencyType value)
{
    // Someday we should be able to change this to use reflection, but it's not yet
    // available (using C++17)
    std::string stringified("automatic");
    switch (value) {
        case Meta::DependencyType::automatic:
            stringified = "automatic";
            break;
        case Meta::DependencyType::internal:
            stringified = "internal";
            break;
        case Meta::DependencyType::addon:
            stringified = "addon";
            break;
        case Meta::DependencyType::python:
            stringified = "python";
            break;
    }
    node->setAttribute(XUTF8Str(key.c_str()).unicodeForm(),
                       XUTF8Str(stringified.c_str()).unicodeForm());
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
        addAttribute(element, "optional", depend.optional);
        addAttribute(element, "type", depend.dependencyType);
    }
}

void Metadata::write(const fs::path& file) const
{
    DOMImplementation* impl =
        DOMImplementationRegistry::getDOMImplementation(XUTF8Str("Core LS").unicodeForm());

    DOMDocument* doc = impl->createDocument(nullptr, XUTF8Str("package").unicodeForm(), nullptr);
    DOMElement* root = doc->getDocumentElement();
    root->setAttribute(XUTF8Str("format").unicodeForm(), XUTF8Str("1").unicodeForm());
    root->setAttribute(XUTF8Str("xmlns").unicodeForm(),
                       XUTF8Str("https://wiki.freecad.org/Package_Metadata").unicodeForm());

    appendToElement(root);

    DOMLSSerializer* theSerializer = ((DOMImplementationLS*)impl)->createLSSerializer();
    DOMConfiguration* config = theSerializer->getDomConfig();
    if (config->canSetParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true)) {
        config->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true);
    }

    // set feature if the serializer supports the feature/mode
    if (config->canSetParameter(XMLUni::fgDOMWRTSplitCdataSections, true)) {
        config->setParameter(XMLUni::fgDOMWRTSplitCdataSections, true);
    }

    if (config->canSetParameter(XMLUni::fgDOMWRTDiscardDefaultContent, true)) {
        config->setParameter(XMLUni::fgDOMWRTDiscardDefaultContent, true);
    }

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
    if (dep.package != _name) {
        return false;
    }
    // The "condition" attribute allows an expression to enable or disable this dependency check: it must contain a valid
    // FreeCAD Expression. If it evaluates to false, this dependency is bypassed (e.g. this function returns false).
    if (!dep.condition.empty()) {
        auto injectedString = dep.condition;
        std::map<std::string, std::string> replacements;
        std::map<std::string, std::string>& config = App::Application::Config();
        replacements.insert(std::make_pair("$BuildVersionMajor", config["BuildVersionMajor"]));
        replacements.insert(std::make_pair("$BuildVersionMinor", config["BuildVersionMinor"]));
        replacements.insert(std::make_pair("$BuildVersionMinor", config["BuildVersionPoint"]));
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
        if (!boost::any_cast<bool>(result->getValueAsAny())) {
            return false;
        }
    }

    if (!dep.version_eq.empty()) {
        return _version == Meta::Version(dep.version_eq);
    }
    // Any of the others might be specified in pairs, so only return the "false" case

    if (!dep.version_lt.empty()) {
        if (!(_version < Meta::Version(dep.version_lt))) {
            return false;
        }
    }
    if (!dep.version_lte.empty()) {
        if (!(_version <= Meta::Version(dep.version_lt))) {
            return false;
        }
    }
    if (!dep.version_gt.empty()) {
        if (!(_version > Meta::Version(dep.version_lt))) {
            return false;
        }
    }
    if (!dep.version_gte.empty()) {
        if (!(_version >= Meta::Version(dep.version_lt))) {
            return false;
        }
    }
    return true;
}

bool Metadata::supportsCurrentFreeCAD() const
{
    static auto fcVersion = Meta::Version();
    if (fcVersion == Meta::Version()) {
        std::map<std::string, std::string>& config = App::Application::Config();
        std::stringstream ss;
        ss << config["BuildVersionMajor"] << "." << config["BuildVersionMinor"] << "."
           << config["BuildVersionPoint"] << "."
           << (config["BuildRevision"].empty() ? "0" : config["BuildRevision"]);
        fcVersion = Meta::Version(ss.str());
    }

    if (_freecadmin != Meta::Version() && _freecadmin > fcVersion) {
        return false;
    }
    if (_freecadmax != Meta::Version() && _freecadmax < fcVersion) {
        return false;
    }
    return true;
}

void Metadata::appendToElement(DOMElement* root) const
{
    appendSimpleXMLNode(root, "name", _name);
    appendSimpleXMLNode(root, "type", _type);
    appendSimpleXMLNode(root, "description", _description);
    if (_version != Meta::Version()) {
        // Only append version if it's not 0.0.0
        appendSimpleXMLNode(root, "version", _version.str());
    }

    if (!_date.empty()) {
        appendSimpleXMLNode(root, "date", _date);
    }

    for (const auto& maintainer : _maintainer) {
        auto element = appendSimpleXMLNode(root, "maintainer", maintainer.name);
        if (element) {
            addAttribute(element, "email", maintainer.email);
        }
    }

    for (const auto& license : _license) {
        auto element = appendSimpleXMLNode(root, "license", license.name);
        if (element) {
            addAttribute(element, "file", license.file.string());
        }
    }

    if (_freecadmin != Meta::Version()) {
        appendSimpleXMLNode(root, "freecadmin", _freecadmin.str());
    }

    if (_freecadmax != Meta::Version()) {
        appendSimpleXMLNode(root, "freecadmax", _freecadmax.str());
    }

    if (_pythonmin != Meta::Version()) {
        appendSimpleXMLNode(root, "pythonmin", _pythonmin.str());
    }

    for (const auto& url : _url) {
        auto element = appendSimpleXMLNode(root, "url", url.location);
        if (element) {
            std::string typeAsString("website");
            switch (url.type) {
                case Meta::UrlType::website:
                    typeAsString = "website";
                    break;
                case Meta::UrlType::repository:
                    typeAsString = "repository";
                    break;
                case Meta::UrlType::bugtracker:
                    typeAsString = "bugtracker";
                    break;
                case Meta::UrlType::readme:
                    typeAsString = "readme";
                    break;
                case Meta::UrlType::documentation:
                    typeAsString = "documentation";
                    break;
                case Meta::UrlType::discussion:
                    typeAsString = "discussion";
                    break;
            }
            addAttribute(element, "type", typeAsString);
            if (url.type == Meta::UrlType::repository) {
                addAttribute(element, "branch", url.branch);
            }
        }
    }

    for (const auto& author : _author) {
        auto element = appendSimpleXMLNode(root, "author", author.name);
        if (element) {
            addAttribute(element, "email", author.email);
        }
    }

    for (const auto& depend : _depend) {
        addDependencyNode(root, "depend", depend);
    }

    for (const auto& conflict : _conflict) {
        addDependencyNode(root, "conflict", conflict);
    }

    for (const auto& replace : _replace) {
        addDependencyNode(root, "replace", replace);
    }

    for (const auto& tag : _tag) {
        appendSimpleXMLNode(root, "tag", tag);
    }

    appendSimpleXMLNode(root, "icon", _icon.string());

    appendSimpleXMLNode(root, "classname", _classname);

    appendSimpleXMLNode(root, "subdirectory", _subdirectory.string());

    for (const auto& file : _file) {
        appendSimpleXMLNode(root, "file", file.string());
    }

    for (const auto& md : _genericMetadata) {
        auto element = appendSimpleXMLNode(root, md.first, md.second.contents);
        for (const auto& attr : md.second.attributes) {
            addAttribute(element, attr.first, attr.second);
        }
    }

    if (!_content.empty()) {
        auto doc = root->getOwnerDocument();
        DOMElement* contentRootElement = doc->createElement(XUTF8Str("content").unicodeForm());
        root->appendChild(contentRootElement);
        for (const auto& content : _content) {
            DOMElement* contentElement =
                doc->createElement(XUTF8Str(content.first.c_str()).unicodeForm());
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
        if (!element) {
            continue;
        }

        auto tag = element->getNodeName();
        auto tagString = StrXUTF8(tag).str;

        if (tagString == "name") {
            _name = StrXUTF8(element->getTextContent()).str;
        }
        else if (tagString == "type") {
            _type = StrXUTF8(element->getTextContent()).str;
        }
        else if (tagString == "version") {
            _version = Meta::Version(StrXUTF8(element->getTextContent()).str);
        }
        else if (tagString == "date") {
            _date = StrXUTF8(element->getTextContent()).str;
        }
        else if (tagString == "description") {
            _description = StrXUTF8(element->getTextContent()).str;
        }
        else if (tagString == "maintainer") {
            _maintainer.emplace_back(element);
        }
        else if (tagString == "license") {
            _license.emplace_back(element);
        }
        else if (tagString == "freecadmin") {
            _freecadmin = Meta::Version(StrXUTF8(element->getTextContent()).str);
        }
        else if (tagString == "freecadmax") {
            _freecadmax = Meta::Version(StrXUTF8(element->getTextContent()).str);
        }
        else if (tagString == "pythonmin") {
            _pythonmin = Meta::Version(StrXUTF8(element->getTextContent()).str);
        }
        else if (tagString == "url") {
            _url.emplace_back(element);
        }
        else if (tagString == "author") {
            _author.emplace_back(element);
        }
        else if (tagString == "depend") {
            _depend.emplace_back(element);
        }
        else if (tagString == "conflict") {
            _conflict.emplace_back(element);
        }
        else if (tagString == "replace") {
            _replace.emplace_back(element);
        }
        else if (tagString == "tag") {
            _tag.emplace_back(StrXUTF8(element->getTextContent()).str);
        }
        else if (tagString == "file") {
            _file.emplace_back(StrXUTF8(element->getTextContent()).str);
        }
        else if (tagString == "classname") {
            _classname = StrXUTF8(element->getTextContent()).str;
        }
        else if (tagString == "subdirectory") {
            _subdirectory = StrXUTF8(element->getTextContent()).str;
        }
        else if (tagString == "icon") {
            _icon = fs::path(StrXUTF8(element->getTextContent()).str);
        }
        else if (tagString == "content") {
            parseContentNodeVersion1(element);// Recursive call
        }
        else {
            // If none of this node's nodeChildren have nodeChildren of their own, it is a simple element and we
            // can handle it as a GenericMetadata object
            auto nodeChildren = element->getChildNodes();
            bool hasGrandchildren = false;
            for (XMLSize_t j = 0; j < nodeChildren->getLength() && !hasGrandchildren; ++j) {
                if (nodeChildren->item(j)->getChildNodes()->getLength() > 0) {
                    hasGrandchildren = true;
                }
            }
            if (!hasGrandchildren) {
                _genericMetadata.insert(std::make_pair(tagString, Meta::GenericMetadata(element)));
            }
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

Meta::Contact::Contact(std::string name, std::string email)
    : name(std::move(name)),
      email(std::move(email))
{
    // This has to be provided manually since we have another constructor
}

Meta::Contact::Contact(const XERCES_CPP_NAMESPACE::DOMElement* elem)
{
    if (!elem){
        return;
    }
    auto emailAttribute = elem->getAttribute(XUTF8Str("email").unicodeForm());
    name = StrXUTF8(elem->getTextContent()).str;
    email = StrXUTF8(emailAttribute).str;
}

bool App::Meta::Contact::operator==(const Contact& rhs) const
{
    return name == rhs.name && email == rhs.email;
}

Meta::License::License(std::string name, fs::path file)
    : name(std::move(name)),
      file(std::move(file))
{
    // This has to be provided manually since we have another constructor
}

Meta::License::License(const XERCES_CPP_NAMESPACE::DOMElement* elem)
{
    if (!elem){
        return;
    }
    auto fileAttribute = elem->getAttribute(XUTF8Str("file").unicodeForm());
    if (XMLString::stringLen(fileAttribute) > 0) {
        file = fs::path(StrXUTF8(fileAttribute).str);
    }
    name = StrXUTF8(elem->getTextContent()).str;
}

bool App::Meta::License::operator==(const License& rhs) const
{
    return name == rhs.name && file == rhs.file;
}

App::Meta::Url::Url()
    : location(""),
      type(App::Meta::UrlType::website)
{}

Meta::Url::Url(std::string location, UrlType type)
    : location(std::move(location)),
      type(type)
{
    // This has to be provided manually since we have another constructor
}

Meta::Url::Url(const XERCES_CPP_NAMESPACE::DOMElement* elem)
{
    if (!elem) {
        return;
    }
    auto typeAttribute = StrXUTF8(elem->getAttribute(XUTF8Str("type").unicodeForm())).str;
    if (typeAttribute.empty() || typeAttribute == "website") {
        type = UrlType::website;
    }
    else if (typeAttribute == "bugtracker") {
        type = UrlType::bugtracker;
    }
    else if (typeAttribute == "repository") {
        type = UrlType::repository;
    }
    else if (typeAttribute == "readme") {
        type = UrlType::readme;
    }
    else if (typeAttribute == "documentation") {
        type = UrlType::documentation;
    }
    else if (typeAttribute == "discussion") {
        type = UrlType::discussion;
    }
    else {
        type = UrlType::website;
    }

    if (type == UrlType::repository) {
        branch = StrXUTF8(elem->getAttribute(XUTF8Str("branch").unicodeForm())).str;
    }
    location = StrXUTF8(elem->getTextContent()).str;
}

bool App::Meta::Url::operator==(const Url& rhs) const
{
    if (type == UrlType::repository && branch != rhs.branch) {
        return false;
    }
    return type == rhs.type && location == rhs.location;
}

App::Meta::Dependency::Dependency()
    : optional(false),
      dependencyType(App::Meta::DependencyType::automatic)
{}

App::Meta::Dependency::Dependency(std::string pkg)
    : package(std::move(pkg)),
      optional(false),
      dependencyType(App::Meta::DependencyType::automatic)
{}

Meta::Dependency::Dependency(const XERCES_CPP_NAMESPACE::DOMElement* elem)
{
    version_lt = StrXUTF8(elem->getAttribute(XUTF8Str("version_lt").unicodeForm())).str;
    version_lte = StrXUTF8(elem->getAttribute(XUTF8Str("version_lte").unicodeForm())).str;
    version_eq = StrXUTF8(elem->getAttribute(XUTF8Str("version_eq").unicodeForm())).str;
    version_gte = StrXUTF8(elem->getAttribute(XUTF8Str("version_gte").unicodeForm())).str;
    version_gt = StrXUTF8(elem->getAttribute(XUTF8Str("version_gt").unicodeForm())).str;
    condition = StrXUTF8(elem->getAttribute(XUTF8Str("condition").unicodeForm())).str;
    std::string opt_string = StrXUTF8(elem->getAttribute(XUTF8Str("optional").unicodeForm())).str;
    if (opt_string == "true"
        || opt_string == "True") {// Support Python capitalization in this one case...
        optional = true;
    }
    else {
        optional = false;
    }
    std::string type_string = StrXUTF8(elem->getAttribute(XUTF8Str("type").unicodeForm())).str;
    if (type_string == "automatic" || type_string.empty()) {
        dependencyType = Meta::DependencyType::automatic;
    }
    else if (type_string == "addon") {
        dependencyType = Meta::DependencyType::addon;
    }
    else if (type_string == "internal") {
        dependencyType = Meta::DependencyType::internal;
    }
    else if (type_string == "python") {
        dependencyType = Meta::DependencyType::python;
    }
    else {
        auto message = std::string("Invalid dependency type \"") + type_string + "\"";
        throw Base::XMLBaseException(message);
    }

    package = StrXUTF8(elem->getTextContent()).str;
}

bool App::Meta::Dependency::operator==(const Dependency& rhs) const
{
    return package == rhs.package && version_lt == rhs.version_lt && version_lte == rhs.version_lte
        && version_eq == rhs.version_eq && version_gte == rhs.version_gte
        && version_gt == rhs.version_gt && condition == rhs.condition && optional == rhs.optional
        && dependencyType == rhs.dependencyType;
}

Meta::Version::Version() = default;

Meta::Version::Version(int major, int minor, int patch, std::string suffix)
    : major(major),
      minor(minor),
      patch(patch),
      suffix(std::move(suffix))
{}

Meta::Version::Version(const std::string& versionString)
{
    std::istringstream stream(versionString);
    char separator {'.'};
    stream >> major;
    if (stream) {
        stream >> separator;
    }
    if (stream) {
        stream >> minor;
    }
    if (stream) {
        stream >> separator;
    }
    if (stream) {
        stream >> patch;
    }
    if (stream) {
        stream >> suffix;
    }
}

std::string Meta::Version::str() const
{
    if (*this == Meta::Version()) {
        return "";
    }
    std::ostringstream stream;
    stream << major << "." << minor << "." << patch << suffix;
    return stream.str();
}

bool Meta::Version::operator<(const Version& rhs) const
{
    return std::tie(major, minor, patch, suffix)
        < std::tie(rhs.major, rhs.minor, rhs.patch, rhs.suffix);
}

bool Meta::Version::operator>(const Version& rhs) const
{
    return std::tie(major, minor, patch, suffix)
        > std::tie(rhs.major, rhs.minor, rhs.patch, rhs.suffix);
}

bool Meta::Version::operator<=(const Version& rhs) const
{
    return std::tie(major, minor, patch, suffix)
        <= std::tie(rhs.major, rhs.minor, rhs.patch, rhs.suffix);
}

bool Meta::Version::operator>=(const Version& rhs) const
{
    return std::tie(major, minor, patch, suffix)
        >= std::tie(rhs.major, rhs.minor, rhs.patch, rhs.suffix);
}

bool Meta::Version::operator==(const Version& rhs) const
{
    return std::tie(major, minor, patch, suffix)
        == std::tie(rhs.major, rhs.minor, rhs.patch, rhs.suffix);
}

bool Meta::Version::operator!=(const Version& rhs) const
{
    return std::tie(major, minor, patch, suffix)
        != std::tie(rhs.major, rhs.minor, rhs.patch, rhs.suffix);
}

Meta::GenericMetadata::GenericMetadata(const XERCES_CPP_NAMESPACE::DOMElement* elem)
{
    contents = StrXUTF8(elem->getTextContent()).str;
    for (XMLSize_t i = 0; i < elem->getAttributes()->getLength(); ++i) {
        auto attr = elem->getAttributes()->item(i);
        attributes.insert(
            std::make_pair(StrXUTF8(attr->getNodeName()).str, StrXUTF8(attr->getTextContent()).str));
    }
}

App::Meta::GenericMetadata::GenericMetadata(std::string contents)
    : contents(std::move(contents))
{}
