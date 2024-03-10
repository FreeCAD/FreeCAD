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


#include "PreCompiled.h"

#include <cassert>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XercesVersion.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/framework/StdOutFormatTarget.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/XMLUni.hpp>
#include <xercesc/util/XMLUniDefs.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <sstream>

#include <zipios++/zipios-config.h>
#include <zipios++/zipfile.h>
#include <zipios++/zipinputstream.h>
#include <zipios++/zipoutputstream.h>
#include <zipios++/meta-iostreams.h>

#include "ProjectFile.h"
#include "DocumentObject.h"
#include <Base/FileInfo.h>
#include <Base/InputSource.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Base/Stream.h>
#include <Base/XMLTools.h>

XERCES_CPP_NAMESPACE_USE
using namespace App;

namespace {
class DocumentMetadata
{
public:
    explicit DocumentMetadata(XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* xmlDocument)
        : xmlDocument{xmlDocument}
    {}

    ProjectFile::Metadata getMetadata() const
    {
        return metadata;
    }

    void readXML()
    {
        readProgramVersion();

        std::map<std::string, std::string> propMap = initMap();

        DOMNodeList* nodes = xmlDocument->getElementsByTagName(XStr("Properties").unicodeForm());
        for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
            DOMNode* node = nodes->item(i);
            if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
                auto elem = static_cast<DOMElement*>(node);  // NOLINT
                DOMNodeList* propList = elem->getElementsByTagName(XStr("Property").unicodeForm());
                for (XMLSize_t j = 0; j < propList->getLength(); j++) {
                    DOMNode* propNode = propList->item(j);
                    readProperty(propNode, propMap);
                }
            }
            break;
        }

        setMetadata(propMap);
    }

private:
    void readProgramVersion()
    {
        if (DOMNodeList* nodes = xmlDocument->getElementsByTagName(XStr("Document").unicodeForm())) {
            for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
                DOMNode* node = nodes->item(i);
                if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
                    DOMNode* nameAttr = node->getAttributes()->getNamedItem(XStr("ProgramVersion").unicodeForm());
                    if (nameAttr) {
                        std::string value = StrX(nameAttr->getNodeValue()).c_str();
                        metadata.programVersion = value;
                        break;
                    }
                }
            }
        }
    }
    static std::map<std::string, std::string> initMap()
    {
        // clang-format off
        std::map<std::string, std::string> propMap = {{"Comment", ""},
                                                      {"Company", ""},
                                                      {"CreatedBy", ""},
                                                      {"CreationDate", ""},
                                                      {"Label", ""},
                                                      {"LastModifiedBy", ""},
                                                      {"LastModifiedDate", ""},
                                                      {"License", ""},
                                                      {"LicenseURL", ""},
                                                      {"Uid", ""}};
        return propMap;
        // clang-format on
    }

    void setMetadata(const std::map<std::string, std::string>& propMap)
    {
        metadata.comment = propMap.at("Comment");
        metadata.company = propMap.at("Company");
        metadata.createdBy = propMap.at("CreatedBy");
        metadata.creationDate = propMap.at("CreationDate");
        metadata.label = propMap.at("Label");
        metadata.lastModifiedBy = propMap.at("LastModifiedBy");
        metadata.lastModifiedDate = propMap.at("LastModifiedDate");
        metadata.license = propMap.at("License");
        metadata.licenseURL = propMap.at("LicenseURL");
        metadata.uuid = propMap.at("Uid");
    }

    static void readProperty(DOMNode* propNode, std::map<std::string, std::string>& propMap)
    {
        DOMNode* nameAttr = propNode->getAttributes()->getNamedItem(XStr("name").unicodeForm());
        if (nameAttr) {
            std::string name = StrX(nameAttr->getNodeValue()).c_str();
            auto it = propMap.find(name);
            if (it != propMap.end()) {
                it->second = readValue(propNode);
            }
        }
    }

    static std::string readValue(DOMNode* node)
    {
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
             if (DOMElement* child = static_cast<DOMElement*>(node)->getFirstElementChild()) {  // NOLINT
                 if (DOMNode* nameAttr = child->getAttributes()->getNamedItem(XStr("value").unicodeForm())) {
                     std::string value = StrX(nameAttr->getNodeValue()).c_str();
                     return value;
                 }
             }
        }

        return {};
    }

