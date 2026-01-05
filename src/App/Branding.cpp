// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2015 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
 *   write to the Free Software Foundation, Inc., 51 Franklin Street,      *
 *   Fifth Floor, Boston, MA  02110-1301, USA                              *
 *                                                                         *
 ***************************************************************************/

#include "Branding.h"

#include <mutex>
#include <system_error>

#include <xercesc/dom/DOM.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/PlatformUtils.hpp>

#include <Base/FileInfo.h>
#include <Base/XMLTools.h>


using namespace App;
using namespace XERCES_CPP_NAMESPACE;

Branding::Branding()
{
    filter.insert("Application");
    filter.insert("WindowTitle");
    filter.insert("CopyrightInfo");
    filter.insert("MaintainerUrl");
    filter.insert("WindowIcon");
    filter.insert("ProgramLogo");
    filter.insert("ProgramIcons");
    filter.insert("DesktopFileName");
    filter.insert("StyleSheet");

    filter.insert("BuildVersionMajor");
    filter.insert("BuildVersionMinor");
    filter.insert("BuildVersionPoint");
    filter.insert("BuildRevision");
    filter.insert("BuildRevisionDate");
    filter.insert("BuildVersionSuffix");
    filter.insert("BuildRepositoryURL");

    filter.insert("AboutImage");
    filter.insert("SplashScreen");
    filter.insert("SplashAlignment");
    filter.insert("SplashTextColor");
    filter.insert("SplashInfoColor");
    filter.insert("SplashInfoFont");
    filter.insert("SplashInfoPosition");
    filter.insert("SplashWarningColor");

    filter.insert("StartWorkbench");

    filter.insert("ExeName");
    filter.insert("ExeVendor");
    filter.insert("ExeVersion");
    filter.insert("AppDataSkipVendor");
    filter.insert("NavigationStyle");
    filter.insert("UserParameterTemplate");
}

static void ensureXercesInitialized()
{
    static std::once_flag once;
    std::call_once(once, []() {
        XMLPlatformUtils::Initialize();
        XMLTools::initialize();
    });
}

bool Branding::readFile(const std::filesystem::path& filePath)
{
    userDefines.clear();

    std::error_code error;
    if (!std::filesystem::exists(filePath, error) || error) {
        return false;
    }

    try {
        ensureXercesInitialized();

        XercesDOMParser parser;
        parser.setValidationScheme(XercesDOMParser::Val_Never);
        parser.setDoNamespaces(true);
        parser.setLoadExternalDTD(false);
        parser.setCreateEntityReferenceNodes(false);

        HandlerBase errorHandler;
        parser.setErrorHandler(&errorHandler);

        const std::string filename = Base::FileInfo::pathToString(filePath);
        XStr xmlPath(filename.c_str());
        LocalFileInputSource source(xmlPath.unicodeForm());
        parser.parse(source);

        DOMDocument* doc = parser.getDocument();
        if (!doc) {
            return false;
        }

        DOMElement* root = doc->getDocumentElement();
        if (!root) {
            return false;
        }

        const XMLCh* rootNameXml = root->getLocalName();
        if (!rootNameXml) {
            rootNameXml = root->getTagName();
        }
        const std::string rootName = XMLTools::toStdString(rootNameXml);
        if (rootName != "Branding") {
            return false;
        }

        if (root->hasAttribute(XStrLiteral("version").unicodeForm())) {
            const std::string version = XMLTools::toStdString(root->getAttribute(XStrLiteral("version").unicodeForm()));
            if (version != "1.0") {
                return false;
            }
        }

        for (DOMNode* child = root->getFirstChild(); child; child = child->getNextSibling()) {
            if (child->getNodeType() != DOMNode::ELEMENT_NODE) {
                continue;
            }
            auto* elem = static_cast<DOMElement*>(child);
            const XMLCh* nameXml = elem->getLocalName();
            if (!nameXml) {
                nameXml = elem->getTagName();
            }
            const std::string name = XMLTools::toStdString(nameXml);
            if (filter.find(name) == filter.end()) {
                continue;
            }
            const std::string value = XMLTools::toStdString(elem->getTextContent());
            userDefines[name] = value;
        }

        return true;
    }
    catch (...) {
        return false;
    }
}

const Branding::XmlConfig& Branding::getUserDefines() const
{
    return userDefines;
}
