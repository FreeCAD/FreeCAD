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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QDirIterator>
#include <QFileInfo>
#include <QString>
#include <QFile>
#include <QIODevice>
#include <QTextStream>
#include <QUuid>
#include <memory>
#include <fstream>
#endif


#include <App/Application.h>
#include <Base/Interpreter.h>


#include "Exceptions.h"
#include "MaterialConfigLoader.h"
#include "MaterialLoader.h"
#include "Model.h"
#include "ModelUuids.h"


using namespace Materials;

bool MaterialConfigLoader::isConfigStyle(const QString& path)
{
    QSettings fcmat(path, QSettings::IniFormat);

    // No [sections] means not .ini
    if (fcmat.childGroups().empty()) {
        return false;
    }

    // Sometimes arrays can create a false positive
    QFile infile(path);
    if (infile.open(QIODevice::ReadOnly)) {
        QTextStream in(&infile);

        if (!in.atEnd()) {
            auto line = in.readLine();
            if (line.trimmed().startsWith(QLatin1Char('-'))
                || line.trimmed().startsWith(QLatin1Char('#'))) {
                // Definitely a YAML file
                return false;
            }
        }
    }
    infile.close();

    // No false positive
    return true;
}

bool MaterialConfigLoader::readFile(const QString& path, QMap<QString, QString>& map)
{
    // This function is necessary as the built in routines don't always return the full value string
    QFile infile(path);
    if (infile.open(QIODevice::ReadOnly)) {
        QTextStream in(&infile);
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        in.setCodec("UTF-8");
#endif
        QString line;
        QString prefix;
        while (!in.atEnd()) {
            line = in.readLine();
            if (line.trimmed().startsWith(QLatin1Char(';'))) {
                continue;
            }

            if (line.startsWith(QLatin1Char('['))) {
                // read prefix
                auto end = line.indexOf(QLatin1Char(']'));
                if (end > 1) {
                    prefix = line.mid(1, end - 1) + QStringLiteral("/");

                    // Render WB uses both Render and Rendering
                    if (prefix == QStringLiteral("Rendering/")) {
                        prefix = QStringLiteral("Render/");
                    }
                }
            }
            else {
                auto separator = line.indexOf(QLatin1Char('='));
                if (separator > 2) {
                    auto left = line.mid(0, separator - 1);
                    auto right = line.mid(separator + 2);
                    map[prefix + left] = right;
                }
            }
        }
        infile.close();
        return true;
    }

    return false;
}

void MaterialConfigLoader::splitTexture(const QString& value, QString* texture, QString* remain)
{
    // Split Texture(...);(...) into its two pieces
    if (value.contains(QLatin1Char(';'))) {
        auto separator = value.indexOf(QLatin1Char(';'));
        auto left = value.mid(0, separator);
        auto right = value.mid(separator + 1);
        if (isTexture(left)) {
            *texture = left;
            *remain = right;
        }
        else {
            *texture = right;
            *remain = left;
        }
    }
    else {
        if (isTexture(value)) {
            *texture = value;
        }
        else {
            *remain = value;
        }
    }
}

void MaterialConfigLoader::splitTextureObject(const QString& value,
                                              QString* texture,
                                              QString* remain,
                                              QString* object)
{
    splitTexture(value, texture, remain);
    if (*remain == QStringLiteral("Object")) {
        *remain = QString();  // Empty string
        *object = QStringLiteral("true");
    }
}

QString MaterialConfigLoader::getAuthorAndLicense(const QString& path)
{
    std::ifstream infile(path.toStdString());
    QString noAuthor;

    // Skip the first line
    std::string line;
    if (!std::getline(infile, line)) {
        return noAuthor;
    }

    // The second line has it in a comment
    if (!std::getline(infile, line)) {
        return noAuthor;
    }
    std::size_t found = line.find(';');
    if (found != std::string::npos) {
        return QString::fromStdString(trim_copy(line.substr(found + 1)));
    }

    return noAuthor;
}

void MaterialConfigLoader::addVectorRendering(const QMap<QString, QString>& fcmat,
                                              const std::shared_ptr<Material>& finalModel)
{
    QString sectionFillPattern = value(fcmat, "VectorRendering/SectionFillPattern", "");
    QString sectionLinewidth = value(fcmat, "VectorRendering/SectionLinewidth", "");
    QString sectionColor = value(fcmat, "VectorRendering/SectionColor", "");
    QString viewColor = value(fcmat, "VectorRendering/ViewColor", "");
    QString viewFillPattern = value(fcmat, "VectorRendering/ViewFillPattern", "");
    QString viewLinewidth = value(fcmat, "VectorRendering/ViewLinewidth", "");

    // Defined by the Render WB
    QString aSection = value(fcmat, "Architectural/SectionColor", "");

    if (!aSection.isEmpty()) {
        sectionColor = aSection;
    }

    if (sectionFillPattern.length() + sectionLinewidth.length() + sectionColor.length()
            + viewColor.length() + viewFillPattern.length() + viewLinewidth.length()
        > 0) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Rendering_Vector);

        // Now add the data
        setAppearanceValue(finalModel, "SectionFillPattern", sectionFillPattern);
        setAppearanceValue(finalModel, "SectionLinewidth", sectionLinewidth);
        setAppearanceValue(finalModel, "SectionColor", sectionColor);
        setAppearanceValue(finalModel, "ViewColor", viewColor);
        setAppearanceValue(finalModel, "ViewFillPattern", viewFillPattern);
        setAppearanceValue(finalModel, "ViewLinewidth", viewLinewidth);
    }
}

