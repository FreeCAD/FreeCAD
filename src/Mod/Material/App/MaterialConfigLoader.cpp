/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <fstream>
#include <App/Application.h>
#include <Base/Interpreter.h>

#include <QDirIterator>
#include <QFileInfo>
#include <QUuid>
#include <QString>

// #include <boost/uuid/uuid.hpp>            // uuid class
// #include <boost/uuid/uuid_generators.hpp> // generators
// #include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

#include "Model.h"
#include "MaterialLoader.h"
#include "MaterialConfigLoader.h"
#include "ModelUuids.h"


using namespace Materials;

MaterialConfigLoader::MaterialConfigLoader()
{
}

/*
 *  Destroys the object and frees any allocated resources
 */
MaterialConfigLoader::~MaterialConfigLoader()
{}

bool MaterialConfigLoader::isConfigStyle(const std::string& path)
{
    std::ifstream infile(path);

    // Check the first 2 lines for a ";"
    for (int i = 0; i < 2; i++) {
        std::string line;
        if (!std::getline(infile, line))
            return false;
        if (line.at(0) != ';')
            return false;
    }

    return true;
}

std::string MaterialConfigLoader::getAuthorAndLicense(const std::string& path)
{
    std::ifstream infile(path);
    std::string noAuthor = "";

    // Skip the first line
    std::string line;
    if (!std::getline(infile, line))
        return noAuthor;

    // The second line has it in a comment
    if (!std::getline(infile, line))
        return noAuthor;
    std::size_t found = line.find(";");
    if (found!=std::string::npos)
        return trim_copy(line.substr(found + 1));

    return noAuthor;
}

void MaterialConfigLoader::addVectorRendering(const QSettings &fcmat, Material *finalModel)
{
    std::string sectionFillPattern = value(fcmat, "VectorRendering/SectionFillPattern", "");
    std::string sectionLinewidth = value(fcmat, "VectorRendering/SectionLinewidth", "");
    std::string sectionColor = value(fcmat, "VectorRendering/SectionColor", "");
    std::string viewColor = value(fcmat, "VectorRendering/ViewColor", "");
    std::string viewFillPattern = value(fcmat, "VectorRendering/ViewFillPattern", "");
    std::string viewLinewidth = value(fcmat, "VectorRendering/ViewLinewidth", "");

    if (sectionFillPattern.length() + sectionLinewidth.length() + sectionColor.length() +
            viewColor.length() + viewFillPattern.length() + viewLinewidth.length() > 0)
        finalModel->addAppearanceModel(ModelUUID_Rendering_Vector);

    // Now add the data
    setAppearanceProperty(finalModel, "SectionFillPattern", sectionFillPattern);
    setAppearanceProperty(finalModel, "SectionLinewidth", sectionLinewidth);
    setAppearanceProperty(finalModel, "SectionColor", sectionColor);
    setAppearanceProperty(finalModel, "ViewColor", viewColor);
    setAppearanceProperty(finalModel, "ViewFillPattern", viewFillPattern);
    setAppearanceProperty(finalModel, "ViewLinewidth", viewLinewidth);
}

void MaterialConfigLoader::addRendering(const QSettings &fcmat, Material *finalModel)
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

    // Check which model we need
    bool useTexture = false;
    bool useAdvanced = false;
    bool useBasic = false;
    if (texturePath.length() + textureScaling.length() > 0)
        useTexture = true;
    if (fragmentShader.length() + vertexShader.length() > 0)
        useAdvanced = true;
    if (ambientColor.length() + diffuseColor.length() + emissiveColor.length() +
            shininess.length() + specularColor.length() + transparency.length() > 0)
        useBasic = true;

    if (useAdvanced)
    {
        finalModel->addAppearanceModel(ModelUUID_Rendering_Advanced);
    }
    else if (useTexture)
    {
        finalModel->addAppearanceModel(ModelUUID_Rendering_Texture);
    }
    else if (useBasic)
    {
        finalModel->addAppearanceModel(ModelUUID_Rendering_Basic);
    }   

    // Now add the data
    setAppearanceProperty(finalModel, "AmbientColor", ambientColor);
    setAppearanceProperty(finalModel, "DiffuseColor", diffuseColor);
    setAppearanceProperty(finalModel, "EmissiveColor", emissiveColor);
    setAppearanceProperty(finalModel, "Shininess", shininess);
    setAppearanceProperty(finalModel, "SpecularColor", specularColor);
    setAppearanceProperty(finalModel, "Transparency", transparency);
    setAppearanceProperty(finalModel, "TexturePath", texturePath);
    setAppearanceProperty(finalModel, "TextureScaling", textureScaling);
    setAppearanceProperty(finalModel, "FragmentShader", fragmentShader);
    setAppearanceProperty(finalModel, "VertexShader", vertexShader);
}

