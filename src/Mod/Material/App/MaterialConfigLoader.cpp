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

#include <fstream>
#include <map>
#include <memory>
#include <string>

#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QSettings>
#include <QString>
#include <QTextStream>
#include <QUuid>



#include <App/Application.h>
#include <Base/Interpreter.h>


#include "Exceptions.h"
#include "MaterialConfigLoader.h"
#include "MaterialLoader.h"
#include "Library.h"
#include "MaterialLibrary.h"
#include "Model.h"
#include "ModelUuids.h"


using namespace Materials;

bool MaterialConfigLoader::isConfigStyle(const std::string& path)
{
    QSettings fcmat(QString::fromStdString(path), QSettings::IniFormat);

    // No [sections] means not .ini
    if (fcmat.childGroups().empty()) {
        return false;
    }

    // Sometimes arrays can create a false positive
    QFile infile(QString::fromStdString(path));
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

bool MaterialConfigLoader::readFile(const std::string& path,
                                   std::map<std::string, std::string>& map)
{
    // This function is necessary as the built in routines don't always return the full value string
    QFile infile(QString::fromStdString(path));
    if (infile.open(QIODevice::ReadOnly)) {
        QTextStream in(&infile);
        QString line;
        std::string prefix;
        while (!in.atEnd()) {
            line = in.readLine();
            if (line.trimmed().startsWith(QLatin1Char(';'))) {
                continue;
            }

            if (line.startsWith(QLatin1Char('['))) {
                // read prefix
                auto end = line.indexOf(QLatin1Char(']'));
                if (end > 1) {
                    prefix = line.mid(1, end - 1).toStdString() + "/";

                    // Render WB uses both Render and Rendering
                    if (prefix == "Rendering/") {
                        prefix = "Render/";
                    }
                }
            }
            else {
                auto separator = line.indexOf(QLatin1Char('='));
                if (separator > 2) {
                    auto left = line.mid(0, separator - 1);
                    auto right = line.mid(separator + 2);
                    map[prefix + left.toStdString()] = right.toStdString();
                }
            }
        }
        infile.close();
        return true;
    }

    return false;
}

void MaterialConfigLoader::splitTexture(const std::string& value,
                                        std::string* texture,
                                        std::string* remain)
{
    // Split Texture(...);(...) into its two pieces
    if (const auto separator = value.find(';'); separator != std::string::npos) {
        auto left = value.substr(0, separator);
        auto right = value.substr(separator + 1);
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

void MaterialConfigLoader::splitTextureObject(const std::string& value,
                                              std::string* texture,
                                              std::string* remain,
                                              std::string* object)
{
    splitTexture(value, texture, remain);
    if (*remain == "Object") {
        remain->clear();
        *object = "true";
    }
}

std::string MaterialConfigLoader::getAuthorAndLicense(const std::string& path)
{
    std::ifstream infile(path);
    const std::string noAuthor;

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
        return trim_copy(line.substr(found + 1));
    }

    return noAuthor;
}

void MaterialConfigLoader::addVectorRendering(const std::map<std::string, std::string>& fcmat,
                                              const std::shared_ptr<Material>& finalModel)
{
    std::string sectionFillPattern = value(fcmat, "VectorRendering/SectionFillPattern", "");
    std::string sectionLinewidth = value(fcmat, "VectorRendering/SectionLinewidth", "");
    std::string sectionColor = value(fcmat, "VectorRendering/SectionColor", "");
    std::string viewColor = value(fcmat, "VectorRendering/ViewColor", "");
    std::string viewFillPattern = value(fcmat, "VectorRendering/ViewFillPattern", "");
    std::string viewLinewidth = value(fcmat, "VectorRendering/ViewLinewidth", "");

    // Defined by the Render WB
    std::string aSection = value(fcmat, "Architectural/SectionColor", "");

    if (!aSection.empty()) {
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

void MaterialConfigLoader::addRendering(const std::map<std::string, std::string>& fcmat,
                                        const std::shared_ptr<Material>& finalModel)
{
    std::string ambientColor = value(fcmat, "Rendering/AmbientColor", "");
    std::string diffuseColor = value(fcmat, "Rendering/DiffuseColor", "");
    std::string emissiveColor = value(fcmat, "Rendering/EmissiveColor", "");
    std::string shininess = value(fcmat, "Rendering/Shininess", "");
    std::string specularColor = value(fcmat, "Rendering/SpecularColor", "");
    std::string transparency = value(fcmat, "Rendering/Transparency", "");
    std::string texturePath = value(fcmat, "Rendering/TexturePath", "");
    std::string textureScaling = value(fcmat, "Rendering/TextureScaling", "");
    std::string fragmentShader = value(fcmat, "Rendering/FragmentShader", "");
    std::string vertexShader = value(fcmat, "Rendering/VertexShader", "");

    // Defined by the Render WB
    std::string aDiffuse = value(fcmat, "Architectural/DiffuseColor", "");
    std::string aTransparency = value(fcmat, "Architectural/Transparency", "");

    if (!aDiffuse.empty()) {
        diffuseColor = aDiffuse;
    }
    if (!aTransparency.empty()) {
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

std::string MaterialConfigLoader::multiLineKey(std::map<std::string, std::string>& fcmat,
                                               const std::string& prefix)
{
    std::string multiLineString;
    for (const auto& entry : fcmat) {
        const auto& key = entry.first;
        if (key.starts_with(prefix) || key.starts_with("Render/" + prefix)) {
            if (!multiLineString.empty()) {
                multiLineString += '\n';
            }
            multiLineString += value(fcmat, key, "");
        }
    }

    return multiLineString;
}

void MaterialConfigLoader::addRenderAppleseed(std::map<std::string, std::string>& fcmat,
                                              const std::shared_ptr<Material>& finalModel)
{
    std::string prefix = "Render.Appleseed";
    std::string string = multiLineKey(fcmat, prefix);

    if (!string.empty()) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Render_Appleseed);

        // Now add the data
        setAppearanceValue(finalModel, "Render.Appleseed", string);
    }
}

void MaterialConfigLoader::addRenderCarpaint(std::map<std::string, std::string>& fcmat,
                                             const std::shared_ptr<Material>& finalModel)
{
    std::string renderBaseColorValue = value(fcmat, "Render/Render.Carpaint.BaseColor", "");
    std::string renderBump = value(fcmat, "Render/Render.Carpaint.Bump", "");
    std::string renderDisplacement = value(fcmat, "Render/Render.Carpaint.Displacement", "");
    std::string renderNormal = value(fcmat, "Render/Render.Carpaint.Normal", "");

    // Split out the textures
    std::string renderBaseColor;
    std::string renderBaseColorTexture;
    std::string renderBaseColorObject;
    splitTextureObject(renderBaseColorValue,
                       &renderBaseColorTexture,
                       &renderBaseColor,
                       &renderBaseColorObject);

    if (!renderBaseColorValue.empty() || !renderBump.empty() || !renderDisplacement.empty()
        || !renderNormal.empty()) {
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

void MaterialConfigLoader::addRenderCycles(std::map<std::string, std::string>& fcmat,
                                           const std::shared_ptr<Material>& finalModel)
{
    std::string prefix = "Render.Cycles";
    std::string string = multiLineKey(fcmat, prefix);
    if (!string.empty()) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Render_Cycles);

        // Now add the data
        setAppearanceValue(finalModel, "Render.Cycles", string);
    }
}

void MaterialConfigLoader::addRenderDiffuse(std::map<std::string, std::string>& fcmat,
                                            const std::shared_ptr<Material>& finalModel)
{
    std::string renderBump = value(fcmat, "Render/Render.Diffuse.Bump", "");
    std::string renderColorValue = value(fcmat, "Render/Render.Diffuse.Color", "");
    std::string renderDisplacement = value(fcmat, "Render/Render.Diffuse.Displacement", "");
    std::string renderNormal = value(fcmat, "Render/Render.Diffuse.Normal", "");

    // Split out the textures
    std::string renderColor;
    std::string renderColorTexture;
    std::string renderColorObject;
    splitTextureObject(renderColorValue, &renderColorTexture, &renderColor, &renderColorObject);

    if (!renderBump.empty() || !renderColorValue.empty() || !renderDisplacement.empty()
        || !renderNormal.empty()) {
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

void MaterialConfigLoader::addRenderDisney(std::map<std::string, std::string>& fcmat,
                                           const std::shared_ptr<Material>& finalModel)
{
    std::string renderAnisotropicValue = value(fcmat, "Render/Render.Disney.Anisotropic", "");
    std::string renderBaseColorValue = value(fcmat, "Render/Render.Disney.BaseColor", "");
    std::string renderBump = value(fcmat, "Render/Render.Disney.Bump", "");
    std::string renderClearCoatValue = value(fcmat, "Render/Render.Disney.ClearCoat", "");
    std::string renderClearCoatGlossValue = value(fcmat, "Render/Render.Disney.ClearCoatGloss", "");
    std::string renderDisplacement = value(fcmat, "Render/Render.Disney.Displacement", "");
    std::string renderMetallicValue = value(fcmat, "Render/Render.Disney.Metallic", "");
    std::string renderNormal = value(fcmat, "Render/Render.Disney.Normal", "");
    std::string renderRoughnessValue = value(fcmat, "Render/Render.Disney.Roughness", "");
    std::string renderSheenValue = value(fcmat, "Render/Render.Disney.Sheen", "");
    std::string renderSheenTintValue = value(fcmat, "Render/Render.Disney.SheenTint", "");
    std::string renderSpecularValue = value(fcmat, "Render/Render.Disney.Specular", "");
    std::string renderSpecularTintValue = value(fcmat, "Render/Render.Disney.SpecularTint", "");
    std::string renderSubsurfaceValue = value(fcmat, "Render/Render.Disney.Subsurface", "");

    // Split out the textures
    std::string renderAnisotropic;
    std::string renderAnisotropicTexture;
    splitTexture(renderAnisotropicValue, &renderAnisotropicTexture, &renderAnisotropic);
    std::string renderBaseColor;
    std::string renderBaseColorTexture;
    std::string renderBaseColorObject;
    splitTextureObject(renderBaseColorValue,
                       &renderBaseColorTexture,
                       &renderBaseColor,
                       &renderBaseColorObject);
    std::string renderClearCoat;
    std::string renderClearCoatTexture;
    std::string renderClearCoatObject;
    splitTextureObject(renderClearCoatValue,
                       &renderClearCoatTexture,
                       &renderClearCoat,
                       &renderClearCoatObject);
    std::string renderClearCoatGloss;
    std::string renderClearCoatGlossTexture;
    std::string renderClearCoatGlossObject;
    splitTextureObject(renderClearCoatGlossValue,
                       &renderClearCoatGlossTexture,
                       &renderClearCoatGloss,
                       &renderClearCoatGlossObject);
    std::string renderMetallic;
    std::string renderMetallicTexture;
    splitTexture(renderMetallicValue, &renderMetallicTexture, &renderMetallic);
    std::string renderRoughness;
    std::string renderRoughnessTexture;
    splitTexture(renderRoughnessValue, &renderRoughnessTexture, &renderRoughness);
    std::string renderSheen;
    std::string renderSheenTexture;
    splitTexture(renderSheenValue, &renderSheenTexture, &renderSheen);
    std::string renderSheenTint;
    std::string renderSheenTintTexture;
    splitTexture(renderSheenTintValue, &renderSheenTintTexture, &renderSheenTint);
    std::string renderSpecular;
    std::string renderSpecularTexture;
    std::string renderSpecularObject;
    splitTextureObject(renderSpecularValue,
                       &renderSpecularTexture,
                       &renderSpecular,
                       &renderSpecularObject);
    std::string renderSpecularTint;
    std::string renderSpecularTintTexture;
    std::string renderSpecularTintObject;
    splitTextureObject(renderSpecularTintValue,
                       &renderSpecularTintTexture,
                       &renderSpecularTint,
                       &renderSpecularTintObject);
    std::string renderSubsurface;
    std::string renderSubsurfaceTexture;
    splitTexture(renderSubsurfaceValue, &renderSubsurfaceTexture, &renderSubsurface);

    if (!renderAnisotropicValue.empty() || !renderBaseColorValue.empty()
        || !renderBump.empty() || !renderClearCoatValue.empty()
        || !renderClearCoatGlossValue.empty() || !renderDisplacement.empty()
        || !renderMetallicValue.empty() || !renderNormal.empty()
        || !renderRoughnessValue.empty() || !renderSheenValue.empty()
        || !renderSheenTintValue.empty() || !renderSpecularValue.empty()
        || !renderSpecularTintValue.empty() || !renderSubsurfaceValue.empty()) {
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

void MaterialConfigLoader::addRenderEmission(std::map<std::string, std::string>& fcmat,
                                             const std::shared_ptr<Material>& finalModel)
{
    std::string renderBump = value(fcmat, "Render/Render.Emission.Bump", "");
    std::string renderColorValue = value(fcmat, "Render/Render.Emission.Color", "");
    std::string renderNormal = value(fcmat, "Render/Render.Emission.Normal", "");
    std::string renderPowerValue = value(fcmat, "Render/Render.Emission.Power", "");

    // Split out the textures
    std::string renderColor;
    std::string renderColorTexture;
    std::string renderColorObject;
    splitTextureObject(renderColorValue, &renderColorTexture, &renderColor, &renderColorObject);
    std::string renderPower;
    std::string renderPowerTexture;
    splitTexture(renderPowerValue, &renderPowerTexture, &renderPower);

    if (!renderColorValue.empty() || !renderBump.empty() || !renderPowerValue.empty()
        || !renderNormal.empty()) {
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

void MaterialConfigLoader::addRenderGlass(std::map<std::string, std::string>& fcmat,
                                          const std::shared_ptr<Material>& finalModel)
{
    std::string renderBump = value(fcmat, "Render/Render.Glass.Bump", "");
    std::string renderColorValue = value(fcmat, "Render/Render.Glass.Color", "");
    std::string renderIORValue = value(fcmat, "Render/Render.Glass.IOR", "");
    std::string renderDisplacement = value(fcmat, "Render/Render.Glass.Displacement", "");
    std::string renderNormal = value(fcmat, "Render/Render.Glass.Normal", "");

    // Split out the textures
    std::string renderColor;
    std::string renderColorTexture;
    std::string renderColorObject;
    splitTextureObject(renderColorValue, &renderColorTexture, &renderColor, &renderColorObject);
    std::string renderIOR;
    std::string renderIORTexture;
    splitTexture(renderIORValue, &renderIORTexture, &renderIOR);

    if (!renderBump.empty() || !renderColorValue.empty() || !renderIORValue.empty()
        || !renderDisplacement.empty() || !renderNormal.empty()) {
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

void MaterialConfigLoader::addRenderLuxcore(std::map<std::string, std::string>& fcmat,
                                            const std::shared_ptr<Material>& finalModel)
{
    std::string prefix = "Render.Luxcore";
    std::string string = multiLineKey(fcmat, prefix);

    if (!string.empty()) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Render_Luxcore);

        // Now add the data
        setAppearanceValue(finalModel, "Render.Luxcore", string);
    }
}

void MaterialConfigLoader::addRenderLuxrender(std::map<std::string, std::string>& fcmat,
                                              const std::shared_ptr<Material>& finalModel)
{
    std::string prefix = "Render.Luxrender";
    std::string string = multiLineKey(fcmat, prefix);

    if (!string.empty()) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Render_Luxrender);

        // Now add the data
        setAppearanceValue(finalModel, "Render.Luxrender", string);
    }
}

void MaterialConfigLoader::addRenderMixed(std::map<std::string, std::string>& fcmat,
                                          const std::shared_ptr<Material>& finalModel)
{
    std::string renderBump = value(fcmat, "Render/Render.Mixed.Bump", "");
    std::string renderDiffuseColorValue = value(fcmat, "Render/Render.Mixed.Diffuse.Color", "");
    std::string renderDisplacement = value(fcmat, "Render/Render.Mixed.Displacement", "");
    std::string renderGlassColorValue = value(fcmat, "Render/Render.Mixed.Glass.Color", "");
    std::string renderGlassIORValue = value(fcmat, "Render/Render.Mixed.Glass.IOR", "");
    std::string renderNormal = value(fcmat, "Render/Render.Mixed.Normal", "");
    std::string renderTransparencyValue = value(fcmat, "Render/Render.Mixed.Transparency", "");

    // Split out the textures
    std::string renderDiffuseColor;
    std::string renderDiffuseColorTexture;
    std::string renderDiffuseColorObject;
    splitTextureObject(renderDiffuseColorValue,
                       &renderDiffuseColorTexture,
                       &renderDiffuseColor,
                       &renderDiffuseColorObject);
    std::string renderGlassColor;
    std::string renderGlassColorTexture;
    std::string renderGlassColorObject;
    splitTextureObject(renderGlassColorValue,
                       &renderGlassColorTexture,
                       &renderGlassColor,
                       &renderGlassColorObject);
    std::string renderGlassIOR;
    std::string renderGlassIORTexture;
    splitTexture(renderGlassIORValue, &renderGlassIORTexture, &renderGlassIOR);
    std::string renderTransparency;
    std::string renderTransparencyTexture;
    splitTexture(renderTransparencyValue, &renderTransparencyTexture, &renderTransparency);

    if (!renderBump.empty() || !renderDiffuseColorValue.empty() || !renderDisplacement.empty()
        || !renderGlassColorValue.empty() || !renderGlassIORValue.empty()
        || !renderNormal.empty() || !renderTransparencyValue.empty()) {
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

void MaterialConfigLoader::addRenderOspray(std::map<std::string, std::string>& fcmat,
                                           const std::shared_ptr<Material>& finalModel)
{
    std::string prefix = "Render.Ospray";
    std::string string = multiLineKey(fcmat, prefix);

    if (!string.empty()) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Render_Ospray);

        // Now add the data
        setAppearanceValue(finalModel, "Render.Ospray", string);
    }
}

void MaterialConfigLoader::addRenderPbrt(std::map<std::string, std::string>& fcmat,
                                         const std::shared_ptr<Material>& finalModel)
{
    std::string prefix = "Render.Pbrt";
    std::string string = multiLineKey(fcmat, prefix);

    if (!string.empty()) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Render_Pbrt);

        // Now add the data
        setAppearanceValue(finalModel, "Render.Pbrt", string);
    }
}

void MaterialConfigLoader::addRenderPovray(std::map<std::string, std::string>& fcmat,
                                           const std::shared_ptr<Material>& finalModel)
{
    std::string prefix = "Render.Povray";
    std::string string = multiLineKey(fcmat, prefix);

    if (!string.empty()) {
        finalModel->addAppearance(ModelUUIDs::ModelUUID_Render_Povray);

        // Now add the data
        setAppearanceValue(finalModel, "Render.Povray", string);
    }
}

void MaterialConfigLoader::addRenderSubstancePBR(std::map<std::string, std::string>& fcmat,
                                                 const std::shared_ptr<Material>& finalModel)
{
    std::string renderBaseColorValue = value(fcmat, "Render/Render.Substance_PBR.BaseColor", "");
    std::string renderBump = value(fcmat, "Render/Render.Substance_PBR.Bump", "");
    std::string renderMetallicValue = value(fcmat, "Render/Render.Substance_PBR.Metallic", "");
    std::string renderNormal = value(fcmat, "Render/Render.Substance_PBR.Normal", "");
    std::string renderRoughnessValue = value(fcmat, "Render/Render.Substance_PBR.Roughness", "");
    std::string renderSpecularValue = value(fcmat, "Render/Render.Substance_PBR.Specular", "");

    // Split out the textures
    std::string renderBaseColor;
    std::string renderBaseColorTexture;
    std::string renderBaseColorObject;
    splitTextureObject(renderBaseColorValue,
                       &renderBaseColorTexture,
                       &renderBaseColor,
                       &renderBaseColorObject);
    std::string renderMetallic;
    std::string renderMetallicTexture;
    splitTexture(renderMetallicValue, &renderMetallicTexture, &renderMetallic);
    std::string renderRoughness;
    std::string renderRoughnessTexture;
    splitTexture(renderRoughnessValue, &renderRoughnessTexture, &renderRoughness);
    std::string renderSpecular;
    std::string renderSpecularTexture;
    splitTexture(renderSpecularValue, &renderSpecularTexture, &renderSpecular);

    if (!renderBaseColorValue.empty() || !renderBump.empty() || !renderMetallicValue.empty()
        || !renderNormal.empty() || !renderRoughnessValue.empty()
        || !renderSpecularValue.empty()) {
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

void MaterialConfigLoader::addRenderTexture(std::map<std::string, std::string>& fcmat,
                                            const std::shared_ptr<Material>& finalModel)
{
    std::string renderName;
    auto renderImage = std::make_shared<QList<QVariant>>();
    std::string renderScale;
    std::string renderRotation;
    std::string renderTranslationU;
    std::string renderTranslationV;

    for (const auto& entry : fcmat) {
        const auto& key = entry.first;
        if (key.starts_with("Render/Render.Textures.")) {
            std::vector<std::string> list1 = Library::split(key, '.');
            if (renderName.empty()) {
                renderName = list1[2];
            }
            if (list1[3] == "Images") {
                renderImage->push_back(QString::fromStdString(value(fcmat, key, "")));
            }
            else if (list1[3] == "Scale") {
                renderScale = value(fcmat, key, "");
            }
            else if (list1[3] == "Rotation") {
                renderRotation = value(fcmat, key, "");
            }
            else if (list1[3] == "TranslationU") {
                renderTranslationU = value(fcmat, key, "");
            }
            else if (list1[3] == " TranslationV") {
                renderTranslationV = value(fcmat, key, "");
            }
        }
    }

    if (!renderName.empty()) {
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

void MaterialConfigLoader::addRenderWB(std::map<std::string, std::string>& fcmat,
                                       const std::shared_ptr<Material>& finalModel)
{
    std::string useObjectColor = value(fcmat, "General/UseObjectColor", "");
    std::string renderType = value(fcmat, "Render/Render.Type", "");

    if (!renderType.empty()) {
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

void MaterialConfigLoader::addCosts(const std::map<std::string, std::string>& fcmat,
                                    const std::shared_ptr<Material>& finalModel)
{
    std::string productURL = value(fcmat, "Cost/ProductURL", "");
    std::string specificPrice = value(fcmat, "Cost/SpecificPrice", "");
    std::string vendor = value(fcmat, "Cost/Vendor", "");

    if (productURL.length() + specificPrice.length() + vendor.length() > 0) {
        finalModel->addPhysical(ModelUUIDs::ModelUUID_Costs_Default);

        // Now add the data
        setPhysicalValue(finalModel, "ProductURL", productURL);
        setPhysicalValue(finalModel, "SpecificPrice", specificPrice);
        setPhysicalValue(finalModel, "Vendor", vendor);
    }
}

void MaterialConfigLoader::addArchitectural(const std::map<std::string, std::string>& fcmat,
                                            const std::shared_ptr<Material>& finalModel)
{
    std::string color = value(fcmat, "Architectural/Color", "");
    std::string environmentalEfficiencyClass =
        value(fcmat, "Architectural/EnvironmentalEfficiencyClass", "");
    std::string executionInstructions = value(fcmat, "Architectural/ExecutionInstructions", "");
    std::string finish = value(fcmat, "Architectural/Finish", "");
    std::string fireResistanceClass = value(fcmat, "Architectural/FireResistanceClass", "");
    std::string model = value(fcmat, "Architectural/Model", "");
    std::string soundTransmissionClass = value(fcmat, "Architectural/SoundTransmissionClass", "");
    std::string unitsPerQuantity = value(fcmat, "Architectural/UnitsPerQuantity", "");

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

void MaterialConfigLoader::addElectromagnetic(const std::map<std::string, std::string>& fcmat,
                                              const std::shared_ptr<Material>& finalModel)
{
    std::string relativePermittivity = value(fcmat, "Electromagnetic/RelativePermittivity", "");
    std::string electricalConductivity = value(fcmat, "Electromagnetic/ElectricalConductivity", "");
    std::string relativePermeability = value(fcmat, "Electromagnetic/RelativePermeability", "");

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

void MaterialConfigLoader::addThermal(const std::map<std::string, std::string>& fcmat,
                                      const std::shared_ptr<Material>& finalModel)
{
    std::string specificHeat = value(fcmat, "Thermal/SpecificHeat", "");
    std::string thermalConductivity = value(fcmat, "Thermal/ThermalConductivity", "");
    std::string thermalExpansionCoefficient = value(fcmat, "Thermal/ThermalExpansionCoefficient", "");

    if (specificHeat.length() + thermalConductivity.length() + thermalExpansionCoefficient.length()
        > 0) {
        finalModel->addPhysical(ModelUUIDs::ModelUUID_Thermal_Default);

        // Now add the data
        setPhysicalValue(finalModel, "SpecificHeat", specificHeat);
        setPhysicalValue(finalModel, "ThermalConductivity", thermalConductivity);
        setPhysicalValue(finalModel, "ThermalExpansionCoefficient", thermalExpansionCoefficient);
    }
}

void MaterialConfigLoader::addFluid(const std::map<std::string, std::string>& fcmat,
                                    const std::shared_ptr<Material>& finalModel)
{
    std::string density = value(fcmat, "Fluidic/Density", "");
    std::string dynamicViscosity = value(fcmat, "Fluidic/DynamicViscosity", "");
    std::string kinematicViscosity = value(fcmat, "Fluidic/KinematicViscosity", "");
    std::string prandtlNumber = value(fcmat, "Fluidic/PrandtlNumber", "");

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

void MaterialConfigLoader::addMechanical(const std::map<std::string, std::string>& fcmat,
                                         const std::shared_ptr<Material>& finalModel)
{
    std::string density = value(fcmat, "Mechanical/Density", "");
    std::string bulkModulus = value(fcmat, "Mechanical/BulkModulus", "");
    std::string poissonRatio = value(fcmat, "Mechanical/PoissonRatio", "");
    std::string shearModulus = value(fcmat, "Mechanical/ShearModulus", "");
    std::string youngsModulus = value(fcmat, "Mechanical/YoungsModulus", "");
    std::string angleOfFriction = value(fcmat, "Mechanical/AngleOfFriction", "");
    std::string compressiveStrength = value(fcmat, "Mechanical/CompressiveStrength", "");
    std::string fractureToughness = value(fcmat, "Mechanical/FractureToughness", "");
    std::string ultimateStrain = value(fcmat, "Mechanical/UltimateStrain", "");
    std::string ultimateTensileStrength = value(fcmat, "Mechanical/UltimateTensileStrength", "");
    std::string yieldStrength = value(fcmat, "Mechanical/YieldStrength", "");
    std::string stiffness = value(fcmat, "Mechanical/Stiffness", "");

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

void MaterialConfigLoader::addLegacy(const std::map<std::string, std::string>& fcmat,
                                     const std::shared_ptr<Material>& finalModel)
{
    for (auto const& entry : fcmat) {
        auto name = entry.first;
        if (const auto last = name.rfind('/'); last != std::string::npos && last > 0) {
            name = name.substr(last + 1);
        }

        if (!finalModel->hasNonLegacyProperty(name)) {
            setLegacyValue(finalModel, name, entry.second);
        }
    }
}

std::shared_ptr<Material>
MaterialConfigLoader::getMaterialFromPath(const std::shared_ptr<MaterialLibraryLocal>& library,
                                          const std::string& path)
{
    std::string author = getAuthorAndLicense(path);  // Place them both in the author field

    std::map<std::string, std::string> fcmat;
    if (!readFile(path, fcmat)) {
        Base::Console().log("Error reading '{}'\n", path);
        throw MaterialReadError();
    }

    // General section
    // QString name = value(fcmat, "Name", ""); - always get the name from the filename
    std::string name = QFileInfo(QString::fromStdString(path))
                           .fileName()
                           .remove(QLatin1String(".FCMat"), Qt::CaseInsensitive)
                           .toStdString();
    std::string uuid = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();

    std::string description = value(fcmat, "Description", "");
    std::string sourceReference = value(fcmat, "ReferenceSource", "");
    std::string sourceURL = value(fcmat, "SourceURL", "");

    auto baseLibrary = std::static_pointer_cast<Materials::MaterialLibrary>(library);
    std::shared_ptr<Material> finalModel =
        std::make_shared<Material>(baseLibrary, path, uuid, name);
    finalModel->setOldFormat(true);

    finalModel->setAuthor(author);
    finalModel->setDescription(description);
    finalModel->setReference(sourceReference);
    finalModel->setURL(sourceURL);

    std::string father = value(fcmat, "Father", "");
    if (!father.empty()) {
        finalModel->addPhysical(ModelUUIDs::ModelUUID_Legacy_Father);

        // Now add the data
        setPhysicalValue(finalModel, "Father", father);
    }

    std::string kindOfMaterial = value(fcmat, "KindOfMaterial", "");
    std::string materialNumber = value(fcmat, "MaterialNumber", "");
    std::string norm = value(fcmat, "Norm", "");
    std::string standardCode = value(fcmat, "StandardCode", "");
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