private:
    XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* xmlDocument;
    ProjectFile::Metadata metadata;
};
}

ProjectFile::ProjectFile()
    : xmlDocument(nullptr)
{}

ProjectFile::ProjectFile(std::string zipArchive)
    : stdFile(std::move(zipArchive))
    , xmlDocument(nullptr)
{}

ProjectFile::~ProjectFile()
{
    delete xmlDocument;
}

void ProjectFile::setProjectFile(const std::string& zipArchive)
{
    stdFile = zipArchive;
    delete xmlDocument;
    xmlDocument = nullptr;
}

bool ProjectFile::loadDocument()
{
    if (xmlDocument) {
        return true;  // already loaded
    }

    zipios::ZipFile project(stdFile);
    if (!project.isValid()) {
        return false;
    }
    std::unique_ptr<std::istream> str(project.getInputStream("Document.xml"));
    if (str) {
        std::unique_ptr<XercesDOMParser> parser(new XercesDOMParser);
        parser->setValidationScheme(XercesDOMParser::Val_Auto);
        parser->setDoNamespaces(false);
        parser->setDoSchema(false);
        parser->setValidationSchemaFullChecking(false);
        parser->setCreateEntityReferenceNodes(false);

        try {
            Base::StdInputSource inputSource(*str, stdFile.c_str());
            parser->parse(inputSource);
            xmlDocument = parser->adoptDocument();
            return true;
        }
        catch (const XMLException&) {
            return false;
        }
        catch (const DOMException&) {
            return false;
        }
    }

    return false;
}

ProjectFile::Metadata ProjectFile::getMetadata() const
{
    if (!xmlDocument) {
        return {};
    }

    DocumentMetadata reader(xmlDocument);
    reader.readXML();
    return reader.getMetadata();
}

std::list<ProjectFile::Object> ProjectFile::getObjects() const
{
    std::list<Object> names;
    if (!xmlDocument) {
        return names;
    }

    DOMNodeList* nodes = xmlDocument->getElementsByTagName(XStr("Objects").unicodeForm());
    for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
        DOMNode* node = nodes->item(i);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
            DOMNodeList* objectList =
                static_cast<DOMElement*>(node)->getElementsByTagName(XStr("Object").unicodeForm());  // NOLINT
            for (XMLSize_t j = 0; j < objectList->getLength(); j++) {
                DOMNode* objectNode = objectList->item(j);
                DOMNode* typeAttr =
                    objectNode->getAttributes()->getNamedItem(XStr("type").unicodeForm());
                DOMNode* nameAttr =
                    objectNode->getAttributes()->getNamedItem(XStr("name").unicodeForm());
                if (typeAttr && nameAttr) {
                    Object obj;
                    obj.name = StrX(nameAttr->getNodeValue()).c_str();
                    obj.type = Base::Type::fromName(StrX(typeAttr->getNodeValue()).c_str());
                    names.push_back(obj);
                }
            }
        }
    }

    return names;
}

std::list<std::string> ProjectFile::getObjectsOfType(const Base::Type& typeId) const
{
    std::list<std::string> names;
    if (!xmlDocument) {
        return names;
    }

    DOMNodeList* nodes = xmlDocument->getElementsByTagName(XStr("Objects").unicodeForm());
    for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
        DOMNode* node = nodes->item(i);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
            DOMNodeList* objectList =
                static_cast<DOMElement*>(node)->getElementsByTagName(XStr("Object").unicodeForm());  // NOLINT
            for (XMLSize_t j = 0; j < objectList->getLength(); j++) {
                DOMNode* objectNode = objectList->item(j);
                DOMNode* typeAttr =
                    objectNode->getAttributes()->getNamedItem(XStr("type").unicodeForm());
                DOMNode* nameAttr =
                    objectNode->getAttributes()->getNamedItem(XStr("name").unicodeForm());
                if (typeAttr && nameAttr) {
                    if (Base::Type::fromName(StrX(typeAttr->getNodeValue()).c_str()) == typeId) {
                        names.emplace_back(StrX(nameAttr->getNodeValue()).c_str());
                    }
                }
            }
        }
    }

    return names;
}