void MaterialConfigLoader::addRendering(const QMap<QString, QString>& fcmat,
                                        const std::shared_ptr<Material>& finalModel)
{
    QString ambientColor = value(fcmat, "Rendering/AmbientColor", "");
    QString diffuseColor = value(fcmat, "Rendering/DiffuseColor", "");
    QString emissiveColor = value(fcmat, "Rendering/EmissiveColor", "");
    QString shininess = value(fcmat, "Rendering/Shininess", "");
    QString specularColor = value(fcmat, "Rendering/SpecularColor", "");
    QString transparency = value(fcmat, "Rendering/Transparency", "");
    QString texturePath = value(fcmat, "Rendering/TexturePath", "");
    QString textureScaling = value(fcmat, "Rendering/TextureScaling", "");
    QString fragmentShader = value(fcmat, "Rendering/FragmentShader", "");
    QString vertexShader = value(fcmat, "Rendering/VertexShader", "");

    // Defined by the Render WB
    QString aDiffuse = value(fcmat, "Architectural/DiffuseColor", "");
    QString aTransparency = value(fcmat, "Architectural/Transparency", "");

    if (!aDiffuse.isEmpty()) {
        diffuseColor = aDiffuse;
    }
    if (!aTransparency.isEmpty()) {
        transparency = aTransparency;
    }

    // Check which model we need
    bool useTexture = false;
    bool useAdvanced = false;
    bool useBasic = false;
    if (texturePath.length() + textureScaling.length() > 0) {
        useTexture = true;
    }
    if (fragmentShader.length() + vertexShader.length() > 0) {
        useAdvanced = true;
    }
    if (ambientColor.length() + diffuseColor.length() + emissiveColor.length() + shininess.length()
            + specularColor.length() + transparency.length()
        > 0) {
        useBasic = true;
    }

    if (useAdvanced) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Rendering_Advanced);
    }
    else if (useTexture) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Rendering_Texture);
    }
    else if (useBasic) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Rendering_Basic);
    }

    // Now add the data
    setAppearanceValue(finalModel, "AmbientColor", ambientColor);
    setAppearanceValue(finalModel, "DiffuseColor", diffuseColor);
    setAppearanceValue(finalModel, "EmissiveColor", emissiveColor);
    setAppearanceValue(finalModel, "Shininess", shininess);
    setAppearanceValue(finalModel, "SpecularColor", specularColor);
    setAppearanceValue(finalModel, "Transparency", transparency);
    setAppearanceValue(finalModel, "TexturePath", texturePath);
    setAppearanceValue(finalModel, "TextureScaling", textureScaling);
    setAppearanceValue(finalModel, "FragmentShader", fragmentShader);
    setAppearanceValue(finalModel, "VertexShader", vertexShader);
}

QString MaterialConfigLoader::multiLineKey(QMap<QString, QString>& fcmat, const QString& prefix)
{
    // fcmat.beginGroup(QStringLiteral("Render"));
    QString multiLineString;
    auto keys = fcmat.keys();
    for (const auto& key : keys) {
        if (key.startsWith(prefix) || key.startsWith(QStringLiteral("Render/") + prefix)) {
            QString string = value(fcmat, key.toStdString(), "");
            if (multiLineString.isEmpty()) {
                multiLineString += string;
            }
            else {
                multiLineString += QStringLiteral("\n") + string;
            }
        }
    }
    // fcmat.endGroup();

    return multiLineString;
}

void MaterialConfigLoader::addRenderAppleseed(QMap<QString, QString>& fcmat,
                                              const std::shared_ptr<Material>& finalModel)
{
    QString prefix = QStringLiteral("Render.Appleseed");
    QString string = multiLineKey(fcmat, prefix);

    if (!string.isEmpty()) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Render_Appleseed);

        // Now add the data
        setAppearanceValue(finalModel, "Render.Appleseed", string);
    }
}

void MaterialConfigLoader::addRenderCarpaint(QMap<QString, QString>& fcmat,
                                             const std::shared_ptr<Material>& finalModel)
{
    QString renderBaseColorValue = value(fcmat, "Render/Render.Carpaint.BaseColor", "");
    QString renderBump = value(fcmat, "Render/Render.Carpaint.Bump", "");
    QString renderDisplacement = value(fcmat, "Render/Render.Carpaint.Displacement", "");
    QString renderNormal = value(fcmat, "Render/Render.Carpaint.Normal", "");

    // Split out the textures
    QString renderBaseColor;
    QString renderBaseColorTexture;
    QString renderBaseColorObject;
    splitTextureObject(renderBaseColorValue,
                       &renderBaseColorTexture,
                       &renderBaseColor,
                       &renderBaseColorObject);

    if (!renderBaseColorValue.isEmpty() || !renderBump.isEmpty() || !renderDisplacement.isEmpty()
        || !renderNormal.isEmpty()) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Render_Carpaint);

        // Now add the data
        setAppearanceValue(finalModel, "Render.Carpaint.BaseColor", renderBaseColor);
        setAppearanceValue(finalModel, "Render.Carpaint.BaseColor.Texture", renderBaseColorTexture);
        setAppearanceValue(finalModel, "Render.Carpaint.BaseColor.Object", renderBaseColorObject);
        setAppearanceValue(finalModel, "Render.Carpaint.Bump", renderBump);
        setAppearanceValue(finalModel, "Render.Carpaint.Displacement", renderDisplacement);
        setAppearanceValue(finalModel, "Render.Carpaint.Normal", renderNormal);
    }
}

void MaterialConfigLoader::addRenderCycles(QMap<QString, QString>& fcmat,
                                           const std::shared_ptr<Material>& finalModel)
{
    QString prefix = QStringLiteral("Render.Cycles");
    QString string = multiLineKey(fcmat, prefix);
    if (!string.isEmpty()) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Render_Cycles);

        // Now add the data
        setAppearanceValue(finalModel, "Render.Cycles", string);
    }
}

