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
#endif

#include <QUuid>

#include <App/Application.h>
#include <Base/Interpreter.h>
#include <fstream>


#include "MaterialConfigLoader.h"
#include "MaterialLoader.h"
#include "Model.h"
#include "ModelUuids.h"


using namespace Materials;

MaterialConfigLoader::MaterialConfigLoader()
{}

bool MaterialConfigLoader::isConfigStyle(const QString& path)
{
    std::ifstream infile(path.toStdString());

    // Check the first 2 lines for a ";"
    for (int i = 0; i < 2; i++) {
        std::string line;
        if (!std::getline(infile, line)) {
            return false;
        }
        if (line.at(0) != ';') {
            return false;
        }
    }

    return true;
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
    std::size_t found = line.find(";");
    if (found != std::string::npos) {
        return QString::fromStdString(trim_copy(line.substr(found + 1)));
    }

    return noAuthor;
}

void MaterialConfigLoader::addVectorRendering(const QSettings& fcmat, Material* finalModel)
{
    QString sectionFillPattern = value(fcmat, "VectorRendering/SectionFillPattern", "");
    QString sectionLinewidth = value(fcmat, "VectorRendering/SectionLinewidth", "");
    QString sectionColor = value(fcmat, "VectorRendering/SectionColor", "");
    QString viewColor = value(fcmat, "VectorRendering/ViewColor", "");
    QString viewFillPattern = value(fcmat, "VectorRendering/ViewFillPattern", "");
    QString viewLinewidth = value(fcmat, "VectorRendering/ViewLinewidth", "");

    if (sectionFillPattern.length() + sectionLinewidth.length() + sectionColor.length()
            + viewColor.length() + viewFillPattern.length() + viewLinewidth.length()
        > 0) {
        finalModel->addAppearance(ModelUUID_Rendering_Vector);
    }

    // Now add the data
    setAppearanceValue(finalModel, "SectionFillPattern", sectionFillPattern);
    setAppearanceValue(finalModel, "SectionLinewidth", sectionLinewidth);
    setAppearanceValue(finalModel, "SectionColor", sectionColor);
    setAppearanceValue(finalModel, "ViewColor", viewColor);
    setAppearanceValue(finalModel, "ViewFillPattern", viewFillPattern);
    setAppearanceValue(finalModel, "ViewLinewidth", viewLinewidth);
}

void MaterialConfigLoader::addRendering(const QSettings& fcmat, Material* finalModel)
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
        finalModel->addAppearance(ModelUUID_Rendering_Advanced);
    }
    else if (useTexture) {
        finalModel->addAppearance(ModelUUID_Rendering_Texture);
    }
    else if (useBasic) {
        finalModel->addAppearance(ModelUUID_Rendering_Basic);
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

void MaterialConfigLoader::addCosts(const QSettings& fcmat, Material* finalModel)
{
    QString productURL = value(fcmat, "Cost/ProductURL", "");
    QString specificPrice = value(fcmat, "Cost/SpecificPrice", "");
    QString vendor = value(fcmat, "Cost/Vendor", "");

    if (productURL.length() + specificPrice.length() + vendor.length() > 0) {
        finalModel->addPhysical(ModelUUID_Costs_Default);
    }

    // Now add the data
    setPhysicalValue(finalModel, "ProductURL", productURL);
    setPhysicalValue(finalModel, "SpecificPrice", specificPrice);
    setPhysicalValue(finalModel, "Vendor", vendor);
}

void MaterialConfigLoader::addArchitectural(const QSettings& fcmat, Material* finalModel)
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

    if (color.length() + environmentalEfficiencyClass.length() + executionInstructions.length()
            + finish.length() + fireResistanceClass.length() + model.length()
            + soundTransmissionClass.length() + unitsPerQuantity.length()
        > 0) {
        finalModel->addPhysical(ModelUUID_Architectural_Default);
    }

    // Now add the data
    setPhysicalValue(finalModel, "Color", color);
    setPhysicalValue(finalModel, "EnvironmentalEfficiencyClass", environmentalEfficiencyClass);
    setPhysicalValue(finalModel, "ExecutionInstructions", executionInstructions);
    setPhysicalValue(finalModel, "Finish", finish);
    setPhysicalValue(finalModel, "FireResistanceClass", fireResistanceClass);
    setPhysicalValue(finalModel, "Model", model);
    setPhysicalValue(finalModel, "SoundTransmissionClass", soundTransmissionClass);
    setPhysicalValue(finalModel, "UnitsPerQuantity", unitsPerQuantity);
}