bool ProjectFile::restoreObject(const std::string& name,
                                App::PropertyContainer* obj,
                                bool verbose)
{
    Base::FileInfo fi(stdFile);
    Base::ifstream file(fi, std::ios::in | std::ios::binary);

    zipios::ZipInputStream zipstream(file);
    Base::XMLReader reader(stdFile.c_str(), zipstream);
    reader.setVerbose(verbose);

    if (!reader.isValid()) {
        return false;
    }

    // skip document properties
    reader.readElement("Properties");
    reader.readEndElement("Properties");

    // skip objects
    reader.readElement("Objects");
    reader.readEndElement("Objects");

    reader.readElement("ObjectData");
    long Cnt = reader.getAttributeAsInteger("Count");
    for (long i = 0; i < Cnt; i++) {
        reader.readElement("Object");
        std::string nameAttr = reader.getAttribute("name");

        if (nameAttr == name) {
            // obj->StatusBits.set(4);
            obj->Restore(reader);
            // obj->StatusBits.reset(4);
        }
        reader.readEndElement("Object");
    }
    reader.readEndElement("ObjectData");

    reader.readFiles(zipstream);

    return true;
}

Base::Type ProjectFile::getTypeId(const std::string& name) const
{
    // <Objects Count="1">
    //   <Object type="Mesh::MeshFeature" name="Mesh" />
    // <Objects/>
    if (!xmlDocument) {
        return Base::Type::badType();
    }

    DOMNodeList* nodes = xmlDocument->getElementsByTagName(XStr("Objects").unicodeForm());
    for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
        DOMNode* node = nodes->item(i);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
            DOMNodeList* objectList =
                static_cast<DOMElement*>(node)->getElementsByTagName(XStr("Object").unicodeForm());  // NOLINT
            for (XMLSize_t j = 0; j < objectList->getLength(); j++) {
                DOMNode* objectNode = objectList->item(j);
                DOMNode* typeAttr =
                    objectNode->getAttributes()->getNamedItem(XStr("type").unicodeForm());
                DOMNode* nameAttr =
                    objectNode->getAttributes()->getNamedItem(XStr("name").unicodeForm());
                if (typeAttr && nameAttr) {
                    if (strcmp(name.c_str(), StrX(nameAttr->getNodeValue()).c_str()) == 0) {
                        std::string typeId = StrX(typeAttr->getNodeValue()).c_str();
                        return Base::Type::fromName(typeId.c_str());
                    }
                }
            }
        }
    }

    return Base::Type::badType();
}

std::list<ProjectFile::PropertyFile>
ProjectFile::getPropertyFiles(const std::string& name) const
{
    // <ObjectData Count="1">
    //   <Object name="Mesh">
    //     <Properties Count="1">
    //       <Property name="Mesh" type="Mesh::PropertyMeshKernel">
    //         <Mesh file="MeshKernel.bms"/>
    //       <Property/>
    //     <Properties/>
    //   <Object/>
    // <ObjectData/>
    if (!xmlDocument) {
        return {};
    }

    std::list<PropertyFile> files;
    DOMNodeList* nodes = xmlDocument->getElementsByTagName(XStr("ObjectData").unicodeForm());
    for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
        DOMNode* node = nodes->item(i);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
            DOMNodeList* objectList =
                static_cast<DOMElement*>(node)->getElementsByTagName(XStr("Object").unicodeForm());  // NOLINT
            for (XMLSize_t j = 0; j < objectList->getLength(); j++) {
                DOMNode* objectNode = objectList->item(j);
                DOMNode* nameAttr =
                    objectNode->getAttributes()->getNamedItem(XStr("name").unicodeForm());
                if (nameAttr && strcmp(name.c_str(), StrX(nameAttr->getNodeValue()).c_str()) == 0) {
                    // now go recursively through the sub-tree (i.e. the properties) and collect
                    // every file attribute
                    findFiles(objectNode, files);
                    break;
                }
            }
        }
    }
    return files;
}