void MaterialConfigLoader::addRenderDiffuse(QMap<QString, QString>& fcmat,
                                            const std::shared_ptr<Material>& finalModel)
{
    QString renderBump = value(fcmat, "Render/Render.Diffuse.Bump", "");
    QString renderColorValue = value(fcmat, "Render/Render.Diffuse.Color", "");
    QString renderDisplacement = value(fcmat, "Render/Render.Diffuse.Displacement", "");
    QString renderNormal = value(fcmat, "Render/Render.Diffuse.Normal", "");

    // Split out the textures
    QString renderColor;
    QString renderColorTexture;
    QString renderColorObject;
    splitTextureObject(renderColorValue, &renderColorTexture, &renderColor, &renderColorObject);

    if (!renderBump.isEmpty() || !renderColorValue.isEmpty() || !renderDisplacement.isEmpty()
        || !renderNormal.isEmpty()) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Render_Diffuse);

        // Now add the data
        setAppearanceValue(finalModel, "Render.Diffuse.Bump", renderBump);
        setAppearanceValue(finalModel, "Render.Diffuse.Color", renderColor);
        setAppearanceValue(finalModel, "Render.Diffuse.Color.Texture", renderColorTexture);
        setAppearanceValue(finalModel, "Render.Diffuse.Color.Object", renderColorObject);
        setAppearanceValue(finalModel, "Render.Diffuse.Displacement", renderDisplacement);
        setAppearanceValue(finalModel, "Render.Diffuse.Normal", renderNormal);
    }
}

void MaterialConfigLoader::addRenderDisney(QMap<QString, QString>& fcmat,
                                           const std::shared_ptr<Material>& finalModel)
{
    QString renderAnisotropicValue = value(fcmat, "Render/Render.Disney.Anisotropic", "");
    QString renderBaseColorValue = value(fcmat, "Render/Render.Disney.BaseColor", "");
    QString renderBump = value(fcmat, "Render/Render.Disney.Bump", "");
    QString renderClearCoatValue = value(fcmat, "Render/Render.Disney.ClearCoat", "");
    QString renderClearCoatGlossValue = value(fcmat, "Render/Render.Disney.ClearCoatGloss", "");
    QString renderDisplacement = value(fcmat, "Render/Render.Disney.Displacement", "");
    QString renderMetallicValue = value(fcmat, "Render/Render.Disney.Metallic", "");
    QString renderNormal = value(fcmat, "Render/Render.Disney.Normal", "");
    QString renderRoughnessValue = value(fcmat, "Render/Render.Disney.Roughness", "");
    QString renderSheenValue = value(fcmat, "Render/Render.Disney.Sheen", "");
    QString renderSheenTintValue = value(fcmat, "Render/Render.Disney.SheenTint", "");
    QString renderSpecularValue = value(fcmat, "Render/Render.Disney.Specular", "");
    QString renderSpecularTintValue = value(fcmat, "Render/Render.Disney.SpecularTint", "");
    QString renderSubsurfaceValue = value(fcmat, "Render/Render.Disney.Subsurface", "");

    // Split out the textures
    QString renderAnisotropic;
    QString renderAnisotropicTexture;
    splitTexture(renderAnisotropicValue, &renderAnisotropicTexture, &renderAnisotropic);
    QString renderBaseColor;
    QString renderBaseColorTexture;
    QString renderBaseColorObject;
    splitTextureObject(renderBaseColorValue,
                       &renderBaseColorTexture,
                       &renderBaseColor,
                       &renderBaseColorObject);
    QString renderClearCoat;
    QString renderClearCoatTexture;
    QString renderClearCoatObject;
    splitTextureObject(renderClearCoatValue,
                       &renderClearCoatTexture,
                       &renderClearCoat,
                       &renderClearCoatObject);
    QString renderClearCoatGloss;
    QString renderClearCoatGlossTexture;
    QString renderClearCoatGlossObject;
    splitTextureObject(renderClearCoatGlossValue,
                       &renderClearCoatGlossTexture,
                       &renderClearCoatGloss,
                       &renderClearCoatGlossObject);
    QString renderMetallic;
    QString renderMetallicTexture;
    splitTexture(renderMetallicValue, &renderMetallicTexture, &renderMetallic);
    QString renderRoughness;
    QString renderRoughnessTexture;
    splitTexture(renderRoughnessValue, &renderRoughnessTexture, &renderRoughness);
    QString renderSheen;
    QString renderSheenTexture;
    splitTexture(renderSheenValue, &renderSheenTexture, &renderSheen);
    QString renderSheenTint;
    QString renderSheenTintTexture;
    splitTexture(renderSheenTintValue, &renderSheenTintTexture, &renderSheenTint);
    QString renderSpecular;
    QString renderSpecularTexture;
    QString renderSpecularObject;
    splitTextureObject(renderSpecularValue,
                       &renderSpecularTexture,
                       &renderSpecular,
                       &renderSpecularObject);
    QString renderSpecularTint;
    QString renderSpecularTintTexture;
    QString renderSpecularTintObject;
    splitTextureObject(renderSpecularTintValue,
                       &renderSpecularTintTexture,
                       &renderSpecularTint,
                       &renderSpecularTintObject);
    QString renderSubsurface;
    QString renderSubsurfaceTexture;
    splitTexture(renderSubsurfaceValue, &renderSubsurfaceTexture, &renderSubsurface);

    if (!renderAnisotropicValue.isEmpty() || !renderBaseColorValue.isEmpty()
        || !renderBump.isEmpty() || !renderClearCoatValue.isEmpty()
        || !renderClearCoatGlossValue.isEmpty() || !renderDisplacement.isEmpty()
        || !renderMetallicValue.isEmpty() || !renderNormal.isEmpty()
        || !renderRoughnessValue.isEmpty() || !renderSheenValue.isEmpty()
        || !renderSheenTintValue.isEmpty() || !renderSpecularValue.isEmpty()
        || !renderSpecularTintValue.isEmpty() || !renderSubsurfaceValue.isEmpty()) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Render_Disney);

        // Now add the data
        setAppearanceValue(finalModel, "Render.Disney.Anisotropic", renderAnisotropic);
        setAppearanceValue(finalModel,
                           "Render.Disney.Anisotropic.Texture",
                           renderAnisotropicTexture);
        setAppearanceValue(finalModel, "Render.Disney.BaseColor", renderBaseColor);
        setAppearanceValue(finalModel, "Render.Disney.BaseColor.Texture", renderBaseColorTexture);
        setAppearanceValue(finalModel, "Render.Disney.Bump", renderBump);
        setAppearanceValue(finalModel, "Render.Disney.ClearCoat", renderClearCoat);
        setAppearanceValue(finalModel, "Render.Disney.ClearCoat.Texture", renderClearCoatTexture);
        setAppearanceValue(finalModel, "Render.Disney.ClearCoatGloss", renderClearCoatGloss);
        setAppearanceValue(finalModel,
                           "Render.Disney.ClearCoatGloss.Texture",
                           renderClearCoatGlossTexture);
        setAppearanceValue(finalModel, "Render.Disney.Displacement", renderDisplacement);
        setAppearanceValue(finalModel, "Render.Disney.Metallic", renderMetallic);
        setAppearanceValue(finalModel, "Render.Disney.Metallic.Texture", renderMetallicTexture);
        setAppearanceValue(finalModel, "Render.Disney.Normal", renderNormal);
        setAppearanceValue(finalModel, "Render.Disney.Roughness", renderRoughness);
        setAppearanceValue(finalModel, "Render.Disney.Roughness.Texture", renderRoughnessTexture);
        setAppearanceValue(finalModel, "Render.Disney.Sheen", renderSheen);
        setAppearanceValue(finalModel, "Render.Disney.Sheen.Texture", renderSheenTexture);
        setAppearanceValue(finalModel, "Render.Disney.SheenTint", renderSheenTint);
        setAppearanceValue(finalModel, "Render.Disney.SheenTint.Texture", renderSheenTintTexture);
        setAppearanceValue(finalModel, "Render.Disney.Specular", renderSpecular);
        setAppearanceValue(finalModel, "Render.Disney.Specular.Texture", renderSpecularTexture);
        setAppearanceValue(finalModel, "Render.Disney.SpecularTint", renderSpecularTint);
        setAppearanceValue(finalModel,
                           "Render.Disney.SpecularTint.Texture",
                           renderSpecularTintTexture);
        setAppearanceValue(finalModel, "Render.Disney.Subsurface", renderSubsurface);
        setAppearanceValue(finalModel, "Render.Disney.Subsurface.Texture", renderSubsurfaceTexture);
    }
}

