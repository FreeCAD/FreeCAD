// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2015 Werner Mayer
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#pragma once

#include <string>
#include <QDomDocument>
#include <QMap>
#include <QString>
#include <QVector>


class QIODevice;

namespace App
{

class Branding
{
public:
    using XmlConfig = QMap<std::string, std::string>;
    Branding();

    bool readFile(const QString& fn);
    XmlConfig getUserDefines() const;

private:
    QVector<std::string> filter;
    bool evaluateXML(QIODevice* device, QDomDocument& xmlDocument);
    QDomDocument domDocument;
};

}  // namespace App