void MaterialConfigLoader::addCosts(const QSettings &fcmat, Material *finalModel)
{
    std::string productURL = value(fcmat, "Cost/ProductURL", "");
    std::string specificPrice = value(fcmat, "Cost/SpecificPrice", "");
    std::string vendor = value(fcmat, "Cost/Vendor", "");

    if (productURL.length() + specificPrice.length() + vendor.length() > 0)
        finalModel->addModel(ModelUUID_Costs_Default);

    // Now add the data
    setProperty(finalModel, "ProductURL", productURL);
    setProperty(finalModel, "SpecificPrice", specificPrice);
    setProperty(finalModel, "Vendor", vendor);
}

void MaterialConfigLoader::addArchitectural(const QSettings &fcmat, Material *finalModel)
{
    std::string color = value(fcmat, "Architectural/Color", "");
    std::string environmentalEfficiencyClass = value(fcmat, "Architectural/EnvironmentalEfficiencyClass", "");
    std::string executionInstructions = value(fcmat, "Architectural/ExecutionInstructions", "");
    std::string finish = value(fcmat, "Architectural/Finish", "");
    std::string fireResistanceClass = value(fcmat, "Architectural/FireResistanceClass", "");
    std::string model = value(fcmat, "Architectural/Model", "");
    std::string soundTransmissionClass = value(fcmat, "Architectural/SoundTransmissionClass", "");
    std::string unitsPerQuantity = value(fcmat, "Architectural/UnitsPerQuantity", "");

    if (color.length() + environmentalEfficiencyClass.length() + executionInstructions.length() +
            finish.length() + fireResistanceClass.length() + model.length() +
            soundTransmissionClass.length() + unitsPerQuantity.length() > 0)
        finalModel->addModel(ModelUUID_Architectural_Default);

    // Now add the data
    setProperty(finalModel, "Color", color);
    setProperty(finalModel, "EnvironmentalEfficiencyClass", environmentalEfficiencyClass);
    setProperty(finalModel, "ExecutionInstructions", executionInstructions);
    setProperty(finalModel, "Finish", finish);
    setProperty(finalModel, "FireResistanceClass", fireResistanceClass);
    setProperty(finalModel, "Model", model);
    setProperty(finalModel, "SoundTransmissionClass", soundTransmissionClass);
    setProperty(finalModel, "UnitsPerQuantity", unitsPerQuantity);
}

void MaterialConfigLoader::addElectromagnetic(const QSettings &fcmat, Material *finalModel)
{
    std::string relativePermittivity = value(fcmat, "Electromagnetic/RelativePermittivity", "");
    std::string electricalConductivity = value(fcmat, "Electromagnetic/ElectricalConductivity", "");
    std::string relativePermeability = value(fcmat, "Electromagnetic/RelativePermeability", "");

    if (relativePermittivity.length() + electricalConductivity.length() + relativePermeability.length() > 0)
        finalModel->addModel(ModelUUID_Electromagnetic_Default);

    // Now add the data
    setProperty(finalModel, "RelativePermittivity", relativePermittivity);
    setProperty(finalModel, "ElectricalConductivity", electricalConductivity);
    setProperty(finalModel, "RelativePermeability", relativePermeability);
}

void MaterialConfigLoader::addThermal(const QSettings &fcmat, Material *finalModel)
{
    std::string specificHeat = value(fcmat, "Thermal/SpecificHeat", "");
    std::string thermalConductivity = value(fcmat, "Thermal/ThermalConductivity", "");
    std::string thermalExpansionCoefficient = value(fcmat, "Thermal/ThermalExpansionCoefficient", "");

    if (specificHeat.length() + thermalConductivity.length() + thermalExpansionCoefficient.length() > 0)
        finalModel->addModel(ModelUUID_Thermal_Default);

    // Now add the data
    setProperty(finalModel, "SpecificHeat", specificHeat);
    setProperty(finalModel, "ThermalConductivity", thermalConductivity);
    setProperty(finalModel, "ThermalExpansionCoefficient", thermalExpansionCoefficient);
}

