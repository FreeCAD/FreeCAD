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

#ifndef MATERIAL_MATERIALFILTER_H
#define MATERIAL_MATERIALFILTER_H

#include <memory>

#include <QSet>
#include <QString>

#include <Base/BaseClass.h>

#include <Mod/Material/MaterialGlobal.h>

namespace Materials
{

class Material;

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

    /* Indicates if we should include empty folders
     *
     * Default is to include empty folders
     */
    bool includeEmptyFolders() const
    {
        return _includeFolders;
    }
    void setIncludeEmptyFolders(bool include)
    {
        _includeFolders = include;
    }

    /* Indicates if we should include materials in the older format
     *
     * Default is to include legacy format materials
     */
    bool includeLegacy() const
    {
        return _includeLegacy;
    }
    void setIncludeLegacy(bool legacy)
    {
        _includeLegacy = legacy;
    }

    /* Sets of model UUIDs that should be included. Optionally, we can
     * specify models that must contain values for all properties.
     *
     * Models only need to be included in one set.
     */
    bool modelIncluded(const std::shared_ptr<Material>& material) const;

    void addRequired(const QString& uuid);
    void addRequiredComplete(const QString& uuid);

private:
    bool _includeFolders;
    bool _includeLegacy;
    QSet<QString> _required;
    QSet<QString> _requiredComplete;
};

}  // namespace Materials

#endif  // MATERIAL_MATERIALFILTER_H
