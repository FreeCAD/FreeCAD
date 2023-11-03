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

#ifndef MATERIAL_MATERIALCONFIGLOADER_H
#define MATERIAL_MATERIALCONFIGLOADER_H

#include <memory>

#include <QDir>
#include <QSettings>
#include <QString>

#include "Materials.h"

namespace Materials
{

class MaterialConfigLoader
{
public:
    MaterialConfigLoader();
    virtual ~MaterialConfigLoader() = default;


    static bool isConfigStyle(const QString& path);
    static std::shared_ptr<Material> getMaterialFromPath(std::shared_ptr<MaterialLibrary> library,
                                                         const QString& path);

private:
    static QString
    value(const QSettings& fcmat, const std::string& name, const std::string& defaultValue)
    {
        return fcmat.value(QString::fromStdString(name), QString::fromStdString(defaultValue))
            .toString();
    }

    static void setPhysicalValue(std::shared_ptr<Material> finalModel,
                                 const std::string& name,
                                 const QString& value)
    {
        if (value.length() > 0) {
            finalModel->setPhysicalValue(QString::fromStdString(name), value);
        }
    }
    static void setAppearanceValue(std::shared_ptr<Material> finalModel,
                                   const std::string& name,
                                   const QString& value)
    {
        if (value.length() > 0) {
            finalModel->setAppearanceValue(QString::fromStdString(name), value);
        }
    }

    static QString getAuthorAndLicense(const QString& path);
    static void addMechanical(const QSettings& fcmat, std::shared_ptr<Material> finalModel);
    static void addFluid(const QSettings& fcmat, std::shared_ptr<Material> finalModel);
    static void addThermal(const QSettings& fcmat, std::shared_ptr<Material> finalModel);
    static void addElectromagnetic(const QSettings& fcmat, std::shared_ptr<Material> finalModel);
    static void addArchitectural(const QSettings& fcmat, std::shared_ptr<Material> finalModel);
    static void addCosts(const QSettings& fcmat, std::shared_ptr<Material> finalModel);
    static void addRendering(const QSettings& fcmat, std::shared_ptr<Material> finalModel);
    static void addVectorRendering(const QSettings& fcmat, std::shared_ptr<Material> finalModel);
};

}  // namespace Materials

#endif  // MATERIAL_MATERIALCONFIGLOADER_H
