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
    std::string sectionFillPattern = value(fcmat, "SectionFillPattern", "");
    std::string sectionLinewidth = value(fcmat, "SectionLinewidth", "");
    std::string sectionColor = value(fcmat, "SectionColor", "");
    std::string viewColor = value(fcmat, "ViewColor", "");
    std::string viewFillPattern = value(fcmat, "ViewFillPattern", "");
    std::string viewLinewidth = value(fcmat, "ViewLinewidth", "");

    if (sectionFillPattern.length() + sectionLinewidth.length() + sectionColor.length() +
            viewColor.length() + viewFillPattern.length() + viewLinewidth.length() > 0)
        finalModel->addModel(ModelUUID_Rendering_Vector);

    // Now add the data

}

void MaterialConfigLoader::addRendering(const QSettings &fcmat, Material *finalModel)
{
    std::string ambientColor = value(fcmat, "AmbientColor", "");
    std::string diffuseColor = value(fcmat, "DiffuseColor", "");
    std::string emissiveColor = value(fcmat, "EmissiveColor", "");
    std::string shininess = value(fcmat, "Shininess", "");
    std::string specularColor = value(fcmat, "SpecularColor", "");
    std::string transparency = value(fcmat, "Transparency", "");
    std::string texturePath = value(fcmat, "TexturePath", "");
    std::string textureScaling = value(fcmat, "TextureScaling", "");
    std::string fragmentShader = value(fcmat, "FragmentShader", "");
    std::string vertexShader = value(fcmat, "VertexShader", "");

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
        finalModel->addModel(ModelUUID_Rendering_Advanced);

        // Now add the data
    }
    else if (useTexture)
    {
        finalModel->addModel(ModelUUID_Rendering_Texture);

        // Now add the data
    }
    else if (useBasic)
    {
        finalModel->addModel(ModelUUID_Rendering_Basic);

        // Now add the data
    }   
}

void MaterialConfigLoader::addCosts(const QSettings &fcmat, Material *finalModel)
{
    std::string productURL = value(fcmat, "ProductURL", "");
    std::string specificPrice = value(fcmat, "SpecificPrice", "");
    std::string vendor = value(fcmat, "Vendor", "");

    if (productURL.length() + specificPrice.length() + vendor.length() > 0)
        finalModel->addModel(ModelUUID_Costs_Default);

    // Now add the data

}

void MaterialConfigLoader::addArchitectural(const QSettings &fcmat, Material *finalModel)
{
    std::string color = value(fcmat, "Color", "");
    std::string environmentalEfficiencyClass = value(fcmat, "EnvironmentalEfficiencyClass", "");
    std::string executionInstructions = value(fcmat, "ExecutionInstructions", "");
    std::string finish = value(fcmat, "Finish", "");
    std::string fireResistanceClass = value(fcmat, "FireResistanceClass", "");
    std::string model = value(fcmat, "Architecture/Model", "");
    std::string soundTransmissionClass = value(fcmat, "SoundTransmissionClass", "");
    std::string unitsPerQuantity = value(fcmat, "UnitsPerQuantity", "");

    if (color.length() + environmentalEfficiencyClass.length() + executionInstructions.length() +
            finish.length() + fireResistanceClass.length() + model.length() +
            soundTransmissionClass.length() + unitsPerQuantity.length() > 0)
        finalModel->addModel(ModelUUID_Architectural_Default);

    // Now add the data

}

void MaterialConfigLoader::addElectromagnetic(const QSettings &fcmat, Material *finalModel)
{
    std::string relativePermittivity = value(fcmat, "RelativePermittivity", "");
    std::string electricalConductivity = value(fcmat, "ElectricalConductivity", "");
    std::string relativePermeability = value(fcmat, "RelativePermeability", "");

    if (relativePermittivity.length() + electricalConductivity.length() + relativePermeability.length() > 0)
        finalModel->addModel(ModelUUID_Electromagnetic_Default);

    // Now add the data

}

void MaterialConfigLoader::addThermal(const QSettings &fcmat, Material *finalModel)
{
    std::string specificHeat = value(fcmat, "SpecificHeat", "");
    std::string thermalConductivity = value(fcmat, "ThermalConductivity", "");
    std::string thermalExpansionCoefficient = value(fcmat, "ThermalExpansionCoefficient", "");

    if (specificHeat.length() + thermalConductivity.length() + thermalExpansionCoefficient.length() > 0)
        finalModel->addModel(ModelUUID_Thermal_Default);

    // Now add the data
}

void MaterialConfigLoader::addFluid(const QSettings &fcmat, Material *finalModel)
{
    std::string density = value(fcmat, "Fluidic/Density", "");
    std::string dynamicViscosity = value(fcmat, "DynamicViscosity", "");
    std::string kinematicViscosity = value(fcmat, "KinematicViscosity", "");
    std::string prandtlNumber = value(fcmat, "PrandtlNumber", "");

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

        // Now add the data
    } else if (useDensity)
    {
        finalModel->addModel(ModelUUID_Mechanical_Density);

        // Now add the data
    }

}

void MaterialConfigLoader::addMechanical(const QSettings &fcmat, Material *finalModel)
{
    std::string density = value(fcmat, "Mechanical/Density", "");
    std::string bulkModulus = value(fcmat, "BulkModulus", "");
    std::string poissonRatio = value(fcmat, "PoissonRatio", "");
    std::string shearModulus = value(fcmat, "ShearModulus", "");
    std::string youngsModulus = value(fcmat, "YoungsModulus", "");
    std::string angleOfFriction = value(fcmat, "AngleOfFriction", "");
    std::string compressiveStrength = value(fcmat, "CompressiveStrength", "");
    std::string fractureToughness = value(fcmat, "FractureToughness", "");
    std::string ultimateStrain = value(fcmat, "UltimateStrain", "");
    std::string ultimateTensileStrength = value(fcmat, "UltimateTensileStrength", "");
    std::string yieldStrength = value(fcmat, "YieldStrength", "");
    std::string stiffness = value(fcmat, "Stiffness", "");

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

        // Now add the data
    } else if (useIso)
    {
        finalModel->addModel(ModelUUID_Mechanical_IsotropicLinearElastic);

        // Now add the data
    } else if (useDensity)
    {
        finalModel->addModel(ModelUUID_Mechanical_Density);

        // Now add the data
    }

}

Material *MaterialConfigLoader::getMaterialFromPath(const MaterialLibrary &library, const std::string &path)
{
    QDir modelDir(QString::fromStdString(path));
    std::string authorAndLicense = getAuthorAndLicense(path);

    QSettings fcmat(QString::fromStdString(path), QSettings::IniFormat);

    // General section
    std::string name = value(fcmat, "Name", "");
    std::string uuid = QUuid::createUuid().toString().toStdString();

    std::string version = QUuid::createUuid().toString().toStdString();
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
    }

    std::string kindOfMaterial = value(fcmat, "KindOfMaterial", "");
    std::string materialNumber = value(fcmat, "MaterialNumber", "");
    std::string norm = value(fcmat, "Norm", "");
    std::string standardCode = value(fcmat, "StandardCode", "");
    if (kindOfMaterial.length() + materialNumber.length() + norm.length() + standardCode.length() > 0)
    {
        finalModel->addModel(ModelUUID_Legacy_MaterialStandard);

        // Now add the data
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