void MaterialConfigLoader::addRenderEmission(QMap<QString, QString>& fcmat,
                                             const std::shared_ptr<Material>& finalModel)
{
    QString renderBump = value(fcmat, "Render/Render.Emission.Bump", "");
    QString renderColorValue = value(fcmat, "Render/Render.Emission.Color", "");
    QString renderNormal = value(fcmat, "Render/Render.Emission.Normal", "");
    QString renderPowerValue = value(fcmat, "Render/Render.Emission.Power", "");

    // Split out the textures
    QString renderColor;
    QString renderColorTexture;
    QString renderColorObject;
    splitTextureObject(renderColorValue, &renderColorTexture, &renderColor, &renderColorObject);
    QString renderPower;
    QString renderPowerTexture;
    splitTexture(renderPowerValue, &renderPowerTexture, &renderPower);

    if (!renderColorValue.isEmpty() || !renderBump.isEmpty() || !renderPowerValue.isEmpty()
        || !renderNormal.isEmpty()) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Render_Emission);

        // Now add the data
        setAppearanceValue(finalModel, "Render.Emission.Bump", renderBump);
        setAppearanceValue(finalModel, "Render.Emission.Color", renderColor);
        setAppearanceValue(finalModel, "Render.Emission.Color.Texture", renderColorTexture);
        setAppearanceValue(finalModel, "Render.Emission.Color.Object", renderColorObject);
        setAppearanceValue(finalModel, "Render.Emission.Normal", renderNormal);
        setAppearanceValue(finalModel, "Render.Emission.Power", renderPower);
        setAppearanceValue(finalModel, "Render.Emission.Power.Texture", renderPowerTexture);
    }
}

void MaterialConfigLoader::addRenderGlass(QMap<QString, QString>& fcmat,
                                          const std::shared_ptr<Material>& finalModel)
{
    QString renderBump = value(fcmat, "Render/Render.Glass.Bump", "");
    QString renderColorValue = value(fcmat, "Render/Render.Glass.Color", "");
    QString renderIORValue = value(fcmat, "Render/Render.Glass.IOR", "");
    QString renderDisplacement = value(fcmat, "Render/Render.Glass.Displacement", "");
    QString renderNormal = value(fcmat, "Render/Render.Glass.Normal", "");

    // Split out the textures
    QString renderColor;
    QString renderColorTexture;
    QString renderColorObject;
    splitTextureObject(renderColorValue, &renderColorTexture, &renderColor, &renderColorObject);
    QString renderIOR;
    QString renderIORTexture;
    splitTexture(renderIORValue, &renderIORTexture, &renderIOR);

    if (!renderBump.isEmpty() || !renderColorValue.isEmpty() || !renderIORValue.isEmpty()
        || !renderDisplacement.isEmpty() || !renderNormal.isEmpty()) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Render_Glass);

        setAppearanceValue(finalModel, "Render.Glass.Bump", renderBump);
        setAppearanceValue(finalModel, "Render.Glass.Color", renderColor);
        setAppearanceValue(finalModel, "Render.Glass.Color.Texture", renderColorTexture);
        setAppearanceValue(finalModel, "Render.Glass.Color.Object", renderColorObject);
        setAppearanceValue(finalModel, "Render.Glass.IOR", renderIOR);
        setAppearanceValue(finalModel, "Render.Glass.IOR.Texture", renderIORTexture);
        setAppearanceValue(finalModel, "Render.Glass.Displacement", renderDisplacement);
        setAppearanceValue(finalModel, "Render.Glass.Normal", renderNormal);
    }
}

void MaterialConfigLoader::addRenderLuxcore(QMap<QString, QString>& fcmat,
                                            const std::shared_ptr<Material>& finalModel)
{
    QString prefix = QStringLiteral("Render.Luxcore");
    QString string = multiLineKey(fcmat, prefix);

    if (!string.isEmpty()) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Render_Luxcore);

        // Now add the data
        setAppearanceValue(finalModel, "Render.Luxcore", string);
    }
}

