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

#include <gtest/gtest.h>

#include <Mod/Material/App/PreCompiled.h>
#ifndef _PreComp_
#endif

#include <QLocale>
#include <QMetaType>
#include <QString>

#include <App/Application.h>
#include <Base/Quantity.h>
#include <Base/Interpreter.h>
#include <Gui/MetaTypes.h>
#include <src/App/InitApplication.h>

#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/Model.h>
#include <Mod/Material/App/ModelManager.h>
#include <Mod/Material/App/ModelUuids.h>

#ifdef _MSC_VER
#pragma warning(disable : 4834)
#endif

// clang-format off

class TestMaterial : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    if (App::Application::GetARGC() == 0) {
        tests::initApplication();
    }
  }

  void SetUp() override {
    Base::Interpreter().runString("import Part");
    _modelManager = new Materials::ModelManager();
    _materialManager = new Materials::MaterialManager();
  }

  // void TearDown() override {}
  Materials::ModelManager* _modelManager;
  Materials::MaterialManager* _materialManager;
};

TEST_F(TestMaterial, TestInstallation)
{
    ASSERT_NE(_modelManager, nullptr);

    // We should have loaded at least the system library
    auto libraries = _materialManager->getMaterialLibraries();
    ASSERT_GT(libraries->size(), 0);

    // We should have at least one material
    auto materials = _materialManager->getMaterials();
    ASSERT_GT(materials->size(), 0);
}

TEST_F(TestMaterial, TestMaterialsWithModel)
{
    ASSERT_NE(_materialManager, nullptr);

    auto materials = _materialManager->materialsWithModel(
        QString::fromStdString("f6f9e48c-b116-4e82-ad7f-3659a9219c50")); // IsotropicLinearElastic
    EXPECT_GT(materials->size(), 0);

    auto materialsComplete = _materialManager->materialsWithModelComplete(
        QString::fromStdString("f6f9e48c-b116-4e82-ad7f-3659a9219c50"));  // IsotropicLinearElastic
    EXPECT_LE(materialsComplete->size(), materials->size());

    auto materialsLinearElastic = _materialManager->materialsWithModel(
        QString::fromStdString("7b561d1d-fb9b-44f6-9da9-56a4f74d7536")); // LinearElastic

    // All LinearElastic models should be in IsotropicLinearElastic since it is inherited
    EXPECT_LE(materialsLinearElastic->size(), materials->size());
    for (auto itp : *materialsLinearElastic) {
        auto mat = itp.first;
        EXPECT_NO_THROW(materials->at(mat));
    }
}

TEST_F(TestMaterial, TestMaterialByPath)
{
    ASSERT_NE(_materialManager, nullptr);

    auto steel = _materialManager->getMaterialByPath(
        QString::fromStdString("Standard/Metal/Steel/CalculiX-Steel.FCMat"),
        QString::fromStdString("System"));
    EXPECT_NE(&steel, nullptr);
    EXPECT_EQ(steel->getName(), QString::fromStdString("CalculiX-Steel"));
    EXPECT_EQ(steel->getUUID(), QString::fromStdString("92589471-a6cb-4bbc-b748-d425a17dea7d"));

    // The same but with a leading '/'
    auto steel2 = _materialManager->getMaterialByPath(
        QString::fromStdString("/Standard/Metal/Steel/CalculiX-Steel.FCMat"),
        QString::fromStdString("System"));
    EXPECT_NE(&steel2, nullptr);
    EXPECT_EQ(steel2->getName(), QString::fromStdString("CalculiX-Steel"));
    EXPECT_EQ(steel2->getUUID(), QString::fromStdString("92589471-a6cb-4bbc-b748-d425a17dea7d"));

    // Same with the library name as a prefix
    auto steel3 = _materialManager->getMaterialByPath(
        QString::fromStdString("/System/Standard/Metal/Steel/CalculiX-Steel.FCMat"),
        QString::fromStdString("System"));
    EXPECT_NE(&steel3, nullptr);
    EXPECT_EQ(steel3->getName(), QString::fromStdString("CalculiX-Steel"));
    EXPECT_EQ(steel3->getUUID(), QString::fromStdString("92589471-a6cb-4bbc-b748-d425a17dea7d"));
}

