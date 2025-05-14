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

#include <QMetaType>
#include <QString>

#include <App/Application.h>
#include <Gui/MetaTypes.h>

#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/Model.h>
#include <Mod/Material/App/ModelManager.h>
#include <Mod/Material/App/ModelUuids.h>

// clang-format off

class TestMaterialValue : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
  }
};

TEST_F(TestMaterialValue, TestNoneType)
{
    auto mat1 = Materials::MaterialValue();
    EXPECT_EQ(mat1.getType(), Materials::MaterialValue::None);
    EXPECT_TRUE(mat1.isNull());
    auto variant = mat1.getValue();
    EXPECT_TRUE(variant.isNull());
    EXPECT_FALSE(variant.canConvert<QString>());
    EXPECT_TRUE(variant.toString().isNull());
    EXPECT_TRUE(variant.toString().isEmpty());
    EXPECT_EQ(variant.toString().size(), 0);
}

TEST_F(TestMaterialValue, TestStringType)
{
    auto mat1 = Materials::MaterialValue(Materials::MaterialValue::String);
    EXPECT_EQ(mat1.getType(), Materials::MaterialValue::String);
    EXPECT_TRUE(mat1.isNull());
    auto variant = mat1.getValue();
    EXPECT_TRUE(variant.isNull());
    EXPECT_TRUE(variant.canConvert<QString>());
    EXPECT_TRUE(variant.toString().isNull());
    EXPECT_TRUE(variant.toString().isEmpty());
    EXPECT_EQ(variant.toString().size(), 0);
}

TEST_F(TestMaterialValue, TestBooleanType)
{
    auto mat1 = Materials::MaterialValue(Materials::MaterialValue::Boolean);
    EXPECT_EQ(mat1.getType(), Materials::MaterialValue::Boolean);
    EXPECT_TRUE(mat1.isNull());
    auto variant = mat1.getValue();
    EXPECT_TRUE(variant.isNull());
    EXPECT_TRUE(variant.canConvert<bool>());
    EXPECT_FALSE(variant.toString().isNull());
    EXPECT_FALSE(variant.toString().isEmpty());
    EXPECT_EQ(variant.toString().size(), 5);
    EXPECT_EQ(variant.toString(), QStringLiteral("false"));
    EXPECT_EQ(variant.toBool(), false);
}

TEST_F(TestMaterialValue, TestIntegerType)
{
    auto mat1 = Materials::MaterialValue(Materials::MaterialValue::Integer);
    EXPECT_EQ(mat1.getType(), Materials::MaterialValue::Integer);
    EXPECT_TRUE(mat1.isNull());
    auto variant = mat1.getValue();
    EXPECT_TRUE(variant.isNull());
    EXPECT_TRUE(variant.canConvert<int>());
    EXPECT_FALSE(variant.toString().isNull());
    EXPECT_FALSE(variant.toString().isEmpty());
    EXPECT_EQ(variant.toString().size(), 1);
    EXPECT_EQ(variant.toString(), QStringLiteral("0"));
    EXPECT_EQ(variant.toInt(), 0);
}

TEST_F(TestMaterialValue, TestFloatType)
{
    auto mat1 = Materials::MaterialValue(Materials::MaterialValue::Float);
    EXPECT_EQ(mat1.getType(), Materials::MaterialValue::Float);
    EXPECT_TRUE(mat1.isNull());
    auto variant = mat1.getValue();
    EXPECT_TRUE(variant.isNull());
    EXPECT_TRUE(variant.canConvert<float>());
    EXPECT_FALSE(variant.toString().isNull());
    EXPECT_FALSE(variant.toString().isEmpty());
    EXPECT_EQ(variant.toString().size(), 1);
    EXPECT_EQ(variant.toString(), QStringLiteral("0"));
    EXPECT_EQ(variant.toFloat(), 0);
}

