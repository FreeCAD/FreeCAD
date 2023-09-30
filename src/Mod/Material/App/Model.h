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

#ifndef MATERIAL_MODEL_H
#define MATERIAL_MODEL_H

#include <Mod/Material/MaterialGlobal.h>

#include <Base/BaseClass.h>
#include <Base/Quantity.h>
#include <QDir>
#include <QString>

#include "MaterialValue.h"
#include "ModelLibrary.h"

namespace Materials
{

class MaterialsExport ModelProperty: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    ModelProperty();
    explicit ModelProperty(const QString& name,
                           const QString& type,
                           const QString& units,
                           const QString& url,
                           const QString& description);
    explicit ModelProperty(const ModelProperty& other);
    ~ModelProperty() override = default;

    const QString getName() const
    {
        return _name;
    }
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

private:
    QString _name;
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
    explicit Model(const ModelLibrary& library,
                   ModelType type,
                   const QString& name,
                   const QString& directory,
                   const QString& uuid,
                   const QString& description,
                   const QString& url,
                   const QString& doi);
    ~Model() override = default;

    const ModelLibrary& getLibrary() const
    {
        return _library;
    }
    const QString getBase() const
    {
        return (_type == ModelType_Physical) ? QString::fromStdString("Model")
                                             : QString::fromStdString("AppearanceModel");
    }
    const QString getName() const
    {
        return _name;
    }
    ModelType getType() const
    {
        return _type;
    }
    const QString getDirectory() const
    {
        return _directory;
    }
    const QString getDirectoryPath() const
    {
        return QDir(_directory).absolutePath();
    }
    const QString getRelativePath() const
    {
        return QDir(_directory).relativeFilePath(QDir(_directory).absolutePath());
    }
    const QString getUUID() const
    {
        return _uuid;
    }
    const QString getDescription() const
    {
        return _description;
    }
    const QString getURL() const
    {
        return _url;
    }
    const QString getDOI() const
    {
        return _doi;
    }

    void setLibrary(const ModelLibrary& library)
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
    void setDirectory(const QString& directory)
    {
        _directory = directory;
    }
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
        _inheritedUuids.push_back(uuid);
    }
    const std::vector<QString>& getInheritance() const
    {
        return _inheritedUuids;
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

private:
    ModelLibrary _library;
    ModelType _type;
    QString _name;
    QString _directory;
    QString _uuid;
    QString _description;
    QString _url;
    QString _doi;
    std::vector<QString> _inheritedUuids;
    std::map<QString, ModelProperty> _properties;
};

}  // namespace Materials

#endif  // MATERIAL_MODEL_H
