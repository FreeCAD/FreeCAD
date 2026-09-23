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

#include <memory>

#include <QMetaType>
#include <QString>

#include <App/Application.h>
#include <Gui/MetaTypes.h>

#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/Model.h>
#include <Mod/Material/App/ModelManager.h>
#include <Mod/Material/App/ModelUuids.h>

class TestMaterialProperties: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {}

    void SetUp() override
    {
        // 2D Properties
        modelProp = Materials::ModelProperty(
            "Density",  // Name
            "D",        // Header
            "2DArray",  // Type
            "",         // Units
            "",         // URL
            "desc"
        );  // Description
        modelProp1 = Materials::ModelProperty("Temperature", "T", "Quantity", "C", "", "desc1");
        modelProp2 = Materials::ModelProperty(
            "Density",
            "D",
            "Quantity",
            "kg/m^3",
            "https://en.wikipedia.org/wiki/Density",
            "desc2"
        );

        modelProp.addColumn(modelProp1);
        modelProp.addColumn(modelProp2);

        // 3D properties
        model3DProp = Materials::ModelProperty(
            "StressStrain",     // Name
            "Stress / Strain",  // Header
            "3DArray",          // Type
            "",                 // Units
            "",                 // URL
            "3 Dimensional array showing stress and strain as a function of "
            "temperature"
        );  // Description
        model3DProp1 = Materials::ModelProperty("Temperature", "T", "Quantity", "C", "", "desc1");
        model3DProp2 = Materials::ModelProperty("Stress", "Stress", "Quantity", "MPa", "", "desc2");
        model3DProp3 = Materials::ModelProperty("Strain", "Strain", "Quantity", "MPa", "", "desc3");

        model3DProp.addColumn(model3DProp1);
        model3DProp.addColumn(model3DProp2);
        model3DProp.addColumn(model3DProp3);
    }

    // void TearDown() override {}

    Materials::ModelProperty modelProp;
    Materials::ModelProperty modelProp1;
    Materials::ModelProperty modelProp2;
    Materials::ModelProperty model3DProp;
    Materials::ModelProperty model3DProp1;
    Materials::ModelProperty model3DProp2;
    Materials::ModelProperty model3DProp3;
};

TEST_F(TestMaterialProperties, TestEmpty)
{
    Materials::MaterialProperty prop;
    EXPECT_EQ(prop.getType(), Materials::MaterialValue::None);
    EXPECT_TRUE(prop.isNull());
    auto variant = prop.getValue();
    EXPECT_TRUE(variant.isNull());
    EXPECT_FALSE(variant.canConvert<QString>());
    EXPECT_TRUE(variant.toString().isNull());
    EXPECT_TRUE(variant.toString().isEmpty());
    EXPECT_EQ(variant.toString().size(), 0);
}

TEST_F(TestMaterialProperties, TestSingle)
{
    Materials::MaterialProperty prop(modelProp1, "sampleUUID");
    EXPECT_EQ(prop.getType(), Materials::MaterialValue::Quantity);
    EXPECT_EQ(prop.getModelUUID(), "sampleUUID");
    EXPECT_TRUE(prop.isNull());
    auto variant = prop.getValue();
    EXPECT_TRUE(variant.canConvert<Base::Quantity>());
    EXPECT_FALSE(variant.value<Base::Quantity>().isValid());
    EXPECT_FALSE(variant.canConvert<QString>());
    EXPECT_TRUE(variant.toString().isNull());
    EXPECT_TRUE(variant.toString().isEmpty());
    EXPECT_EQ(variant.toString().size(), 0);
}

void check2DArray(Materials::MaterialProperty& prop)
{
    EXPECT_EQ(prop.getType(), Materials::MaterialValue::Array2D);
    EXPECT_EQ(prop.getModelUUID(), "sampleUUID");
    EXPECT_TRUE(prop.isNull());
    auto array = std::static_pointer_cast<Materials::Array2D>(prop.getMaterialValue());
    EXPECT_EQ(array->rows(), 0);
    auto variant = prop.getValue();  // Throw an error?
    EXPECT_FALSE(variant.canConvert<QString>());
    EXPECT_TRUE(variant.toString().isNull());
    EXPECT_TRUE(variant.toString().isEmpty());
    EXPECT_EQ(variant.toString().size(), 0);

    // Check the columns
    EXPECT_EQ(prop.columns(), 2);
}

TEST_F(TestMaterialProperties, Test2DArray)
{
    Materials::MaterialProperty prop(modelProp, "sampleUUID");
    check2DArray(prop);
}

TEST_F(TestMaterialProperties, Test2DArrayCopy)
{
    Materials::MaterialProperty propBase(modelProp, "sampleUUID");
    Materials::MaterialProperty prop(propBase);
    check2DArray(prop);
}

TEST_F(TestMaterialProperties, Test2DArrayAssignment)
{
    Materials::MaterialProperty propBase(modelProp, "sampleUUID");
    Materials::MaterialProperty prop;

    prop = propBase;
    check2DArray(prop);
}

void check3DArray(Materials::MaterialProperty& prop)
{
    EXPECT_EQ(prop.getType(), Materials::MaterialValue::Array3D);
    EXPECT_EQ(prop.getModelUUID(), "sampleUUID");
    EXPECT_TRUE(prop.isNull());
    auto array = std::static_pointer_cast<Materials::Array3D>(prop.getMaterialValue());
    EXPECT_EQ(array->depth(), 0);
    auto variant = prop.getValue();  // Throw an error?
    EXPECT_FALSE(variant.canConvert<QString>());
    EXPECT_TRUE(variant.toString().isNull());
    EXPECT_TRUE(variant.toString().isEmpty());
    EXPECT_EQ(variant.toString().size(), 0);

    // Check the columns
    EXPECT_EQ(prop.columns(), 3);
}

TEST_F(TestMaterialProperties, Test3DArray)
{
    Materials::MaterialProperty prop(model3DProp, "sampleUUID");
    check3DArray(prop);
}

TEST_F(TestMaterialProperties, Test3DArrayCopy)
{
    Materials::MaterialProperty propBase(model3DProp, "sampleUUID");
    Materials::MaterialProperty prop(propBase);
    check3DArray(prop);
}

TEST_F(TestMaterialProperties, Test3DArrayAssignment)
{
    Materials::MaterialProperty propBase(model3DProp, "sampleUUID");
    Materials::MaterialProperty prop;

    prop = propBase;
    check3DArray(prop);
}

// clang-format off

// clang-format on
