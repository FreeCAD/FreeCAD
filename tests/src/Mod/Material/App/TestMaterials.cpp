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

#include <string>

#include <QLocale>
#include <QMetaType>

#include <App/Application.h>
#include <Base/Interpreter.h>
#include <Base/Quantity.h>
#include <Gui/MetaTypes.h>
#include <src/App/InitApplication.h>

#include <Mod/Material/App/Exceptions.h>
#include <Mod/Material/App/MaterialLibrary.h>
#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/MaterialValue.h>
#include <Mod/Material/App/Model.h>
#include <Mod/Material/App/ModelManager.h>
#include <Mod/Material/App/ModelUuids.h>

#ifdef _MSC_VER
# pragma warning(disable : 4834)
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
    _modelManager = &(Materials::ModelManager::getManager());
    _materialManager = &(Materials::MaterialManager::getManager());
  }

  // void TearDown() override {}
  Materials::ModelManager* _modelManager;
  Materials::MaterialManager* _materialManager;
};

TEST_F(TestMaterial, TestInstallation)
{
    ASSERT_NE(_modelManager, nullptr);

    // We should have loaded at least the system library
    auto libraries = _materialManager->getLibraries();
    ASSERT_GT(libraries->size(), 0);

    // We should have at least one material
    auto materials = _materialManager->getLocalMaterials();
    ASSERT_GT(materials->size(), 0);
}

TEST_F(TestMaterial, TestMaterialsWithModel)
{
    ASSERT_NE(_materialManager, nullptr);

    auto materials = _materialManager->materialsWithModel(
        "f6f9e48c-b116-4e82-ad7f-3659a9219c50"); // IsotropicLinearElastic
    EXPECT_GT(materials->size(), 0);

    auto materialsComplete = _materialManager->materialsWithModelComplete(
        "f6f9e48c-b116-4e82-ad7f-3659a9219c50");  // IsotropicLinearElastic
    EXPECT_LE(materialsComplete->size(), materials->size());

    auto materialsLinearElastic = _materialManager->materialsWithModel(
        "7b561d1d-fb9b-44f6-9da9-56a4f74d7536"); // LinearElastic

    // All LinearElastic models should be in IsotropicLinearElastic since it is inherited
    EXPECT_LE(materialsLinearElastic->size(), materials->size());
    for (auto &itp : *materialsLinearElastic) {
        auto mat = itp.first;
        EXPECT_NO_THROW(materials->at(mat));
    }
}