TEST_F(TestMaterial, TestAddPhysicalModel)
{
    // Start with an empty material
    Materials::Material material;
    auto models = material.getPhysicalModels();
    EXPECT_NE(&models, nullptr);
    EXPECT_EQ(models->size(), 0);

    // Add a model
    material.addPhysical(Materials::ModelUUIDs::ModelUUID_Electromagnetic_Default);
    models = material.getPhysicalModels();
    EXPECT_EQ(models->size(), 1);

    // Add a second model
    material.addPhysical(Materials::ModelUUIDs::ModelUUID_Mechanical_LinearElastic);
    models = material.getPhysicalModels();
    EXPECT_EQ(models->size(), 2);

    // Add an inherited model
    material.addPhysical(Materials::ModelUUIDs::ModelUUID_Mechanical_IsotropicLinearElastic);
    models = material.getPhysicalModels();
    EXPECT_EQ(models->size(), 2);

    // Add a super model
    material.clearModels();
    EXPECT_EQ(models->size(), 0);

    material.addPhysical(Materials::ModelUUIDs::ModelUUID_Mechanical_IsotropicLinearElastic);
    models = material.getPhysicalModels();
    EXPECT_EQ(models->size(), 1);
    material.addPhysical(Materials::ModelUUIDs::ModelUUID_Mechanical_LinearElastic);
    models = material.getPhysicalModels();
    EXPECT_EQ(models->size(), 1);

    // Remove the inherited model
    material.removePhysical(Materials::ModelUUIDs::ModelUUID_Mechanical_IsotropicLinearElastic);
    models = material.getPhysicalModels();
    EXPECT_EQ(models->size(), 1);

    // Remove the super model
    material.removePhysical(Materials::ModelUUIDs::ModelUUID_Mechanical_LinearElastic);
    models = material.getPhysicalModels();
    EXPECT_EQ(models->size(), 0);
}

TEST_F(TestMaterial, TestAddAppearanceModel)
{
    // Start with an empty material
    Materials::Material material;
    auto models = material.getAppearanceModels();
    EXPECT_NE(models, nullptr);
    EXPECT_EQ(models->size(), 0);

    // Add a model
    material.addAppearance(Materials::ModelUUIDs::ModelUUID_Rendering_Vector);
    models = material.getAppearanceModels();
    EXPECT_EQ(models->size(), 1);

    // Add a second model
    material.addAppearance(Materials::ModelUUIDs::ModelUUID_Rendering_Advanced);
    models = material.getAppearanceModels();
    EXPECT_EQ(models->size(), 2);

    // Add an inherited model
    material.addAppearance(Materials::ModelUUIDs::ModelUUID_Rendering_Basic);
    models = material.getAppearanceModels();
    EXPECT_EQ(models->size(), 2);

    // Add a super model
    material.clearModels();
    EXPECT_EQ(models->size(), 0);

    material.addAppearance(Materials::ModelUUIDs::ModelUUID_Rendering_Basic);
    models = material.getAppearanceModels();
    EXPECT_EQ(models->size(), 1);
    material.addAppearance(Materials::ModelUUIDs::ModelUUID_Rendering_Advanced);
    models = material.getAppearanceModels();
    EXPECT_EQ(models->size(), 1);

    // Remove the inherited model
    material.removeAppearance(Materials::ModelUUIDs::ModelUUID_Rendering_Basic);
    models = material.getAppearanceModels();
    EXPECT_EQ(models->size(), 1);

    // Remove the super model
    material.removeAppearance(Materials::ModelUUIDs::ModelUUID_Rendering_Advanced);
    models = material.getAppearanceModels();
    EXPECT_EQ(models->size(), 0);
}

QString parseQuantity(const char *string)
{
    QString value = QString::fromStdString(string);
    return Base::Quantity::parse(value).getUserString();
}