TEST_F(TestMaterialValue, TestQuantityType)
{
    auto mat1 = Materials::MaterialValue(Materials::MaterialValue::Quantity);
    EXPECT_EQ(mat1.getType(), Materials::MaterialValue::Quantity);
    EXPECT_TRUE(mat1.isNull());

    auto variant = mat1.getValue();
    EXPECT_FALSE(variant.isNull());
    EXPECT_TRUE(variant.canConvert<Base::Quantity>());
    EXPECT_TRUE(variant.toString().isNull());
    EXPECT_TRUE(variant.toString().isEmpty());
    EXPECT_EQ(variant.toString().size(), 0);
    auto quantity = variant.value<Base::Quantity>();
    EXPECT_FALSE(quantity.isValid());
    EXPECT_EQ(quantity.getUserString(), "nan");
    EXPECT_TRUE(std::isnan(quantity.getValue()));

    // Test a copy
    auto mat2 = Materials::MaterialValue(mat1);
    EXPECT_EQ(mat2.getType(), Materials::MaterialValue::Quantity);
    EXPECT_TRUE(mat2.isNull());

    variant = mat2.getValue();
    EXPECT_FALSE(variant.isNull());
    EXPECT_TRUE(variant.canConvert<Base::Quantity>());
    EXPECT_TRUE(variant.toString().isNull());
    EXPECT_TRUE(variant.toString().isEmpty());
    EXPECT_EQ(variant.toString().size(), 0);
    quantity = variant.value<Base::Quantity>();
    EXPECT_FALSE(quantity.isValid());
    EXPECT_EQ(quantity.getUserString(), "nan");
    EXPECT_TRUE(std::isnan(quantity.getValue()));
}

TEST_F(TestMaterialValue, TestListType)
{
    auto mat1 = Materials::MaterialValue(Materials::MaterialValue::List);
    EXPECT_EQ(mat1.getType(), Materials::MaterialValue::List);
    EXPECT_TRUE(mat1.isNull());
    auto variant = mat1.getValue();
    EXPECT_TRUE(variant.value<QList<QVariant>>().isEmpty());
    EXPECT_EQ(variant.value<QList<QVariant>>().size(), 0);
    EXPECT_FALSE(variant.isNull());
    EXPECT_FALSE(variant.canConvert<QVariant>());
    EXPECT_TRUE(variant.toString().isNull());
    EXPECT_TRUE(variant.toString().isEmpty());
    EXPECT_EQ(variant.toString().size(), 0);
    auto list = mat1.getList();
    EXPECT_TRUE(list.isEmpty());
    EXPECT_EQ(list.size(), 0);
}

TEST_F(TestMaterialValue, TestArray2DType)
{
    EXPECT_THROW(auto mat1 = Materials::MaterialValue(Materials::MaterialValue::Array2D), Materials::InvalidMaterialType);

    auto mat2 = Materials::Array2D();
    EXPECT_EQ(mat2.getType(), Materials::MaterialValue::Array2D);
    EXPECT_TRUE(mat2.isNull());
    EXPECT_EQ(mat2.rows(), 0);
}

