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
#include <QList>
#include <QMap>
#include <QSettings>
#include <QString>
#include <QVariant>

#include "Materials.h"

namespace Materials
{

class MaterialLibraryLocal;

class MaterialConfigLoader
{
public:
    MaterialConfigLoader() = default;
    virtual ~MaterialConfigLoader() = default;


    static bool isConfigStyle(const QString& path);
    static std::shared_ptr<Material>
    getMaterialFromPath(const std::shared_ptr<MaterialLibraryLocal>& library, const QString& path);

private:
    static QString value(const QMap<QString, QString>& fcmat,
                         const std::string& name,
                         const std::string& defaultValue)
    {
        try {
            return fcmat[QString::fromStdString(name)];
        }
        catch (const std::out_of_range&) {
        }

        return QString::fromStdString(defaultValue);
    }

    static void setPhysicalValue(const std::shared_ptr<Material>& finalModel,
                                 const std::string& name,
                                 const QString& value)
    {
        if (!value.isEmpty()) {
            finalModel->setPhysicalValue(QString::fromStdString(name), value);
        }
    }
    static void setAppearanceValue(const std::shared_ptr<Material>& finalModel,
                                   const std::string& name,
                                   const QString& value)
    {
        if (!value.isEmpty()) {
            finalModel->setAppearanceValue(QString::fromStdString(name), value);
        }
    }
    static void setAppearanceValue(const std::shared_ptr<Material>& finalModel,
                                   const std::string& name,
                                   const std::shared_ptr<QList<QVariant>>& value)
    {
        if (!value->isEmpty()) {
            finalModel->setAppearanceValue(QString::fromStdString(name), value);
        }
    }
    static void setLegacyValue(const std::shared_ptr<Material>& finalModel,
                                   const std::string& name,
                                   const QString& value)
    {
        if (!value.isEmpty()) {
            finalModel->setLegacyValue(QString::fromStdString(name), value);
        }
    }

    static bool isTexture(const QString& value)
    {
        return value.contains(QStringLiteral("Texture"), Qt::CaseInsensitive);
    }

    static bool readFile(const QString& path, QMap<QString, QString>& map);
    static void splitTexture(const QString& value, QString* texture, QString* remain);
    static void
    splitTextureObject(const QString& value, QString* texture, QString* remain, QString* object);

    static QString getAuthorAndLicense(const QString& path);
    static void addMechanical(const QMap<QString, QString>& fcmat,
                              const std::shared_ptr<Material>& finalModel);
    static void addFluid(const QMap<QString, QString>& fcmat,
                         const std::shared_ptr<Material>& finalModel);
    static void addThermal(const QMap<QString, QString>& fcmat,
                           const std::shared_ptr<Material>& finalModel);
    static void addElectromagnetic(const QMap<QString, QString>& fcmat,
                                   const std::shared_ptr<Material>& finalModel);
    static void addArchitectural(const QMap<QString, QString>& fcmat,
                                 const std::shared_ptr<Material>& finalModel);
    static void addCosts(const QMap<QString, QString>& fcmat,
                         const std::shared_ptr<Material>& finalModel);
    static void addRendering(const QMap<QString, QString>& fcmat,
                             const std::shared_ptr<Material>& finalModel);
    static void addVectorRendering(const QMap<QString, QString>& fcmat,
                                   const std::shared_ptr<Material>& finalModel);

    static QString multiLineKey(QMap<QString, QString>& fcmat, const QString& prefix);
    static void addRenderAppleseed(QMap<QString, QString>& fcmat,
                                   const std::shared_ptr<Material>& finalModel);
    static void addRenderCarpaint(QMap<QString, QString>& fcmat,
                                  const std::shared_ptr<Material>& finalModel);
    static void addRenderCycles(QMap<QString, QString>& fcmat,
                                const std::shared_ptr<Material>& finalModel);
    static void addRenderDiffuse(QMap<QString, QString>& fcmat,
                                 const std::shared_ptr<Material>& finalModel);
    static void addRenderDisney(QMap<QString, QString>& fcmat,
                                const std::shared_ptr<Material>& finalModel);
    static void addRenderEmission(QMap<QString, QString>& fcmat,
                                  const std::shared_ptr<Material>& finalModel);
    static void addRenderGlass(QMap<QString, QString>& fcmat,
                               const std::shared_ptr<Material>& finalModel);
    static void addRenderLuxcore(QMap<QString, QString>& fcmat,
                                 const std::shared_ptr<Material>& finalModel);
    static void addRenderLuxrender(QMap<QString, QString>& fcmat,
                                   const std::shared_ptr<Material>& finalModel);
    static void addRenderMixed(QMap<QString, QString>& fcmat,
                               const std::shared_ptr<Material>& finalModel);
    static void addRenderOspray(QMap<QString, QString>& fcmat,
                                const std::shared_ptr<Material>& finalModel);
    static void addRenderPbrt(QMap<QString, QString>& fcmat,
                              const std::shared_ptr<Material>& finalModel);
    static void addRenderPovray(QMap<QString, QString>& fcmat,
                                const std::shared_ptr<Material>& finalModel);
    static void addRenderSubstancePBR(QMap<QString, QString>& fcmat,
                                      const std::shared_ptr<Material>& finalModel);
    static void addRenderTexture(QMap<QString, QString>& fcmat,
                                 const std::shared_ptr<Material>& finalModel);
    static void addRenderWB(QMap<QString, QString>& fcmat,
                            const std::shared_ptr<Material>& finalModel);
    static void addLegacy(const QMap<QString, QString>& fcmat,
                            const std::shared_ptr<Material>& finalModel);
};

}  // namespace Materials