void MaterialConfigLoader::addRenderLuxrender(QMap<QString, QString>& fcmat,
                                              const std::shared_ptr<Material>& finalModel)
{
    QString prefix = QStringLiteral("Render.Luxrender");
    QString string = multiLineKey(fcmat, prefix);

    if (!string.isEmpty()) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Render_Luxrender);

        // Now add the data
        setAppearanceValue(finalModel, "Render.Luxrender", string);
    }
}

void MaterialConfigLoader::addRenderMixed(QMap<QString, QString>& fcmat,
                                          const std::shared_ptr<Material>& finalModel)
{
    QString renderBump = value(fcmat, "Render/Render.Mixed.Bump", "");
    QString renderDiffuseColorValue = value(fcmat, "Render/Render.Mixed.Diffuse.Color", "");
    QString renderDisplacement = value(fcmat, "Render/Render.Mixed.Displacement", "");
    QString renderGlassColorValue = value(fcmat, "Render/Render.Mixed.Glass.Color", "");
    QString renderGlassIORValue = value(fcmat, "Render/Render.Mixed.Glass.IOR", "");
    QString renderNormal = value(fcmat, "Render/Render.Mixed.Normal", "");
    QString renderTransparencyValue = value(fcmat, "Render/Render.Mixed.Transparency", "");

    // Split out the textures
    QString renderDiffuseColor;
    QString renderDiffuseColorTexture;
    QString renderDiffuseColorObject;
    splitTextureObject(renderDiffuseColorValue,
                       &renderDiffuseColorTexture,
                       &renderDiffuseColor,
                       &renderDiffuseColorObject);
    QString renderGlassColor;
    QString renderGlassColorTexture;
    QString renderGlassColorObject;
    splitTextureObject(renderGlassColorValue,
                       &renderGlassColorTexture,
                       &renderGlassColor,
                       &renderGlassColorObject);
    QString renderGlassIOR;
    QString renderGlassIORTexture;
    splitTexture(renderGlassIORValue, &renderGlassIORTexture, &renderGlassIOR);
    QString renderTransparency;
    QString renderTransparencyTexture;
    splitTexture(renderTransparencyValue, &renderTransparencyTexture, &renderTransparency);

    if (!renderBump.isEmpty() || !renderDiffuseColorValue.isEmpty() || !renderDisplacement.isEmpty()
        || !renderGlassColorValue.isEmpty() || !renderGlassIORValue.isEmpty()
        || !renderNormal.isEmpty() || !renderTransparencyValue.isEmpty()) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Render_Mixed);

        // Now add the data
        setAppearanceValue(finalModel, "Render.Mixed.Bump", renderBump);
        setAppearanceValue(finalModel, "Render.Mixed.Diffuse.Color", renderDiffuseColor);
        setAppearanceValue(finalModel,
                           "Render.Mixed.Diffuse.Color.Texture",
                           renderDiffuseColorTexture);
        setAppearanceValue(finalModel,
                           "Render.Mixed.Diffuse.Color.Object",
                           renderDiffuseColorObject);
        setAppearanceValue(finalModel, "Render.Mixed.Displacement", renderDisplacement);
        setAppearanceValue(finalModel, "Render.Mixed.Glass.Color", renderGlassColor);
        setAppearanceValue(finalModel, "Render.Mixed.Glass.Color.Texture", renderGlassColorTexture);
        setAppearanceValue(finalModel, "Render.Mixed.Glass.Color.Object", renderGlassColorObject);
        setAppearanceValue(finalModel, "Render.Mixed.Glass.IOR", renderGlassIOR);
        setAppearanceValue(finalModel, "Render.Mixed.Glass.IOR.Texture", renderGlassIORTexture);
        setAppearanceValue(finalModel, "Render.Mixed.Normal", renderNormal);
        setAppearanceValue(finalModel, "Render.Mixed.Transparency", renderTransparency);
        setAppearanceValue(finalModel,
                           "Render.Mixed.Transparency.Texture",
                           renderTransparencyTexture);
    }
}

void MaterialConfigLoader::addRenderOspray(QMap<QString, QString>& fcmat,
                                           const std::shared_ptr<Material>& finalModel)
{
    QString prefix = QStringLiteral("Render.Ospray");
    QString string = multiLineKey(fcmat, prefix);

    if (!string.isEmpty()) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Render_Ospray);

        // Now add the data
        setAppearanceValue(finalModel, "Render.Ospray", string);
    }
}

void MaterialConfigLoader::addRenderPbrt(QMap<QString, QString>& fcmat,
                                         const std::shared_ptr<Material>& finalModel)
{
    QString prefix = QStringLiteral("Render.Pbrt");
    QString string = multiLineKey(fcmat, prefix);

    if (!string.isEmpty()) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Render_Pbrt);

        // Now add the data
        setAppearanceValue(finalModel, "Render.Pbrt", string);
    }
}

void MaterialConfigLoader::addRenderPovray(QMap<QString, QString>& fcmat,
                                           const std::shared_ptr<Material>& finalModel)
{
    QString prefix = QStringLiteral("Render.Povray");
    QString string = multiLineKey(fcmat, prefix);

    if (!string.isEmpty()) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Render_Povray);

        // Now add the data
        setAppearanceValue(finalModel, "Render.Povray", string);
    }
}