void MaterialConfigLoader::addFluid(const QSettings &fcmat, Material *finalModel)
{
    std::string density = value(fcmat, "Fluidic/Density", "");
    std::string dynamicViscosity = value(fcmat, "Fluidic/DynamicViscosity", "");
    std::string kinematicViscosity = value(fcmat, "Fluidic/KinematicViscosity", "");
    std::string prandtlNumber = value(fcmat, "Fluidic/PrandtlNumber", "");

    // Check which model we need
    bool useDensity = false;
    bool useFluid = false;
    if (density.length() > 0)
        useDensity = true;
    if (dynamicViscosity.length() + kinematicViscosity.length() + prandtlNumber.length() > 0)
        useFluid = true;

    if (useFluid)
    {
        finalModel->addModel(ModelUUID_Fluid_Default);
    } else if (useDensity)
    {
        finalModel->addModel(ModelUUID_Mechanical_Density);
    }

    // Now add the data
    setProperty(finalModel, "Density", density);
    setProperty(finalModel, "DynamicViscosity", dynamicViscosity);
    setProperty(finalModel, "KinematicViscosity", kinematicViscosity);
    setProperty(finalModel, "PrandtlNumber", prandtlNumber);
}

void MaterialConfigLoader::addMechanical(const QSettings &fcmat, Material *finalModel)
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
    if (density.length() > 0)
        useDensity = true;
    if (bulkModulus.length() + poissonRatio.length() + shearModulus.length() + youngsModulus.length() > 0)
        useIso = true;
    if (angleOfFriction.length() + compressiveStrength.length() + fractureToughness.length()
         + ultimateStrain.length() + ultimateTensileStrength.length() + yieldStrength.length() + stiffness.length() > 0)
        useLinearElastic = true;

    if (useLinearElastic)
    {
        finalModel->addModel(ModelUUID_Mechanical_LinearElastic);
    } else 
    {
        if (useIso)
            finalModel->addModel(ModelUUID_Mechanical_IsotropicLinearElastic);
        if (useDensity)
            finalModel->addModel(ModelUUID_Mechanical_Density);
    }

    // Now add the data
    setProperty(finalModel, "Density", density);
    setProperty(finalModel, "BulkModulus", bulkModulus);
    setProperty(finalModel, "PoissonRatio", poissonRatio);
    setProperty(finalModel, "ShearModulus", shearModulus);
    setProperty(finalModel, "YoungsModulus", youngsModulus);
    setProperty(finalModel, "AngleOfFriction", angleOfFriction);
    setProperty(finalModel, "CompressiveStrength", compressiveStrength);
    setProperty(finalModel, "FractureToughness", fractureToughness);
    setProperty(finalModel, "UltimateStrain", ultimateStrain);
    setProperty(finalModel, "UltimateTensileStrength", ultimateTensileStrength);
    setProperty(finalModel, "YieldStrength", yieldStrength);
    setProperty(finalModel, "Stiffness", stiffness);

}

Material *MaterialConfigLoader::getMaterialFromPath(const MaterialLibrary &library, const std::string &path)
{
    QDir modelDir(QString::fromStdString(path));
    std::string authorAndLicense = getAuthorAndLicense(path);

    QSettings fcmat(QString::fromStdString(path), QSettings::IniFormat);

    // General section
    std::string name = value(fcmat, "Name", "");
    std::string uuid = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();

    std::string version = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    std::string description = value(fcmat, "Description", "");
    std::string sourceReference = value(fcmat, "ReferenceSource", "");
    std::string sourceURL = value(fcmat, "SourceURL", "");

    Material *finalModel = new Material(library, modelDir, uuid, name);
    finalModel->setVersion(version);
    finalModel->setAuthorAndLicense(authorAndLicense);
    finalModel->setDescription(description);
    finalModel->setReference(sourceReference);
    finalModel->setURL(sourceURL);

    std::string father = value(fcmat, "Father", "");
    if (father.length() > 0)
    {
        finalModel->addModel(ModelUUID_Legacy_Father);

        // Now add the data
        setProperty(finalModel, "Father", father);
    }

    std::string kindOfMaterial = value(fcmat, "KindOfMaterial", "");
    std::string materialNumber = value(fcmat, "MaterialNumber", "");
    std::string norm = value(fcmat, "Norm", "");
    std::string standardCode = value(fcmat, "StandardCode", "");
    if (kindOfMaterial.length() + materialNumber.length() + norm.length() + standardCode.length() > 0)
    {
        finalModel->addModel(ModelUUID_Legacy_MaterialStandard);

        // Now add the data
        setProperty(finalModel, "KindOfMaterial", kindOfMaterial);
        setProperty(finalModel, "MaterialNumber", materialNumber);
        setProperty(finalModel, "StandardCode", norm);              // Norm is the same as StandardCode
        setProperty(finalModel, "StandardCode", standardCode);
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


#include "moc_MaterialConfigLoader.cpp"
