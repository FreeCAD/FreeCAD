/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
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
#endif

#include <App/Application.h>

#include "Materials.h"
#include "MaterialLibrary.h"
#include "ModelManager.h"


using namespace Materials;

/* TRANSLATOR Material::Materials */

TYPESYSTEM_SOURCE(Materials::MaterialLibrary, Base::BaseClass)

MaterialLibrary::MaterialLibrary()
{}

MaterialLibrary::MaterialLibrary(const QString& libraryName, const QDir& dir,
                                 const QString& icon, bool readOnly) :
    _name(libraryName), _directory(dir), _iconPath(icon), _readOnly(readOnly)
{}

MaterialLibrary::~MaterialLibrary()
{
    // delete directory;
}

void MaterialLibrary::createPath(const QString& path)
{
    Q_UNUSED(path)
}

QString MaterialLibrary::getFilePath(const QString &path) const
{
    Base::Console().Log("MaterialLibrary::getFilePath: directory path '%s'\n", getDirectoryPath().toStdString().c_str());

    QString filePath = getDirectoryPath();
    QString prefix = QString::fromStdString("/") + getName();
    if (path.startsWith(prefix))
    {
        // Remove the library name from the path
        filePath += path.right(path.length() - prefix.length());
    } else {
        filePath += path;
    }

    Base::Console().Log("MaterialLibrary::getFilePath: filePath '%s'\n", filePath.toStdString().c_str());
    return filePath;
}

void MaterialLibrary::saveMaterial(Material& material, const QString& path, bool saveAsCopy)
{
    Base::Console().Log("MaterialLibrary::saveMaterial(material(%s), path(%s))\n",
                        material.getName().toStdString().c_str(),
                        path.toStdString().c_str());

    QString filePath = getFilePath(path);
    QFile file(filePath);

    QFileInfo info(file);
    QDir fileDir(info.path());
    if (!fileDir.exists())
        if (!fileDir.mkpath(info.path()))
        {
            Base::Console().Error("Unable to create directory path '%s'\n", info.path().toStdString().c_str());
        }

    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream stream(&file);

        // Write the contents
        material.setLibrary(*this);
        material.setDirectory(fileDir);
        material.save(stream, saveAsCopy);
    }
}

TYPESYSTEM_SOURCE(Materials::MaterialExternalLibrary, MaterialLibrary::MaterialLibrary)

MaterialExternalLibrary::MaterialExternalLibrary()
{}

MaterialExternalLibrary::MaterialExternalLibrary(const QString& libraryName, const QDir& dir,
                                 const QString& icon, bool readOnly) :
    MaterialLibrary(libraryName, dir, icon, readOnly)
{}

MaterialExternalLibrary::~MaterialExternalLibrary()
{
    // delete directory;
}

#include "moc_Materials.cpp"
