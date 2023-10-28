// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <QCoreApplication>
#include <QFile>
#include <QDomDocument>
#include <iostream>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = QCoreApplication::arguments();
    if (args.size() != 2) {
        std::cerr << "Requires input file\n";
        return -1;
    }

    QFile file(args[1]);
    if (!file.open(QFile::ReadOnly)) {
        std::cerr << "Failed to read file\n";
        return -1;
    }

    QDomDocument xml;
    if (!xml.setContent(&file)) {
        std::cerr << "Invalid XML content\n";
        return -1;
    }
    file.close();

    if (!file.open(QFile::WriteOnly)) {
        std::cerr << "Failed to write file\n";
        return -1;
    }

    file.write(xml.toByteArray(4));
    file.close();

    return 0;
}