void MaterialConfigLoader::addElectromagnetic(const QSettings& fcmat, Material* finalModel)
{
    QString relativePermittivity = value(fcmat, "Electromagnetic/RelativePermittivity", "");
    QString electricalConductivity = value(fcmat, "Electromagnetic/ElectricalConductivity", "");
    QString relativePermeability = value(fcmat, "Electromagnetic/RelativePermeability", "");

    if (relativePermittivity.length() + electricalConductivity.length()
            + relativePermeability.length()
        > 0) {
        finalModel->addPhysical(ModelUUID_Electromagnetic_Default);
    }

    // Now add the data
    setPhysicalValue(finalModel, "RelativePermittivity", relativePermittivity);
    setPhysicalValue(finalModel, "ElectricalConductivity", electricalConductivity);
    setPhysicalValue(finalModel, "RelativePermeability", relativePermeability);
}

void MaterialConfigLoader::addThermal(const QSettings& fcmat, Material* finalModel)
{
    QString specificHeat = value(fcmat, "Thermal/SpecificHeat", "");
    QString thermalConductivity = value(fcmat, "Thermal/ThermalConductivity", "");
    QString thermalExpansionCoefficient = value(fcmat, "Thermal/ThermalExpansionCoefficient", "");

    if (specificHeat.length() + thermalConductivity.length() + thermalExpansionCoefficient.length()
        > 0) {
        finalModel->addPhysical(ModelUUID_Thermal_Default);
    }

    // Now add the data
    setPhysicalValue(finalModel, "SpecificHeat", specificHeat);
    setPhysicalValue(finalModel, "ThermalConductivity", thermalConductivity);
    setPhysicalValue(finalModel, "ThermalExpansionCoefficient", thermalExpansionCoefficient);
}

void MaterialConfigLoader::addFluid(const QSettings& fcmat, Material* finalModel)
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
        finalModel->addPhysical(ModelUUID_Fluid_Default);
    }
    else if (useDensity) {
        finalModel->addPhysical(ModelUUID_Mechanical_Density);
    }

    // Now add the data
    setPhysicalValue(finalModel, "Density", density);
    setPhysicalValue(finalModel, "DynamicViscosity", dynamicViscosity);
    setPhysicalValue(finalModel, "KinematicViscosity", kinematicViscosity);
    setPhysicalValue(finalModel, "PrandtlNumber", prandtlNumber);
}

void MaterialConfigLoader::addMechanical(const QSettings& fcmat, Material* finalModel)
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
        finalModel->addPhysical(ModelUUID_Mechanical_LinearElastic);
    }
    else {
        if (useIso) {
            finalModel->addPhysical(ModelUUID_Mechanical_IsotropicLinearElastic);
        }
        if (useDensity) {
            finalModel->addPhysical(ModelUUID_Mechanical_Density);
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

Material* MaterialConfigLoader::getMaterialFromPath(const MaterialLibrary& library,
                                                    const QString& path)
{
    QString authorAndLicense = getAuthorAndLicense(path);

    QSettings fcmat(path, QSettings::IniFormat);

    // General section
    // QString name = value(fcmat, "Name", ""); - always get the name from the filename
    QFileInfo filepath(path);
    QString name =
        filepath.fileName().remove(QString::fromStdString(".FCMat"), Qt::CaseInsensitive);
    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);

    QString description = value(fcmat, "Description", "");
    QString sourceReference = value(fcmat, "ReferenceSource", "");
    QString sourceURL = value(fcmat, "SourceURL", "");

    Material* finalModel = new Material(library, path, uuid, name);
    finalModel->setAuthorAndLicense(authorAndLicense);
    finalModel->setDescription(description);
    finalModel->setReference(sourceReference);
    finalModel->setURL(sourceURL);

    QString father = value(fcmat, "Father", "");
    if (father.length() > 0) {
        finalModel->addPhysical(ModelUUID_Legacy_Father);

        // Now add the data
        setPhysicalValue(finalModel, "Father", father);
    }

    QString kindOfMaterial = value(fcmat, "KindOfMaterial", "");
    QString materialNumber = value(fcmat, "MaterialNumber", "");
    QString norm = value(fcmat, "Norm", "");
    QString standardCode = value(fcmat, "StandardCode", "");
    if (kindOfMaterial.length() + materialNumber.length() + norm.length() + standardCode.length()
        > 0) {
        finalModel->addPhysical(ModelUUID_Legacy_MaterialStandard);

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

    return finalModel;
}