TEST_F(TestMaterial, TestMaterialByPath)
{
    ASSERT_NE(_materialManager, nullptr);

    auto steel = _materialManager->getMaterialByPath(
        "Standard/Metal/Steel/CalculiX-Steel.FCMat",
        "System");
    EXPECT_NE(&steel, nullptr);
    EXPECT_EQ(steel->getName(), "CalculiX-Steel");
    EXPECT_EQ(steel->getUUID(), "92589471-a6cb-4bbc-b748-d425a17dea7d");

    // The same but with a leading '/'
    auto steel2 = _materialManager->getMaterialByPath(
        "/Standard/Metal/Steel/CalculiX-Steel.FCMat",
        "System");
    EXPECT_NE(&steel2, nullptr);
    EXPECT_EQ(steel2->getName(), "CalculiX-Steel");
    EXPECT_EQ(steel2->getUUID(), "92589471-a6cb-4bbc-b748-d425a17dea7d");

    // Same with the library name as a prefix
    auto steel3 = _materialManager->getMaterialByPath(
        "/System/Standard/Metal/Steel/CalculiX-Steel.FCMat",
        "System");
    EXPECT_NE(&steel3, nullptr);
    EXPECT_EQ(steel3->getName(), "CalculiX-Steel");
    EXPECT_EQ(steel3->getUUID(), "92589471-a6cb-4bbc-b748-d425a17dea7d");
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

TEST_F(TestMaterial, TestValidateAppearanceModels)
{
    // The local library and the matching remote one
    const Materials::Library library {"Library", QByteArray(), true};
    auto localLibrary = std::make_shared<Materials::MaterialLibrary>(library);
    auto remoteLibrary = std::make_shared<Materials::MaterialLibrary>(library);

    Materials::Material material;
    material.setLibrary(localLibrary);
    material.addPhysical(Materials::ModelUUIDs::ModelUUID_Mechanical_Density);

    Materials::Material remote(material);
    remote.setLibrary(remoteLibrary);
    EXPECT_NO_THROW(material.validate(remote));

    remote.addAppearance(Materials::ModelUUIDs::ModelUUID_Mechanical_Density);
    EXPECT_EQ(material.getPhysicalModels()->size(), remote.getPhysicalModels()->size());
    EXPECT_EQ(material.getAppearanceModels()->size(), 0);
    EXPECT_EQ(remote.getAppearanceModels()->size(), 1);

    try {
        material.validate(remote);
        FAIL() << "A differing appearance model was accepted";
    }
    catch (const Materials::InvalidMaterial& e) {
        EXPECT_STREQ(e.what(), "Material appearance model counts don't match");
    }
}

std::string parseQuantity(const std::string& value)
{
    auto quantity = Base::Quantity::parse(value);
    quantity.setFormat(Materials::MaterialValue::getQuantityFormat());
    return quantity.getUserString();
}

TEST_F(TestMaterial, TestCalculiXSteel)
{
    ASSERT_NE(_materialManager, nullptr);

    auto steel = _materialManager->getMaterial("92589471-a6cb-4bbc-b748-d425a17dea7d");
    EXPECT_EQ(steel->getName(), "CalculiX-Steel");
    EXPECT_EQ(steel->getUUID(), "92589471-a6cb-4bbc-b748-d425a17dea7d");

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

    EXPECT_TRUE(steel->hasPhysicalProperty("Density"));
    EXPECT_TRUE(steel->hasPhysicalProperty("BulkModulus"));
    EXPECT_TRUE(steel->hasPhysicalProperty("PoissonRatio"));
    EXPECT_TRUE(steel->hasPhysicalProperty("YoungsModulus"));
    EXPECT_TRUE(steel->hasPhysicalProperty("ShearModulus"));
    EXPECT_TRUE(steel->hasPhysicalProperty("SpecificHeat"));
    EXPECT_TRUE(steel->hasPhysicalProperty("ThermalConductivity"));
    EXPECT_TRUE(steel->hasPhysicalProperty("ThermalExpansionCoefficient"));
    EXPECT_TRUE(steel->hasAppearanceProperty("AmbientColor"));
    EXPECT_TRUE(steel->hasAppearanceProperty("DiffuseColor"));
    EXPECT_TRUE(steel->hasAppearanceProperty("EmissiveColor"));
    EXPECT_TRUE(steel->hasAppearanceProperty("Shininess"));
    EXPECT_TRUE(steel->hasAppearanceProperty("SpecularColor"));
    EXPECT_TRUE(steel->hasAppearanceProperty("Transparency"));

    auto& properties = steel->getPhysicalProperties();
    EXPECT_NO_THROW(properties.at("Density"));
    EXPECT_NO_THROW(properties.at("BulkModulus")); // This is different from the Python behaviour
    EXPECT_NO_THROW(properties.at("PoissonRatio"));
    EXPECT_NO_THROW(properties.at("YoungsModulus"));
    EXPECT_NO_THROW(properties.at("ShearModulus"));
    EXPECT_NO_THROW(properties.at("SpecificHeat"));
    EXPECT_NO_THROW(properties.at("ThermalConductivity"));
    EXPECT_NO_THROW(properties.at("ThermalExpansionCoefficient"));
    EXPECT_THROW(properties.at("AmbientColor"), std::out_of_range);
    EXPECT_THROW(properties.at("DiffuseColor"), std::out_of_range);
    EXPECT_THROW(properties.at("EmissiveColor"), std::out_of_range);
    EXPECT_THROW(properties.at("Shininess"), std::out_of_range);
    EXPECT_THROW(properties.at("SpecularColor"), std::out_of_range);
    EXPECT_THROW(properties.at("Transparency"), std::out_of_range);

    auto& properties1 = steel->getAppearanceProperties();
    EXPECT_THROW(properties1.at("Density"), std::out_of_range);
    EXPECT_THROW(properties1.at("BulkModulus"), std::out_of_range);
    EXPECT_THROW(properties1.at("PoissonRatio"), std::out_of_range);
    EXPECT_THROW(properties1.at("YoungsModulus"), std::out_of_range);
    EXPECT_THROW(properties1.at("ShearModulus"), std::out_of_range);
    EXPECT_THROW(properties1.at("SpecificHeat"), std::out_of_range);
    EXPECT_THROW(properties1.at("ThermalConductivity"), std::out_of_range);
    EXPECT_THROW(properties1.at("ThermalExpansionCoefficient"), std::out_of_range);
    EXPECT_NO_THROW(properties1.at("AmbientColor"));
    EXPECT_NO_THROW(properties1.at("DiffuseColor"));
    EXPECT_NO_THROW(properties1.at("EmissiveColor"));
    EXPECT_NO_THROW(properties1.at("Shininess"));
    EXPECT_NO_THROW(properties1.at("SpecularColor"));
    EXPECT_NO_THROW(properties1.at("Transparency"));

    EXPECT_FALSE(properties["Density"]->isNull());
    EXPECT_TRUE(properties["BulkModulus"]->isNull());
    EXPECT_FALSE(properties["PoissonRatio"]->isNull());
    EXPECT_FALSE(properties["YoungsModulus"]->isNull());
    EXPECT_TRUE(properties["ShearModulus"]->isNull());
    EXPECT_FALSE(properties["SpecificHeat"]->isNull());
    EXPECT_FALSE(properties["ThermalConductivity"]->isNull());
    EXPECT_FALSE(properties["ThermalExpansionCoefficient"]->isNull());
    EXPECT_FALSE(properties1["AmbientColor"]->isNull());
    EXPECT_FALSE(properties1["DiffuseColor"]->isNull());
    EXPECT_FALSE(properties1["EmissiveColor"]->isNull());
    EXPECT_FALSE(properties1["Shininess"]->isNull());
    EXPECT_FALSE(properties1["SpecularColor"]->isNull());
    EXPECT_FALSE(properties1["Transparency"]->isNull());

    QLocale locale;
    EXPECT_EQ(properties["Density"]->getString(), parseQuantity("7900.00 kg/m^3"));
    EXPECT_EQ(properties["PoissonRatio"]->getString(), locale.toString(0.3).toStdString());
    EXPECT_EQ(properties["YoungsModulus"]->getString(), parseQuantity("210.00 GPa"));
    EXPECT_EQ(properties["SpecificHeat"]->getString(), parseQuantity("590.00 J/kg/K"));
    EXPECT_EQ(properties["ThermalConductivity"]->getString(), parseQuantity("43.00 W/m/K"));
    EXPECT_EQ(properties["ThermalExpansionCoefficient"]->getString(), parseQuantity("12.00 µm/m/K"));
    EXPECT_EQ(properties1["AmbientColor"]->getString(), "(0.0020, 0.0020, 0.0020, 1.0)");
    EXPECT_EQ(properties1["DiffuseColor"]->getString(), "(0.0000, 0.0000, 0.0000, 1.0)");
    EXPECT_EQ(properties1["EmissiveColor"]->getString(), "(0.0000, 0.0000, 0.0000, 1.0)");
    EXPECT_EQ(properties1["Shininess"]->getString(), locale.toString(0.06).toStdString());
    EXPECT_EQ(properties1["SpecularColor"]->getString(), "(0.9800, 0.9800, 0.9800, 1.0)");
    EXPECT_EQ(properties1["Transparency"]->getString(), "0");

    EXPECT_TRUE(properties["BulkModulus"]->getString().empty());
    EXPECT_TRUE(properties["ShearModulus"]->getString().empty());

    // These are the preferred method of access
    //
    EXPECT_DOUBLE_EQ(steel->getPhysicalQuantity("Density").getValue(), 7.9e-06);
    EXPECT_NEAR(steel->getPhysicalValue("PoissonRatio").toDouble(), 0.3, 1e-6);
    EXPECT_DOUBLE_EQ(steel->getPhysicalQuantity("YoungsModulus").getValue(), 210000000.0);
    EXPECT_DOUBLE_EQ(steel->getPhysicalQuantity("SpecificHeat").getValue(), 590000000.0);
    EXPECT_DOUBLE_EQ(steel->getPhysicalQuantity("ThermalConductivity").getValue(), 43000.0);
    EXPECT_DOUBLE_EQ(steel->getPhysicalQuantity("ThermalExpansionCoefficient").getValue(), 1.2e-05);
    EXPECT_EQ(steel->getAppearanceValueString("AmbientColor"), "(0.0020, 0.0020, 0.0020, 1.0)");
    EXPECT_EQ(steel->getAppearanceValueString("DiffuseColor"), "(0.0000, 0.0000, 0.0000, 1.0)");
    EXPECT_EQ(steel->getAppearanceValueString("EmissiveColor"), "(0.0000, 0.0000, 0.0000, 1.0)");
    EXPECT_NEAR(steel->getAppearanceValue("Shininess").toDouble(), 0.06, 1e-6);
    EXPECT_EQ(steel->getAppearanceValueString("SpecularColor"), "(0.9800, 0.9800, 0.9800, 1.0)");
    EXPECT_DOUBLE_EQ(steel->getAppearanceValue("Transparency").toDouble(), 0.0);

    EXPECT_EQ(steel->getPhysicalQuantity("Density").getUserString(), parseQuantity("7900.00 kg/m^3"));
    EXPECT_EQ(steel->getPhysicalQuantity("YoungsModulus").getUserString(), parseQuantity("210.00 GPa"));
    EXPECT_EQ(steel->getPhysicalQuantity("SpecificHeat").getUserString(), parseQuantity("590.00 J/kg/K"));
    EXPECT_EQ(steel->getPhysicalQuantity("ThermalConductivity").getUserString(), parseQuantity("43.00 W/m/K"));
    EXPECT_EQ(steel->getPhysicalQuantity("ThermalExpansionCoefficient").getUserString(), parseQuantity("12.00 µm/m/K"));
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

    EXPECT_TRUE(testMaterial.hasPhysicalProperty("TestArray2D"));
    auto array2d = testMaterial.getPhysicalProperty("TestArray2D")->getMaterialValue();
    EXPECT_TRUE(array2d);
    EXPECT_EQ(dynamic_cast<Materials::Array2D &>(*array2d).columns(), 2);

    EXPECT_TRUE(testMaterial.hasPhysicalProperty("TestArray2D3Column"));
    auto array2d3Column = testMaterial.getPhysicalProperty("TestArray2D3Column")->getMaterialValue();
    EXPECT_TRUE(array2d3Column);
    EXPECT_EQ(dynamic_cast<Materials::Array2D &>(*array2d3Column).columns(), 3);

    EXPECT_TRUE(testMaterial.hasPhysicalProperty("TestArray3D"));
    auto array3d = testMaterial.getPhysicalProperty("TestArray3D")->getMaterialValue();
    EXPECT_TRUE(array3d);
    EXPECT_EQ(dynamic_cast<Materials::Array3D &>(*array3d).columns(), 2);
}

// clang-format on