void MaterialConfigLoader::addRenderSubstancePBR(QMap<QString, QString>& fcmat,
                                                 const std::shared_ptr<Material>& finalModel)
{
    QString renderBaseColorValue = value(fcmat, "Render/Render.Substance_PBR.BaseColor", "");
    QString renderBump = value(fcmat, "Render/Render.Substance_PBR.Bump", "");
    QString renderMetallicValue = value(fcmat, "Render/Render.Substance_PBR.Metallic", "");
    QString renderNormal = value(fcmat, "Render/Render.Substance_PBR.Normal", "");
    QString renderRoughnessValue = value(fcmat, "Render/Render.Substance_PBR.Roughness", "");
    QString renderSpecularValue = value(fcmat, "Render/Render.Substance_PBR.Specular", "");

    // Split out the textures
    QString renderBaseColor;
    QString renderBaseColorTexture;
    QString renderBaseColorObject;
    splitTextureObject(renderBaseColorValue,
                       &renderBaseColorTexture,
                       &renderBaseColor,
                       &renderBaseColorObject);
    QString renderMetallic;
    QString renderMetallicTexture;
    splitTexture(renderMetallicValue, &renderMetallicTexture, &renderMetallic);
    QString renderRoughness;
    QString renderRoughnessTexture;
    splitTexture(renderRoughnessValue, &renderRoughnessTexture, &renderRoughness);
    QString renderSpecular;
    QString renderSpecularTexture;
    splitTexture(renderSpecularValue, &renderSpecularTexture, &renderSpecular);

    if (!renderBaseColorValue.isEmpty() || !renderBump.isEmpty() || !renderMetallicValue.isEmpty()
        || !renderNormal.isEmpty() || !renderRoughnessValue.isEmpty()
        || !renderSpecularValue.isEmpty()) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Render_SubstancePBR);

        // Now add the data
        setAppearanceValue(finalModel, "Render.Substance_PBR.BaseColor", renderBaseColor);
        setAppearanceValue(finalModel,
                           "Render.Substance_PBR.BaseColor.Texture",
                           renderBaseColorTexture);
        setAppearanceValue(finalModel,
                           "Render.Substance_PBR.BaseColor.Object",
                           renderBaseColorObject);
        setAppearanceValue(finalModel, "Render.Substance_PBR.Bump", renderBump);
        setAppearanceValue(finalModel, "Render.Substance_PBR.Metallic", renderMetallic);
        setAppearanceValue(finalModel,
                           "Render.Substance_PBR.Metallic.Texture",
                           renderMetallicTexture);
        setAppearanceValue(finalModel, "Render.Substance_PBR.Normal", renderNormal);
        setAppearanceValue(finalModel, "Render.Substance_PBR.Roughness", renderRoughness);
        setAppearanceValue(finalModel,
                           "Render.Substance_PBR.Roughness.Texture",
                           renderRoughnessTexture);
        setAppearanceValue(finalModel, "Render.Substance_PBR.Specular", renderSpecular);
        setAppearanceValue(finalModel,
                           "Render.Substance_PBR.Specular.Texture",
                           renderSpecularTexture);
    }
}

void MaterialConfigLoader::addRenderTexture(QMap<QString, QString>& fcmat,
                                            const std::shared_ptr<Material>& finalModel)
{
    QString renderName;
    auto renderImage = std::make_shared<QList<QVariant>>();
    QString renderScale;
    QString renderRotation;
    QString renderTranslationU;
    QString renderTranslationV;

    auto keys = fcmat.keys();
    for (const auto& key : keys) {
        if (key.startsWith(QStringLiteral("Render/Render.Textures."))) {
            QStringList list1 = key.split(QLatin1Char('.'));
            if (renderName.isEmpty()) {
                renderName = list1[2];
            }
            if (list1[3] == QStringLiteral("Images")) {
                renderImage->push_back(value(fcmat, key.toStdString(), ""));
            }
            else if (list1[3] == QStringLiteral("Scale")) {
                renderScale = value(fcmat, key.toStdString(), "");
            }
            else if (list1[3] == QStringLiteral("Rotation")) {
                renderRotation = value(fcmat, key.toStdString(), "");
            }
            else if (list1[3] == QStringLiteral("TranslationU")) {
                renderTranslationU = value(fcmat, key.toStdString(), "");
            }
            else if (list1[3] == QStringLiteral(" TranslationV")) {
                renderTranslationV = value(fcmat, key.toStdString(), "");
            }
        }
    }

    if (!renderName.isEmpty()) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Render_Texture);

        // Now add the data
        setAppearanceValue(finalModel, "Render.Textures.Name", renderName);
        setAppearanceValue(finalModel, "Render.Textures.Images", renderImage);
        setAppearanceValue(finalModel, "Render.Textures.Scale", renderScale);
        setAppearanceValue(finalModel, "Render.Textures.Rotation", renderRotation);
        setAppearanceValue(finalModel, "Render.Textures.TranslationU", renderTranslationU);
        setAppearanceValue(finalModel, "Render.Textures.TranslationV", renderTranslationV);
    }
}

void MaterialConfigLoader::addRenderWB(QMap<QString, QString>& fcmat,
                                       const std::shared_ptr<Material>& finalModel)
{
    QString useObjectColor = value(fcmat, "General/UseObjectColor", "");
    QString renderType = value(fcmat, "Render/Render.Type", "");

    if (!renderType.isEmpty()) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_RenderWB);

        // Now add the data
        setAppearanceValue(finalModel, "UseObjectColor", useObjectColor);
        setAppearanceValue(finalModel, "Render.Type", renderType);
    }

    addRenderAppleseed(fcmat, finalModel);
    addRenderCarpaint(fcmat, finalModel);
    addRenderCycles(fcmat, finalModel);
    addRenderDiffuse(fcmat, finalModel);
    addRenderDisney(fcmat, finalModel);
    addRenderEmission(fcmat, finalModel);
    addRenderGlass(fcmat, finalModel);
    addRenderLuxcore(fcmat, finalModel);
    addRenderLuxrender(fcmat, finalModel);
    addRenderMixed(fcmat, finalModel);
    addRenderOspray(fcmat, finalModel);
    addRenderPbrt(fcmat, finalModel);
    addRenderPovray(fcmat, finalModel);
    addRenderSubstancePBR(fcmat, finalModel);
    addRenderTexture(fcmat, finalModel);
}

void MaterialConfigLoader::addCosts(const QMap<QString, QString>& fcmat,
                                    const std::shared_ptr<Material>& finalModel)
{
    QString productURL = value(fcmat, "Cost/ProductURL", "");
    QString specificPrice = value(fcmat, "Cost/SpecificPrice", "");
    QString vendor = value(fcmat, "Cost/Vendor", "");

    if (productURL.length() + specificPrice.length() + vendor.length() > 0) {
        finalModel->addPhysical(ModelUUIDs::ModelUUID_Costs_Default);

        // Now add the data
        setPhysicalValue(finalModel, "ProductURL", productURL);
        setPhysicalValue(finalModel, "SpecificPrice", specificPrice);
        setPhysicalValue(finalModel, "Vendor", vendor);
    }
}