void ProjectFile::findFiles(XERCES_CPP_NAMESPACE_QUALIFIER DOMNode* node,
                            std::list<ProjectFile::PropertyFile>& files) const
{
    if (node->hasAttributes()) {
        ProjectFile::PropertyFile prop;
        DOMNode* fileAttr = node->getAttributes()->getNamedItem(XStr("file").unicodeForm());
        if (fileAttr) {
            DOMNode* parentNode = node->getParentNode();
            if (parentNode) {
                DOMNode* nameAttr =
                    parentNode->getAttributes()->getNamedItem(XStr("name").unicodeForm());
                if (nameAttr) {
                    prop.name = StrX(nameAttr->getNodeValue()).c_str();
                }

                DOMNode* typeAttr =
                    parentNode->getAttributes()->getNamedItem(XStr("type").unicodeForm());
                if (typeAttr) {
                    prop.type = Base::Type::fromName(StrX(typeAttr->getNodeValue()).c_str());
                }
            }

            prop.file = StrX(fileAttr->getNodeValue()).c_str();
            files.push_back(prop);
        }
    }

    DOMNodeList* subNodes = node->getChildNodes();
    for (XMLSize_t i = 0; i < subNodes->getLength(); i++) {
        DOMNode* child = subNodes->item(i);
        findFiles(child, files);
    }
}

std::list<std::string> ProjectFile::getInputFiles(const std::string& name) const
{
    // <ObjectData Count="1">
    //   <Object name="Mesh">
    //     <Properties Count="1">
    //       <Property name="Mesh" type="Mesh::PropertyMeshKernel">
    //         <Mesh file="MeshKernel.bms"/>
    //       <Property/>
    //     <Properties/>
    //   <Object/>
    // <ObjectData/>
    if (!xmlDocument) {
        return {};
    }

    std::list<std::string> files;
    DOMNodeList* nodes = xmlDocument->getElementsByTagName(XStr("ObjectData").unicodeForm());
    for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
        DOMNode* node = nodes->item(i);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
            DOMNodeList* objectList =
                static_cast<DOMElement*>(node)->getElementsByTagName(XStr("Object").unicodeForm());  // NOLINT
            for (XMLSize_t j = 0; j < objectList->getLength(); j++) {
                DOMNode* objectNode = objectList->item(j);
                DOMNode* nameAttr =
                    objectNode->getAttributes()->getNamedItem(XStr("name").unicodeForm());
                if (nameAttr && strcmp(name.c_str(), StrX(nameAttr->getNodeValue()).c_str()) == 0) {
                    // now go recursively through the sub-tree (i.e. the properties) and collect
                    // every file attribute
                    findFiles(objectNode, files);
                    break;
                }
            }
        }
    }
    return files;
}

void ProjectFile::findFiles(XERCES_CPP_NAMESPACE_QUALIFIER DOMNode* node,
                            std::list<std::string>& files) const
{
    if (node->hasAttributes()) {
        DOMNode* fileAttr = node->getAttributes()->getNamedItem(XStr("file").unicodeForm());
        if (fileAttr) {
            files.emplace_back(StrX(fileAttr->getNodeValue()).c_str());
        }
    }

    DOMNodeList* subNodes = node->getChildNodes();
    for (XMLSize_t i = 0; i < subNodes->getLength(); i++) {
        DOMNode* child = subNodes->item(i);
        findFiles(child, files);
    }
}

std::string ProjectFile::extractInputFile(const std::string& name)
{
    zipios::ZipFile project(stdFile);
    std::unique_ptr<std::istream> str(project.getInputStream(name));
    if (str) {
        // write it to a tmp. file as writing to the string stream
        // might take too long
        Base::FileInfo fi(Base::FileInfo::getTempFileName());
        Base::ofstream file(fi, std::ios::out | std::ios::binary);
        std::streambuf* buf = file.rdbuf();
        (*str) >> buf;
        file.flush();
        file.close();
        return fi.filePath();
    }

    return {};
}

void ProjectFile::readInputFile(const std::string& name, std::ostream& str)
{
    Base::FileInfo fi(extractInputFile(name));
    if (fi.exists()) {
        Base::ifstream file(fi, std::ios::in | std::ios::binary);
        file >> str.rdbuf();
        file.close();
        fi.deleteFile();
    }
}

// Read the given input file from the zip directly into the given stream (not using a temporary
// file)
void ProjectFile::readInputFileDirect(const std::string& name, std::ostream& str)
{
    zipios::ZipFile project(stdFile);
    std::unique_ptr<std::istream> istr(project.getInputStream(name));
    if (istr) {
        *istr >> str.rdbuf();
    }
}

