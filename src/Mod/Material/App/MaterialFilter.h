// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023-2024 David Carter <dcarter@david.carter.ca>        *
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

#include <QMetaType>
#include <QSet>
#include <QString>

#include <Base/BaseClass.h>

#include <Mod/Material/MaterialGlobal.h>

namespace Materials
{

class Material;

/*
 * This class is used to set options for a material tree search
 *
 */
class MaterialsExport MaterialFilterOptions: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    MaterialFilterOptions();
    virtual ~MaterialFilterOptions() = default;

    /* Indicates if we should show favourite materials
     *
     * Default is to show favourite materials
     */
    bool includeFavorites() const
    {
        return _includeFavorites;
    }
    void setIncludeFavorites(bool value)
    {
        _includeFavorites = value;
    }

    /* Indicates if we should show recent materials
     *
     * Default is to show recent materials
     */
    bool includeRecent() const
    {
        return _includeRecent;
    }
    void setIncludeRecent(bool value)
    {
        _includeRecent = value;
    }

    /* Indicates if we should include empty folders
     *
     * Default is not to include empty folders
     */
    bool includeEmptyFolders() const
    {
        return _includeFolders;
    }
    void setIncludeEmptyFolders(bool value)
    {
        _includeFolders = value;
    }

    /* Indicates if we should include empty libraries
     *
     * Default is to include empty libraries
     */
    bool includeEmptyLibraries() const
    {
        return _includeLibraries;
    }
    void setIncludeEmptyLibraries(bool value)
    {
        _includeLibraries = value;
    }

    /* Indicates if we should include materials in the older format
     *
     * Default is not to include legacy format materials
     */
    bool includeLegacy() const
    {
        return _includeLegacy;
    }
    void setIncludeLegacy(bool legacy)
    {
        _includeLegacy = legacy;
    }

protected:
    bool _includeFavorites;
    bool _includeRecent;
    bool _includeFolders;
    bool _includeLibraries;
    bool _includeLegacy;
};

/*
 * The same class initialized with preferences for the MaterialTreeWidget
 *
 */
class MaterialsExport MaterialFilterTreeWidgetOptions: public MaterialFilterOptions
{

public:
    MaterialFilterTreeWidgetOptions();
    ~MaterialFilterTreeWidgetOptions() override = default;
};

/*
 * This class is used to filter materials during a material tree search
 *
 */
class MaterialsExport MaterialFilter: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    MaterialFilter();
    virtual ~MaterialFilter() = default;

    /*
     * Filter name when used in a list of filters. The name should be
     * unique within the list.
     */
    QString name() const
    {
        return _name;
    }
    void setName(const QString& name)
    {
        _name = name;
    }

    /* Sets of model UUIDs that should be included. Optionally, we can
     * specify models that must contain values for all properties.
     *
     * Models only need to be included in one set.
     */
    bool modelIncluded(const Material& material) const;
    bool modelIncluded(const QString& uuid) const;

    /* Add model UUIDs for required models, or models that are both required
     * and complete.
     */
    void addRequired(const QString& uuid);
    void addRequiredComplete(const QString& uuid);

    /* Require that the materials have physical properties defined.
     */
    void requirePhysical(bool required) { _requirePhysical = required; }

    /* Require that the materials have appearance properties defined.
     */
    void requireAppearance(bool required) { _requireAppearance = required; }

    /* These functions shouldn't normally be called directly. They are
     * for use by conversion methods, such as MaterialFilterPy
     */
    const QSet<QString>* getRequired() const
    {
        return &_required;
    }
    const QSet<QString>* getRequiredComplete() const
    {
        return &_requiredComplete;
    }

    void clear();

private:
    QString _name;
    QSet<QString> _required;
    QSet<QString> _requiredComplete;
    bool _requirePhysical;
    bool _requireAppearance;
};

}  // namespace Materials

Q_DECLARE_METATYPE(Materials::MaterialFilter)