void MaterialConfigLoader::addArchitectural(const QMap<QString, QString>& fcmat,
                                            const std::shared_ptr<Material>& finalModel)
{
    QString color = value(fcmat, "Architectural/Color", "");
    QString environmentalEfficiencyClass =
        value(fcmat, "Architectural/EnvironmentalEfficiencyClass", "");
    QString executionInstructions = value(fcmat, "Architectural/ExecutionInstructions", "");
    QString finish = value(fcmat, "Architectural/Finish", "");
    QString fireResistanceClass = value(fcmat, "Architectural/FireResistanceClass", "");
    QString model = value(fcmat, "Architectural/Model", "");
    QString soundTransmissionClass = value(fcmat, "Architectural/SoundTransmissionClass", "");
    QString unitsPerQuantity = value(fcmat, "Architectural/UnitsPerQuantity", "");

    if (environmentalEfficiencyClass.length() + executionInstructions.length()
            + fireResistanceClass.length() + model.length() + soundTransmissionClass.length()
            + unitsPerQuantity.length()
        > 0) {
        finalModel->addPhysical(ModelUUIDs::ModelUUID_Architectural_Default);
    }
    if (color.length() + finish.length() > 0) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Rendering_Architectural);
    }

    // Now add the data
    setPhysicalValue(finalModel, "EnvironmentalEfficiencyClass", environmentalEfficiencyClass);
    setPhysicalValue(finalModel, "ExecutionInstructions", executionInstructions);
    setPhysicalValue(finalModel, "FireResistanceClass", fireResistanceClass);
    setPhysicalValue(finalModel, "Model", model);
    setPhysicalValue(finalModel, "SoundTransmissionClass", soundTransmissionClass);
    setPhysicalValue(finalModel, "UnitsPerQuantity", unitsPerQuantity);

    setAppearanceValue(finalModel, "Color", color);
    setAppearanceValue(finalModel, "Finish", finish);
}

void MaterialConfigLoader::addElectromagnetic(const QMap<QString, QString>& fcmat,
                                              const std::shared_ptr<Material>& finalModel)
{
    QString relativePermittivity = value(fcmat, "Electromagnetic/RelativePermittivity", "");
    QString electricalConductivity = value(fcmat, "Electromagnetic/ElectricalConductivity", "");
    QString relativePermeability = value(fcmat, "Electromagnetic/RelativePermeability", "");

    if (relativePermittivity.length() + electricalConductivity.length()
            + relativePermeability.length()
        > 0) {
        finalModel->addPhysical(ModelUUIDs::ModelUUID_Electromagnetic_Default);

        // Now add the data
        setPhysicalValue(finalModel, "RelativePermittivity", relativePermittivity);
        setPhysicalValue(finalModel, "ElectricalConductivity", electricalConductivity);
        setPhysicalValue(finalModel, "RelativePermeability", relativePermeability);
    }
}

void MaterialConfigLoader::addThermal(const QMap<QString, QString>& fcmat,
                                      const std::shared_ptr<Material>& finalModel)
{
    QString specificHeat = value(fcmat, "Thermal/SpecificHeat", "");
    QString thermalConductivity = value(fcmat, "Thermal/ThermalConductivity", "");
    QString thermalExpansionCoefficient = value(fcmat, "Thermal/ThermalExpansionCoefficient", "");

    if (specificHeat.length() + thermalConductivity.length() + thermalExpansionCoefficient.length()
        > 0) {
        finalModel->addPhysical(ModelUUIDs::ModelUUID_Thermal_Default);

        // Now add the data
        setPhysicalValue(finalModel, "SpecificHeat", specificHeat);
        setPhysicalValue(finalModel, "ThermalConductivity", thermalConductivity);
        setPhysicalValue(finalModel, "ThermalExpansionCoefficient", thermalExpansionCoefficient);
    }
}

void MaterialConfigLoader::addFluid(const QMap<QString, QString>& fcmat,
                                    const std::shared_ptr<Material>& finalModel)
{
    QString density = value(fcmat, "Fluidic/Density", "");
    QString dynamicViscosity = value(fcmat, "Fluidic/DynamicViscosity", "");
    QString kinematicViscosity = value(fcmat, "Fluidic/KinematicViscosity", "");
    QString prandtlNumber = value(fcmat, "Fluidic/PrandtlNumber", "");

    // Check which model we need
    bool useDensity = false;
    bool useFluid = false;
    if (density.length() > 0) {
        useDensity = true;
    }
    if (dynamicViscosity.length() + kinematicViscosity.length() + prandtlNumber.length() > 0) {
        useFluid = true;
    }

    if (useFluid) {
        finalModel->addPhysical(ModelUUIDs::ModelUUID_Fluid_Default);
    }
    else if (useDensity) {
        finalModel->addPhysical(ModelUUIDs::ModelUUID_Mechanical_Density);
    }

    // Now add the data
    setPhysicalValue(finalModel, "Density", density);
    setPhysicalValue(finalModel, "DynamicViscosity", dynamicViscosity);
    setPhysicalValue(finalModel, "KinematicViscosity", kinematicViscosity);
    setPhysicalValue(finalModel, "PrandtlNumber", prandtlNumber);
}