TEST_F(TestMaterial, TestCalculiXSteel)
{
    ASSERT_NE(_materialManager, nullptr);

    auto steel = _materialManager->getMaterial(QString::fromStdString("92589471-a6cb-4bbc-b748-d425a17dea7d"));
    EXPECT_EQ(steel->getName(), QString::fromStdString("CalculiX-Steel"));
    EXPECT_EQ(steel->getUUID(), QString::fromStdString("92589471-a6cb-4bbc-b748-d425a17dea7d"));

    EXPECT_TRUE(steel->hasPhysicalModel(Materials::ModelUUIDs::ModelUUID_Mechanical_Density)); // Density
    EXPECT_TRUE(steel->hasPhysicalModel(Materials::ModelUUIDs::ModelUUID_Mechanical_IsotropicLinearElastic)); // IsotropicLinearElastic
    EXPECT_TRUE(steel->hasPhysicalModel(Materials::ModelUUIDs::ModelUUID_Thermal_Default)); // Thermal
    EXPECT_FALSE(steel->hasPhysicalModel(Materials::ModelUUIDs::ModelUUID_Mechanical_LinearElastic)); // Legacy linear elastic - Not in the model
    EXPECT_TRUE(steel->hasAppearanceModel(Materials::ModelUUIDs::ModelUUID_Rendering_Basic)); // BasicRendering - inherited from Steel.FCMat

    EXPECT_TRUE(steel->isPhysicalModelComplete(Materials::ModelUUIDs::ModelUUID_Mechanical_Density)); // Density
    EXPECT_FALSE(steel->isPhysicalModelComplete(Materials::ModelUUIDs::ModelUUID_Mechanical_IsotropicLinearElastic)); // IsotropicLinearElastic - incomplete
    EXPECT_FALSE(steel->isPhysicalModelComplete(Materials::ModelUUIDs::ModelUUID_Thermal_Default)); // Thermal
    EXPECT_FALSE(steel->isPhysicalModelComplete(Materials::ModelUUIDs::ModelUUID_Mechanical_LinearElastic)); // Legacy linear elastic - Not in the model
    EXPECT_TRUE(steel->isAppearanceModelComplete(Materials::ModelUUIDs::ModelUUID_Rendering_Basic)); // BasicRendering - inherited from Steel.FCMat

    EXPECT_TRUE(steel->hasPhysicalProperty(QString::fromStdString("Density")));
    EXPECT_TRUE(steel->hasPhysicalProperty(QString::fromStdString("BulkModulus")));
    EXPECT_TRUE(steel->hasPhysicalProperty(QString::fromStdString("PoissonRatio")));
    EXPECT_TRUE(steel->hasPhysicalProperty(QString::fromStdString("YoungsModulus")));
    EXPECT_TRUE(steel->hasPhysicalProperty(QString::fromStdString("ShearModulus")));
    EXPECT_TRUE(steel->hasPhysicalProperty(QString::fromStdString("SpecificHeat")));
    EXPECT_TRUE(steel->hasPhysicalProperty(QString::fromStdString("ThermalConductivity")));
    EXPECT_TRUE(steel->hasPhysicalProperty(QString::fromStdString("ThermalExpansionCoefficient")));
    EXPECT_TRUE(steel->hasAppearanceProperty(QString::fromStdString("AmbientColor")));
    EXPECT_TRUE(steel->hasAppearanceProperty(QString::fromStdString("DiffuseColor")));
    EXPECT_TRUE(steel->hasAppearanceProperty(QString::fromStdString("EmissiveColor")));
    EXPECT_TRUE(steel->hasAppearanceProperty(QString::fromStdString("Shininess")));
    EXPECT_TRUE(steel->hasAppearanceProperty(QString::fromStdString("SpecularColor")));
    EXPECT_TRUE(steel->hasAppearanceProperty(QString::fromStdString("Transparency")));

    auto& properties = steel->getPhysicalProperties();
    EXPECT_NO_THROW(properties.at(QString::fromStdString("Density")));
    EXPECT_NO_THROW(properties.at(QString::fromStdString("BulkModulus"))); // This is different from the Python behaviour
    EXPECT_NO_THROW(properties.at(QString::fromStdString("PoissonRatio")));
    EXPECT_NO_THROW(properties.at(QString::fromStdString("YoungsModulus")));
    EXPECT_NO_THROW(properties.at(QString::fromStdString("ShearModulus")));
    EXPECT_NO_THROW(properties.at(QString::fromStdString("SpecificHeat")));
    EXPECT_NO_THROW(properties.at(QString::fromStdString("ThermalConductivity")));
    EXPECT_NO_THROW(properties.at(QString::fromStdString("ThermalExpansionCoefficient")));
    EXPECT_THROW(properties.at(QString::fromStdString("AmbientColor")), std::out_of_range);
    EXPECT_THROW(properties.at(QString::fromStdString("DiffuseColor")), std::out_of_range);
    EXPECT_THROW(properties.at(QString::fromStdString("EmissiveColor")), std::out_of_range);
    EXPECT_THROW(properties.at(QString::fromStdString("Shininess")), std::out_of_range);
    EXPECT_THROW(properties.at(QString::fromStdString("SpecularColor")), std::out_of_range);
    EXPECT_THROW(properties.at(QString::fromStdString("Transparency")), std::out_of_range);

    auto& properties1 = steel->getAppearanceProperties();
    EXPECT_THROW(properties1.at(QString::fromStdString("Density")), std::out_of_range);
    EXPECT_THROW(properties1.at(QString::fromStdString("BulkModulus")), std::out_of_range);
    EXPECT_THROW(properties1.at(QString::fromStdString("PoissonRatio")), std::out_of_range);
    EXPECT_THROW(properties1.at(QString::fromStdString("YoungsModulus")), std::out_of_range);
    EXPECT_THROW(properties1.at(QString::fromStdString("ShearModulus")), std::out_of_range);
    EXPECT_THROW(properties1.at(QString::fromStdString("SpecificHeat")), std::out_of_range);
    EXPECT_THROW(properties1.at(QString::fromStdString("ThermalConductivity")), std::out_of_range);
    EXPECT_THROW(properties1.at(QString::fromStdString("ThermalExpansionCoefficient")), std::out_of_range);
    EXPECT_NO_THROW(properties1.at(QString::fromStdString("AmbientColor")));
    EXPECT_NO_THROW(properties1.at(QString::fromStdString("DiffuseColor")));
    EXPECT_NO_THROW(properties1.at(QString::fromStdString("EmissiveColor")));
    EXPECT_NO_THROW(properties1.at(QString::fromStdString("Shininess")));
    EXPECT_NO_THROW(properties1.at(QString::fromStdString("SpecularColor")));
    EXPECT_NO_THROW(properties1.at(QString::fromStdString("Transparency")));

    EXPECT_FALSE(properties[QString::fromStdString("Density")]->isNull());
    EXPECT_TRUE(properties[QString::fromStdString("BulkModulus")]->isNull());
    EXPECT_FALSE(properties[QString::fromStdString("PoissonRatio")]->isNull());
    EXPECT_FALSE(properties[QString::fromStdString("YoungsModulus")]->isNull());
    EXPECT_TRUE(properties[QString::fromStdString("ShearModulus")]->isNull());
    EXPECT_FALSE(properties[QString::fromStdString("SpecificHeat")]->isNull());
    EXPECT_FALSE(properties[QString::fromStdString("ThermalConductivity")]->isNull());
    EXPECT_FALSE(properties[QString::fromStdString("ThermalExpansionCoefficient")]->isNull());
    EXPECT_FALSE(properties1[QString::fromStdString("AmbientColor")]->isNull());
    EXPECT_FALSE(properties1[QString::fromStdString("DiffuseColor")]->isNull());
    EXPECT_FALSE(properties1[QString::fromStdString("EmissiveColor")]->isNull());
    EXPECT_FALSE(properties1[QString::fromStdString("Shininess")]->isNull());
    EXPECT_FALSE(properties1[QString::fromStdString("SpecularColor")]->isNull());
    EXPECT_FALSE(properties1[QString::fromStdString("Transparency")]->isNull());

    QLocale locale;
    EXPECT_EQ(properties[QString::fromStdString("Density")]->getString(), parseQuantity("7900.00 kg/m^3"));
    EXPECT_EQ(properties[QString::fromStdString("PoissonRatio")]->getString(), locale.toString(0.3));
    EXPECT_EQ(properties[QString::fromStdString("YoungsModulus")]->getString(), parseQuantity("210.00 GPa"));
    EXPECT_EQ(properties[QString::fromStdString("SpecificHeat")]->getString(), parseQuantity("590.00 J/kg/K"));
    EXPECT_EQ(properties[QString::fromStdString("ThermalConductivity")]->getString(), parseQuantity("43.00 W/m/K"));
    EXPECT_EQ(properties[QString::fromStdString("ThermalExpansionCoefficient")]->getString(), parseQuantity("12.00 µm/m/K"));
    EXPECT_EQ(properties1[QString::fromStdString("AmbientColor")]->getString(), QString::fromStdString("(0.0020, 0.0020, 0.0020, 1.0)"));
    EXPECT_EQ(properties1[QString::fromStdString("DiffuseColor")]->getString(), QString::fromStdString("(0.0000, 0.0000, 0.0000, 1.0)"));
    EXPECT_EQ(properties1[QString::fromStdString("EmissiveColor")]->getString(), QString::fromStdString("(0.0000, 0.0000, 0.0000, 1.0)"));
    EXPECT_EQ(properties1[QString::fromStdString("Shininess")]->getString(), locale.toString(0.06));
    EXPECT_EQ(properties1[QString::fromStdString("SpecularColor")]->getString(), QString::fromStdString("(0.9800, 0.9800, 0.9800, 1.0)"));
    EXPECT_EQ(properties1[QString::fromStdString("Transparency")]->getString(), QString::fromStdString("0"));

    EXPECT_TRUE(properties[QString::fromStdString("BulkModulus")]->getString().isEmpty());
    EXPECT_TRUE(properties[QString::fromStdString("ShearModulus")]->getString().isEmpty());

    // These are the preferred method of access
    //
    EXPECT_DOUBLE_EQ(steel->getPhysicalQuantity(QString::fromStdString("Density")).getValue(), 7.9e-06);
    EXPECT_NEAR(steel->getPhysicalValue(QString::fromStdString("PoissonRatio")).toDouble(), 0.3, 1e-6);
    EXPECT_DOUBLE_EQ(steel->getPhysicalQuantity(QString::fromStdString("YoungsModulus")).getValue(), 210000000.0);
    EXPECT_DOUBLE_EQ(steel->getPhysicalQuantity(QString::fromStdString("SpecificHeat")).getValue(), 590000000.0);
    EXPECT_DOUBLE_EQ(steel->getPhysicalQuantity(QString::fromStdString("ThermalConductivity")).getValue(), 43000.0);
    EXPECT_DOUBLE_EQ(steel->getPhysicalQuantity(QString::fromStdString("ThermalExpansionCoefficient")).getValue(), 1.2e-05);
    EXPECT_EQ(steel->getAppearanceValue(QString::fromStdString("AmbientColor")), QString::fromStdString("(0.0020, 0.0020, 0.0020, 1.0)"));
    EXPECT_EQ(steel->getAppearanceValue(QString::fromStdString("DiffuseColor")), QString::fromStdString("(0.0000, 0.0000, 0.0000, 1.0)"));
    EXPECT_EQ(steel->getAppearanceValue(QString::fromStdString("EmissiveColor")), QString::fromStdString("(0.0000, 0.0000, 0.0000, 1.0)"));
    EXPECT_NEAR(steel->getAppearanceValue(QString::fromStdString("Shininess")).toDouble(), 0.06, 1e-6);
    EXPECT_EQ(steel->getAppearanceValue(QString::fromStdString("SpecularColor")), QString::fromStdString("(0.9800, 0.9800, 0.9800, 1.0)"));
    EXPECT_DOUBLE_EQ(steel->getAppearanceValue(QString::fromStdString("Transparency")).toDouble(), 0.0);

    EXPECT_EQ(steel->getPhysicalQuantity(QString::fromStdString("Density")).getUserString(), parseQuantity("7900.00 kg/m^3"));
    EXPECT_EQ(steel->getPhysicalQuantity(QString::fromStdString("YoungsModulus")).getUserString(), parseQuantity("210.00 GPa"));
    EXPECT_EQ(steel->getPhysicalQuantity(QString::fromStdString("SpecificHeat")).getUserString(), parseQuantity("590.00 J/kg/K"));
    EXPECT_EQ(steel->getPhysicalQuantity(QString::fromStdString("ThermalConductivity")).getUserString(), parseQuantity("43.00 W/m/K"));
    EXPECT_EQ(steel->getPhysicalQuantity(QString::fromStdString("ThermalExpansionCoefficient")).getUserString(), parseQuantity("12.00 µm/m/K"));

}

