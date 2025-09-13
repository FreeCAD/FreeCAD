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



#include <QFile>

#include "Branding.h"


using namespace App;

Branding::Branding()
{
    filter.push_back("Application");
    filter.push_back("WindowTitle");
    filter.push_back("CopyrightInfo");
    filter.push_back("MaintainerUrl");
    filter.push_back("WindowIcon");
    filter.push_back("ProgramLogo");
    filter.push_back("ProgramIcons");
    filter.push_back("DesktopFileName");
    filter.push_back("StyleSheet");

    filter.push_back("BuildVersionMajor");
    filter.push_back("BuildVersionMinor");
    filter.push_back("BuildVersionPoint");
    filter.push_back("BuildRevision");
    filter.push_back("BuildRevisionDate");
    filter.push_back("BuildVersionSuffix");
    filter.push_back("BuildRepositoryURL");

    filter.push_back("AboutImage");
    filter.push_back("SplashScreen");
    filter.push_back("SplashAlignment");
    filter.push_back("SplashTextColor");
    filter.push_back("SplashInfoColor");
    filter.push_back("SplashInfoFont");
    filter.push_back("SplashInfoPosition");
    filter.push_back("SplashWarningColor");

    filter.push_back("StartWorkbench");

    filter.push_back("ExeName");
    filter.push_back("ExeVendor");
    filter.push_back("ExeVersion");
    filter.push_back("AppDataSkipVendor");
    filter.push_back("NavigationStyle");
    filter.push_back("UserParameterTemplate");
}

bool Branding::readFile(const QString& fn)
{
    QFile file(fn);
    if (!file.open(QFile::ReadOnly)) {
        return false;
    }
    if (!evaluateXML(&file, domDocument)) {
        return false;
    }
    file.close();
    return true;
}

Branding::XmlConfig Branding::getUserDefines() const
{
    XmlConfig cfg;
    QDomElement root = domDocument.documentElement();
    QDomElement child;
    if (!root.isNull()) {
        child = root.firstChildElement();
        while (!child.isNull()) {
            std::string name = child.localName().toLatin1().constData();
            std::string value = child.text().toUtf8().constData();
            if (filter.contains(name)) {
                cfg[name] = std::move(value);
            }
            child = child.nextSiblingElement();
        }
    }
    return cfg;
}

bool Branding::evaluateXML(QIODevice* device, QDomDocument& xmlDocument)
{
#if QT_VERSION >= QT_VERSION_CHECK(6,5,0)
    if (!xmlDocument.setContent(device, QDomDocument::ParseOption::UseNamespaceProcessing)) {
        return false;
    }
#else
    QString errorStr;
    int errorLine;
    int errorColumn;
    if (!xmlDocument.setContent(device, true, &errorStr, &errorLine, &errorColumn)) {
        return false;
    }
#endif

    QDomElement root = xmlDocument.documentElement();
    if (root.tagName() != QLatin1String("Branding")) {
        return false;
    }
    if (root.hasAttribute(QLatin1String("version"))) {
        QString attr = root.attribute(QLatin1String("version"));
        if (attr != QLatin1String("1.0")) {
            return false;
        }
    }

    return true;
}
