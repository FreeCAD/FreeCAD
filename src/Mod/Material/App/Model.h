// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <memory>

#include <QDir>
#include <QString>
#include <QStringList>

#include <Base/BaseClass.h>
#include <Base/Quantity.h>

#include <Mod/Material/MaterialGlobal.h>

#include "FolderTree.h"
#include "MaterialValue.h"
// #include "ModelLibrary.h"

namespace Materials
{

class ModelLibrary;

enum ModelFilter
{
    ModelFilter_None,
    ModelFilter_Physical,
    ModelFilter_Appearance
};

class MaterialsExport ModelProperty: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    ModelProperty();
    ModelProperty(const QString& name,
                  const QString& header,
                  const QString& type,
                  const QString& units,
                  const QString& url,
                  const QString& description);
    ModelProperty(const ModelProperty& other);
    ~ModelProperty() override = default;

    const QString getName() const
    {
        return _name;
    }
    const QString getDisplayName() const;
    const QString getPropertyType() const
    {
        return _propertyType;
    }
    const QString getUnits() const
    {
        return _units;
    }
    const QString getURL() const
    {
        return _url;
    }
    const QString getDescription() const
    {
        return _description;
    }
    const QString getInheritance() const
    {
        return _inheritance;
    }
    bool isInherited() const
    {
        return (_inheritance.length() > 0);
    }

    void setName(const QString& name)
    {
        _name = name;
    }
    void setDisplayName(const QString& header)
    {
        _displayName = header;
    }
    virtual void setPropertyType(const QString& type)
    {
        _propertyType = type;
    }
    void setUnits(const QString& units)
    {
        _units = units;
    }
    void setURL(const QString& url)
    {
        _url = url;
    }
    void setDescription(const QString& description)
    {
        _description = description;
    }
    void setInheritance(const QString& uuid)
    {
        _inheritance = uuid;
    }

    void addColumn(ModelProperty& column)
    {
        _columns.push_back(column);
    }
    const std::vector<ModelProperty>& getColumns() const
    {
        return _columns;
    }
    int columns() const
    {
        return _columns.size();
    }

    ModelProperty& operator=(const ModelProperty& other);
    bool operator==(const ModelProperty& other) const;
    bool operator!=(const ModelProperty& other) const
    {
        return !operator==(other);
    }

    void validate(const ModelProperty& other) const;

private:
    QString _name;
    QString _displayName;
    QString _propertyType;
    QString _units;
    QString _url;
    QString _description;
    QString _inheritance;
    std::vector<ModelProperty> _columns;
};

class MaterialsExport Model: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    enum ModelType
    {
        ModelType_Physical,
        ModelType_Appearance
    };

    Model();
    Model(std::shared_ptr<ModelLibrary> library,
          ModelType type,
          const QString& name,
          const QString& directory,
          const QString& uuid,
          const QString& description,
          const QString& url,
          const QString& doi);
    ~Model() override = default;

    std::shared_ptr<ModelLibrary> getLibrary() const
    {
        return _library;
    }
    QString getBase() const
    {
        return (_type == ModelType_Physical) ? QStringLiteral("Model")
                                             : QStringLiteral("AppearanceModel");
    }
    QString getName() const
    {
        return _name;
    }
    ModelType getType() const
    {
        return _type;
    }
    QString getDirectory() const;
    QString getFilename() const;
    QString getFilePath() const;
    QString getUUID() const
    {
        return _uuid;
    }
    QString getDescription() const
    {
        return _description;
    }
    QString getURL() const
    {
        return _url;
    }
    QString getDOI() const
    {
        return _doi;
    }

    void setLibrary(std::shared_ptr<ModelLibrary> library)
    {
        _library = library;
    }
    void setType(ModelType type)
    {
        _type = type;
    }
    void setName(const QString& name)
    {
        _name = name;
    }
    void setDirectory(const QString& directory);
    void setFilename(const QString& filename);
    void setUUID(const QString& uuid)
    {
        _uuid = uuid;
    }
    void setDescription(const QString& description)
    {
        _description = description;
    }
    void setURL(const QString& url)
    {
        _url = url;
    }
    void setDOI(const QString& doi)
    {
        _doi = doi;
    }

    void addInheritance(const QString& uuid)
    {
        _inheritedUuids << uuid;
    }
    const QStringList& getInheritance() const
    {
        return _inheritedUuids;
    }
    bool inherits(const QString& uuid) const
    {
        return _inheritedUuids.contains(uuid);
    }

    bool operator==(const Model& m) const
    {
        return _uuid == m._uuid;
    }
    bool operator!=(const Model& m) const
    {
        return !operator==(m);
    }

    ModelProperty& operator[](const QString& key);
    void addProperty(ModelProperty& property)
    {
        _properties[property.getName()] = property;
    }

    using iterator = typename std::map<QString, ModelProperty>::iterator;
    using const_iterator = typename std::map<QString, ModelProperty>::const_iterator;
    iterator begin()
    {
        return _properties.begin();
    }
    const_iterator begin() const noexcept
    {
        return _properties.begin();
    }
    iterator end() noexcept
    {
        return _properties.end();
    }
    const_iterator end() const noexcept
    {
        return _properties.end();
    }
    const_iterator cbegin() const noexcept
    {
        return _properties.cbegin();
    }
    const_iterator cend() const noexcept
    {
        return _properties.cend();
    }

    void validate(Model& other) const;

private:
    std::shared_ptr<ModelLibrary> _library;
    ModelType _type;
    QString _name;
    QString _directory;
    QString _filename;
    QString _uuid;
    QString _description;
    QString _url;
    QString _doi;
    QStringList _inheritedUuids;
    std::map<QString, ModelProperty> _properties;
};

typedef FolderTreeNode<Model> ModelTreeNode;

}  // namespace Materials