TEST_F(TestMaterial, TestColumns)
{
    // Start with an empty material
    Materials::Material testMaterial;
    auto models = testMaterial.getPhysicalModels();
    EXPECT_NE(&models, nullptr);
    EXPECT_EQ(models->size(), 0);

    // Add a model
    testMaterial.addPhysical(Materials::ModelUUIDs::ModelUUID_Test_Model);
    models = testMaterial.getPhysicalModels();
    EXPECT_EQ(models->size(), 1);

    EXPECT_TRUE(testMaterial.hasPhysicalProperty(QString::fromStdString("TestArray2D")));
    auto array2d = testMaterial.getPhysicalProperty(QString::fromStdString("TestArray2D"))->getMaterialValue();
    EXPECT_TRUE(array2d);
    EXPECT_EQ(dynamic_cast<Materials::Material2DArray &>(*array2d).columns(), 2);

    EXPECT_TRUE(testMaterial.hasPhysicalProperty(QString::fromStdString("TestArray2D3Column")));
    auto array2d3Column = testMaterial.getPhysicalProperty(QString::fromStdString("TestArray2D3Column"))->getMaterialValue();
    EXPECT_TRUE(array2d3Column);
    EXPECT_EQ(dynamic_cast<Materials::Material2DArray &>(*array2d3Column).columns(), 3);

    EXPECT_TRUE(testMaterial.hasPhysicalProperty(QString::fromStdString("TestArray3D")));
    auto array3d = testMaterial.getPhysicalProperty(QString::fromStdString("TestArray3D"))->getMaterialValue();
    EXPECT_TRUE(array3d);
    EXPECT_EQ(dynamic_cast<Materials::Material3DArray &>(*array3d).columns(), 2);
}

// clang-format on