TEST_F(TestMaterialValue, TestArray3DType)
{
    EXPECT_THROW(auto mat1 = Materials::MaterialValue(Materials::MaterialValue::Array3D), Materials::InvalidMaterialType);

    auto mat2 = Materials::Array3D();
    mat2.setColumns(2);
    EXPECT_EQ(mat2.getType(), Materials::MaterialValue::Array3D);
    EXPECT_TRUE(mat2.isNull());

    EXPECT_EQ(mat2.depth(), 0);
    EXPECT_EQ(mat2.rows(), 0);
    EXPECT_EQ(mat2.rows(0), 0);
    EXPECT_THROW(mat2.rows(1), Materials::InvalidIndex);

    Base::Quantity quantity;
    quantity.setInvalid();

    EXPECT_EQ(mat2.addDepth(0, quantity), 0);
    EXPECT_EQ(mat2.depth(), 1);
    EXPECT_EQ(mat2.rows(0), 0);
    EXPECT_THROW(mat2.rows(1), Materials::InvalidIndex);

    EXPECT_EQ(mat2.addDepth(quantity), 1);
    EXPECT_EQ(mat2.depth(), 2);
    EXPECT_EQ(mat2.rows(1), 0);

    EXPECT_THROW(mat2.addDepth(99, quantity), Materials::InvalidIndex);
    EXPECT_EQ(mat2.addDepth(2, quantity), 2);
    EXPECT_EQ(mat2.depth(), 3);
    EXPECT_EQ(mat2.rows(2), 0);

    // Add rows
    auto row = std::make_shared<QList<Base::Quantity>>();
    row->push_back(quantity);
    row->push_back(quantity);

    EXPECT_EQ(mat2.rows(0), 0);
    EXPECT_EQ(mat2.rows(1), 0);
    EXPECT_EQ(mat2.rows(2), 0);

    mat2.addRow(0, row);
    EXPECT_EQ(mat2.rows(0), 1);
    EXPECT_EQ(mat2.rows(1), 0);
    EXPECT_EQ(mat2.rows(2), 0);

    mat2.addRow(1, row);
    EXPECT_EQ(mat2.rows(0), 1);
    EXPECT_EQ(mat2.rows(1), 1);
    EXPECT_EQ(mat2.rows(2), 0);

    mat2.addRow(2, row);
    EXPECT_EQ(mat2.rows(0), 1);
    EXPECT_EQ(mat2.rows(1), 1);
    EXPECT_EQ(mat2.rows(2), 1);

    EXPECT_EQ(mat2.currentDepth(), 0);
    mat2.addRow(row);
    EXPECT_EQ(mat2.rows(0), 2);
    EXPECT_EQ(mat2.rows(1), 1);
    EXPECT_EQ(mat2.rows(2), 1);

    mat2.setCurrentDepth(2);
    EXPECT_EQ(mat2.currentDepth(), 2);
    mat2.addRow(row);
    EXPECT_EQ(mat2.rows(0), 2);
    EXPECT_EQ(mat2.rows(1), 1);
    EXPECT_EQ(mat2.rows(2), 2);

    quantity = Base::Quantity::parse("32 C");
    mat2.setDepthValue(quantity);
    EXPECT_FALSE(mat2.getDepthValue(0).isValid());
    EXPECT_FALSE(mat2.getDepthValue(1).isValid());
    EXPECT_TRUE(mat2.getDepthValue(2).isValid());
    EXPECT_EQ(mat2.getDepthValue(2), Base::Quantity::parse("32 C"));

    mat2.setDepthValue(0, Base::Quantity::parse("9.8 m/s/s"));
    EXPECT_TRUE(mat2.getDepthValue(0).isValid());
    EXPECT_FALSE(mat2.getDepthValue(1).isValid());
    EXPECT_TRUE(mat2.getDepthValue(2).isValid());
    EXPECT_EQ(mat2.getDepthValue(0), Base::Quantity::parse("9.8 m/s/s"));
    EXPECT_EQ(mat2.getDepthValue(2), Base::Quantity::parse("32 C"));

    mat2.setDepthValue(1, Base::Quantity::parse("120 MPa"));
    EXPECT_TRUE(mat2.getDepthValue(0).isValid());
    EXPECT_TRUE(mat2.getDepthValue(1).isValid());
    EXPECT_TRUE(mat2.getDepthValue(2).isValid());
    EXPECT_EQ(mat2.getDepthValue(0), Base::Quantity::parse("9.8 m/s/s"));
    EXPECT_EQ(mat2.getDepthValue(1), Base::Quantity::parse("120 MPa"));
    EXPECT_EQ(mat2.getDepthValue(2), Base::Quantity::parse("32 C"));

    // Rows are currently empty
    EXPECT_THROW(mat2.getValue(2, 0), Materials::InvalidIndex);
    EXPECT_NO_THROW(mat2.getValue(0, 0));
    EXPECT_FALSE(mat2.getValue(0, 0).isValid());
    EXPECT_FALSE(mat2.getValue(0, 1).isValid());

    // set to a valid quantity
    mat2.setValue(0, 0, Base::Quantity::parse("120 MPa"));
    EXPECT_TRUE(mat2.getValue(0, 0).isValid());
    mat2.setValue(0, 1, Base::Quantity::parse("9.8 m/s/s"));
    EXPECT_TRUE(mat2.getValue(0, 1).isValid());
    EXPECT_THROW(mat2.setValue(0, 2, Base::Quantity::parse("32 C")), Materials::InvalidIndex);
}

// clang-format on