std::string ProjectFile::replaceInputFile(const std::string& name, std::istream& inp)
{
    // create a new zip file with the name '<zipfile>.<uuid>'
    std::string uuid = Base::Uuid::createUuid();
    std::string fn = stdFile;
    fn += ".";
    fn += uuid;
    Base::FileInfo tmp(fn);
    Base::ofstream newZip(tmp, std::ios::out | std::ios::binary);

    // standard compression
    const int compressionLevel = 6;
    zipios::ZipOutputStream outZip(newZip);
    outZip.setComment("FreeCAD Document");
    outZip.setLevel(compressionLevel);

    // open the original zip file
    zipios::ZipFile project(stdFile);
    zipios::ConstEntries files = project.entries();
    for (const auto& it : files) {
        std::string file = it->getFileName();
        outZip.putNextEntry(file);
        if (file == name) {
            inp >> outZip.rdbuf();
        }
        else {
            std::unique_ptr<std::istream> str(project.getInputStream(file));
            if (str) {
                *str >> outZip.rdbuf();
            }
        }
    }

    project.close();
    outZip.close();
    newZip.close();

    return fn;
}

std::string ProjectFile::replaceInputFiles(const std::map<std::string, std::istream*>& inp)
{
    // create a new zip file with the name '<zipfile>.<uuid>'
    std::string uuid = Base::Uuid::createUuid();
    std::string fn = stdFile;
    fn += ".";
    fn += uuid;
    Base::FileInfo tmp(fn);
    Base::ofstream newZip(tmp, std::ios::out | std::ios::binary);

    // standard compression
    const int compressionLevel = 6;
    zipios::ZipOutputStream outZip(newZip);
    outZip.setComment("FreeCAD Document");
    outZip.setLevel(compressionLevel);

    // open the original zip file
    zipios::ZipFile project(stdFile);
    zipios::ConstEntries files = project.entries();
    for (const auto& it : files) {
        std::string file = it->getFileName();
        outZip.putNextEntry(file);

        auto jt = inp.find(file);
        if (jt != inp.end()) {
            *jt->second >> outZip.rdbuf();
        }
        else {
            std::unique_ptr<std::istream> str(project.getInputStream(file));
            if (str) {
                *str >> outZip.rdbuf();
            }
        }
    }

    project.close();
    outZip.close();
    newZip.close();

    return fn;
}

std::string
ProjectFile::replacePropertyFiles(const std::map<std::string, App::Property*>& props)
{
    // create a new zip file with the name '<zipfile>.<uuid>'
    std::string uuid = Base::Uuid::createUuid();
    std::string fn = stdFile;
    fn += ".";
    fn += uuid;
    Base::FileInfo tmp(fn);
    Base::ofstream newZip(tmp, std::ios::out | std::ios::binary);

    // open extra scope
    {
        // standard compression
        const int compressionLevel = 6;
        Base::ZipWriter writer(newZip);
        writer.setComment("FreeCAD Document");
        writer.setLevel(compressionLevel);

        // open the original zip file
        zipios::ZipFile project(stdFile);
        zipios::ConstEntries files = project.entries();
        for (const auto& it : files) {
            std::string file = it->getFileName();
            writer.putNextEntry(file.c_str());

            auto jt = props.find(file);
            if (jt != props.end()) {
                jt->second->SaveDocFile(writer);
            }
            else {
                std::unique_ptr<std::istream> str(project.getInputStream(file));
                if (str) {
                    *str >> writer.Stream().rdbuf();
                }
            }
        }
        project.close();
    }

    return fn;
}

bool ProjectFile::replaceProjectFile(const std::string& name, bool keepfile)
{
    std::string uuid = Base::Uuid::createUuid();
    std::string fn = stdFile;
    fn += ".";
    fn += uuid;

    // Now rename the original file to something unique
    Base::FileInfo orig(stdFile);
    if (!orig.renameFile(fn.c_str())) {
        return false;
    }
    orig.setFile(fn);

    // rename the tmp.file to the original file name
    Base::FileInfo other(name);
    if (!other.renameFile(stdFile.c_str())) {
        return false;
    }

    // and delete the renamed original file.
    if (!keepfile) {
        if (!orig.deleteFile()) {
            return false;
        }
    }

    return true;
}
