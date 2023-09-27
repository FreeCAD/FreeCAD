/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
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

#ifndef MATERIAL_MATERIALLIBRARY_H
#define MATERIAL_MATERIALLIBRARY_H

#include <Mod/Material/MaterialGlobal.h>

#include <Base/BaseClass.h>
#include <memory>
#include <QDir>
#include <QString>
#include <QVariant>

#include "Model.h"
#include "ModelLibrary.h"

namespace Materials
{

class Material;

class MaterialsExport MaterialLibrary: public LibraryBase
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    MaterialLibrary();
    explicit MaterialLibrary(const QString& libraryName,
                             const QString& dir,
                             const QString& icon,
                             bool readOnly = true);
    ~MaterialLibrary() override = default;

    bool operator==(const MaterialLibrary& library) const
    {
        return LibraryBase::operator==(library);
    }
    bool operator!=(const MaterialLibrary& library) const
    {
        return !operator==(library);
    }
    const Material& getMaterialByPath(const QString& path) const;

    void createPath(const QString& path);
    Material* saveMaterial(Material& material, const QString& path, bool saveAsCopy);
    Material* addMaterial(const Material& material, const QString& path);

    bool isReadOnly() const
    {
        return _readOnly;
    }

protected:
    const QString getUUIDFromPath(const QString& path) const;

    bool _readOnly;
    static std::unique_ptr<std::map<QString, Material*>> _materialPathMap;
};

class MaterialsExport MaterialExternalLibrary: public MaterialLibrary
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    MaterialExternalLibrary();
    explicit MaterialExternalLibrary(const QString& libraryName,
                                     const QString& dir,
                                     const QString& icon,
                                     bool readOnly = true);
    ~MaterialExternalLibrary() override;
};

}  // namespace Materials

Q_DECLARE_METATYPE(Materials::MaterialLibrary)
Q_DECLARE_METATYPE(Materials::MaterialExternalLibrary)

#endif  // MATERIAL_MATERIALLIBRARY_H
