/***************************************************************************
 *   Copyright (c) 2023 Benjamin Bræstrup Sayoc <benj5378@outlook.com>     *
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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QStringList>
# include <QStandardPaths>
# include <QJsonDocument>
# include <QJsonArray>
# include <QByteArray>
#endif

#include <App/Application.h>
 
#include "FontPaths.h" 

using namespace App;

QStringList FontPaths::getSystemPaths() {
    return QStandardPaths::standardLocations(QStandardPaths::FontsLocation);
}

QStringList FontPaths::getPreferencePaths() {
    QByteArray rawJsonData = QByteArray::fromStdString(
        App::GetApplication().GetUserParameter().GetGroup("BaseApp/Preferences/Fonts")->GetASCII("FontPaths")
    );
    QJsonArray jsonPaths = QJsonDocument::fromJson(rawJsonData).array();
    QStringList stringPaths;
    for(QJsonValue jsonPath : jsonPaths ) {
        stringPaths.push_back(jsonPath.toString());
    }
    return stringPaths;
}

QStringList FontPaths::getAllPaths() {
    return getSystemPaths() + getPreferencePaths();
}