void MaterialConfigLoader::addMechanical(const QMap<QString, QString>& fcmat,
                                         const std::shared_ptr<Material>& finalModel)
{
    QString density = value(fcmat, "Mechanical/Density", "");
    QString bulkModulus = value(fcmat, "Mechanical/BulkModulus", "");
    QString poissonRatio = value(fcmat, "Mechanical/PoissonRatio", "");
    QString shearModulus = value(fcmat, "Mechanical/ShearModulus", "");
    QString youngsModulus = value(fcmat, "Mechanical/YoungsModulus", "");
    QString angleOfFriction = value(fcmat, "Mechanical/AngleOfFriction", "");
    QString compressiveStrength = value(fcmat, "Mechanical/CompressiveStrength", "");
    QString fractureToughness = value(fcmat, "Mechanical/FractureToughness", "");
    QString ultimateStrain = value(fcmat, "Mechanical/UltimateStrain", "");
    QString ultimateTensileStrength = value(fcmat, "Mechanical/UltimateTensileStrength", "");
    QString yieldStrength = value(fcmat, "Mechanical/YieldStrength", "");
    QString stiffness = value(fcmat, "Mechanical/Stiffness", "");

    // Check which model we need
    bool useDensity = false;
    bool useIso = false;
    bool useLinearElastic = false;
    if (density.length() > 0) {
        useDensity = true;
    }
    if (bulkModulus.length() + poissonRatio.length() + shearModulus.length()
            + youngsModulus.length()
        > 0) {
        useIso = true;
    }
    if (angleOfFriction.length() + compressiveStrength.length() + fractureToughness.length()
            + ultimateStrain.length() + ultimateTensileStrength.length() + yieldStrength.length()
            + stiffness.length()
        > 0) {
        useLinearElastic = true;
    }

    if (useLinearElastic) {
        finalModel->addPhysical(ModelUUIDs::ModelUUID_Mechanical_LinearElastic);
    }
    else {
        if (useIso) {
            finalModel->addPhysical(ModelUUIDs::ModelUUID_Mechanical_IsotropicLinearElastic);
        }
        if (useDensity) {
            finalModel->addPhysical(ModelUUIDs::ModelUUID_Mechanical_Density);
        }
    }

    // Now add the data
    setPhysicalValue(finalModel, "Density", density);
    setPhysicalValue(finalModel, "BulkModulus", bulkModulus);
    setPhysicalValue(finalModel, "PoissonRatio", poissonRatio);
    setPhysicalValue(finalModel, "ShearModulus", shearModulus);
    setPhysicalValue(finalModel, "YoungsModulus", youngsModulus);
    setPhysicalValue(finalModel, "AngleOfFriction", angleOfFriction);
    setPhysicalValue(finalModel, "CompressiveStrength", compressiveStrength);
    setPhysicalValue(finalModel, "FractureToughness", fractureToughness);
    setPhysicalValue(finalModel, "UltimateStrain", ultimateStrain);
    setPhysicalValue(finalModel, "UltimateTensileStrength", ultimateTensileStrength);
    setPhysicalValue(finalModel, "YieldStrength", yieldStrength);
    setPhysicalValue(finalModel, "Stiffness", stiffness);
}

void MaterialConfigLoader::addLegacy(const QMap<QString, QString>& fcmat,
                                     const std::shared_ptr<Material>& finalModel)
{
    for (auto const& legacy : fcmat.keys()) {
        auto name = legacy;
        int last = name.lastIndexOf(QStringLiteral("/"));
        if (last > 0) {
            name = name.mid(last + 1);
        }

        if (!finalModel->hasNonLegacyProperty(name)) {
            setLegacyValue(finalModel, name.toStdString(), fcmat[legacy]);
        }
    }
}

std::shared_ptr<Material>
MaterialConfigLoader::getMaterialFromPath(const std::shared_ptr<MaterialLibraryLocal>& library,
                                          const QString& path)
{
    QString author = getAuthorAndLicense(path);  // Place them both in the author field

    QMap<QString, QString> fcmat;
    if (!readFile(path, fcmat)) {
        Base::Console().log("Error reading '%s'\n", path.toStdString().c_str());
        throw MaterialReadError();
    }

    // General section
    // QString name = value(fcmat, "Name", ""); - always get the name from the filename
    QFileInfo filepath(path);
    QString name =
        filepath.fileName().remove(QStringLiteral(".FCMat"), Qt::CaseInsensitive);
    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);

    QString description = value(fcmat, "Description", "");
    QString sourceReference = value(fcmat, "ReferenceSource", "");
    QString sourceURL = value(fcmat, "SourceURL", "");

    auto baseLibrary =
        reinterpret_cast<const std::shared_ptr<Materials::MaterialLibrary>&>(library);
    std::shared_ptr<Material> finalModel =
        std::make_shared<Material>(baseLibrary, path, uuid, name);
    finalModel->setOldFormat(true);

    finalModel->setAuthor(author);
    finalModel->setDescription(description);
    finalModel->setReference(sourceReference);
    finalModel->setURL(sourceURL);

    QString father = value(fcmat, "Father", "");
    if (!father.isEmpty()) {
        finalModel->addPhysical(ModelUUIDs::ModelUUID_Legacy_Father);

        // Now add the data
        setPhysicalValue(finalModel, "Father", father);
    }

    QString kindOfMaterial = value(fcmat, "KindOfMaterial", "");
    QString materialNumber = value(fcmat, "MaterialNumber", "");
    QString norm = value(fcmat, "Norm", "");
    QString standardCode = value(fcmat, "StandardCode", "");
    if (kindOfMaterial.length() + materialNumber.length() + norm.length() + standardCode.length()
        > 0) {
        finalModel->addPhysical(ModelUUIDs::ModelUUID_Legacy_MaterialStandard);

        // Now add the data
        setPhysicalValue(finalModel, "KindOfMaterial", kindOfMaterial);
        setPhysicalValue(finalModel, "MaterialNumber", materialNumber);
        setPhysicalValue(finalModel, "StandardCode", norm);  // Norm is the same as StandardCode
        setPhysicalValue(finalModel, "StandardCode", standardCode);
    }

    // Add the remaining sections
    addMechanical(fcmat, finalModel);
    addFluid(fcmat, finalModel);
    addThermal(fcmat, finalModel);
    addElectromagnetic(fcmat, finalModel);
    addArchitectural(fcmat, finalModel);
    addCosts(fcmat, finalModel);
    addRendering(fcmat, finalModel);
    addVectorRendering(fcmat, finalModel);
    addRenderWB(fcmat, finalModel);
    addLegacy(fcmat, finalModel);

    return finalModel;
}
