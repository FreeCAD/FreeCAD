// libE57Format testing Copyright © 2022 Andy Maloney <asmaloney@gmail.com>
// SPDX-License-Identifier: MIT

#include "gtest/gtest.h"

#include "E57SimpleData.h"
#include "E57SimpleReader.h"
#include "E57SimpleWriter.h"

#include "Helpers.h"
#include "TestData.h"

TEST( SimpleDataHeader, InvalidPointRangeNodeType )
{
   e57::Data3D dataHeader;

   dataHeader.pointCount = 1;
   dataHeader.pointFields.pointRangeNodeType = e57::NumericalNodeType::Integer;

   E57_ASSERT_THROW( e57::Data3DPointsFloat pointsData( dataHeader ) );
}

TEST( SimpleDataHeader, InvalidAngleNodeType )
{
   e57::Data3D dataHeader;

   dataHeader.pointCount = 1;
   dataHeader.pointFields.angleNodeType = e57::NumericalNodeType::Integer;

   E57_ASSERT_THROW( e57::Data3DPointsFloat pointsData( dataHeader ) );
}

TEST( SimpleDataHeader, AutoSetNodeTypes )
{
   e57::Data3D dataHeader;

   dataHeader.pointCount = 1;

   // dataHeader.pointFields.pointRangeNodeType and dataHeader.pointFields.angleNodeType default to
   // Float but we are using a double structure. The constructor should correct these and set them
   // to Double.
   e57::Data3DPointsDouble pointsData( dataHeader );

   EXPECT_EQ( dataHeader.pointFields.pointRangeNodeType, e57::NumericalNodeType::Double );
   EXPECT_EQ( dataHeader.pointFields.angleNodeType, e57::NumericalNodeType::Double );
}

TEST( SimpleDataHeader, HeaderMinMaxFloat )
{
   e57::Data3D dataHeader;

   dataHeader.pointCount = 1;

   EXPECT_EQ( dataHeader.pointFields.pointRangeMinimum, e57::DOUBLE_MIN );
   EXPECT_EQ( dataHeader.pointFields.pointRangeMaximum, e57::DOUBLE_MAX );
   EXPECT_EQ( dataHeader.pointFields.angleMinimum, e57::DOUBLE_MIN );
   EXPECT_EQ( dataHeader.pointFields.angleMaximum, e57::DOUBLE_MAX );
   EXPECT_EQ( dataHeader.pointFields.timeMinimum, e57::DOUBLE_MIN );
   EXPECT_EQ( dataHeader.pointFields.timeMaximum, e57::DOUBLE_MAX );

   // This call should adjust our min/max for a variety of fields since we are using floats.
   e57::Data3DPointsFloat pointsData( dataHeader );

   EXPECT_EQ( dataHeader.pointFields.pointRangeMinimum, e57::FLOAT_MIN );
   EXPECT_EQ( dataHeader.pointFields.pointRangeMaximum, e57::FLOAT_MAX );
   EXPECT_EQ( dataHeader.pointFields.angleMinimum, e57::FLOAT_MIN );
   EXPECT_EQ( dataHeader.pointFields.angleMaximum, e57::FLOAT_MAX );
   EXPECT_EQ( dataHeader.pointFields.timeMinimum, e57::FLOAT_MIN );
   EXPECT_EQ( dataHeader.pointFields.timeMaximum, e57::FLOAT_MAX );
}

TEST( SimpleDataHeader, HeaderMinMaxDouble )
{
   e57::Data3D dataHeader;

   dataHeader.pointCount = 1;

   EXPECT_EQ( dataHeader.pointFields.pointRangeMinimum, e57::DOUBLE_MIN );
   EXPECT_EQ( dataHeader.pointFields.pointRangeMaximum, e57::DOUBLE_MAX );
   EXPECT_EQ( dataHeader.pointFields.angleMinimum, e57::DOUBLE_MIN );
   EXPECT_EQ( dataHeader.pointFields.angleMaximum, e57::DOUBLE_MAX );
   EXPECT_EQ( dataHeader.pointFields.timeMinimum, e57::DOUBLE_MIN );
   EXPECT_EQ( dataHeader.pointFields.timeMaximum, e57::DOUBLE_MAX );

   // This call should NOT adjust our min/max for a variety of fields since we are using doubles.
   e57::Data3DPointsDouble pointsData( dataHeader );

   EXPECT_EQ( dataHeader.pointFields.pointRangeMinimum, e57::DOUBLE_MIN );
   EXPECT_EQ( dataHeader.pointFields.pointRangeMaximum, e57::DOUBLE_MAX );
   EXPECT_EQ( dataHeader.pointFields.angleMinimum, e57::DOUBLE_MIN );
   EXPECT_EQ( dataHeader.pointFields.angleMaximum, e57::DOUBLE_MAX );
   EXPECT_EQ( dataHeader.pointFields.timeMinimum, e57::DOUBLE_MIN );
   EXPECT_EQ( dataHeader.pointFields.timeMaximum, e57::DOUBLE_MAX );
}

// Checks that the Data3D header and the the cartesianX FloatNode data are the same when read,
// written, and read again. https://github.com/asmaloney/libE57Format/issues/126
TEST( SimpleData, ReadWrite )
{
   e57::Reader *originalReader = nullptr;
   e57::E57Root originalFileHeader;
   e57::Data3D originalData3DHeader;
   e57::Data3DPointsDouble *originalPointsData = nullptr;

   // 1. Read in original file
   {
      E57_ASSERT_NO_THROW( originalReader = new e57::Reader(
                              TestData::Path() + "/self/ColouredCubeDouble.e57", {} ) );

      ASSERT_TRUE( originalReader->GetE57Root( originalFileHeader ) );
      ASSERT_TRUE( originalReader->ReadData3D( 0, originalData3DHeader ) );

      const uint64_t cNumPoints = originalData3DHeader.pointCount;

      originalPointsData = new e57::Data3DPointsDouble( originalData3DHeader );

      auto vectorReader =
         originalReader->SetUpData3DPointsData( 0, cNumPoints, *originalPointsData );

      const uint64_t cNumRead = vectorReader.read();

      vectorReader.close();

      EXPECT_EQ( cNumRead, cNumPoints );
   }

   // 2. Write it out again using the Data3D header from the reader
   {
      e57::WriterOptions options;
      options.guid = originalFileHeader.guid;

      e57::Writer *writer = nullptr;
      E57_ASSERT_NO_THROW( writer = new e57::Writer( "./ColouredCubeDoubleCopy.e57", options ) );

      writer->WriteData3DData( originalData3DHeader, *originalPointsData );

      delete writer;
   }

   e57::Reader *copyReader = nullptr;
   e57::E57Root copyFileHeader;
   e57::Data3D copyData3DHeader;

   // 3. Read in what we just wrote
   {
      E57_ASSERT_NO_THROW( copyReader = new e57::Reader( "./ColouredCubeDoubleCopy.e57", {} ) );

      ASSERT_TRUE( copyReader->GetE57Root( copyFileHeader ) );
      ASSERT_TRUE( copyReader->ReadData3D( 0, copyData3DHeader ) );

      const uint64_t cNumPoints = copyData3DHeader.pointCount;

      e57::Data3DPointsDouble copyPointsData( copyData3DHeader );

      auto vectorReader = copyReader->SetUpData3DPointsData( 0, cNumPoints, copyPointsData );

      const uint64_t cNumRead = vectorReader.read();

      vectorReader.close();

      EXPECT_EQ( cNumRead, cNumPoints );
   }

   // 4. Compare the header data
   EXPECT_EQ( originalData3DHeader.name, copyData3DHeader.name );
   EXPECT_EQ( originalData3DHeader.guid, copyData3DHeader.guid );

   EXPECT_EQ( originalData3DHeader.description, copyData3DHeader.description );

   EXPECT_EQ( originalData3DHeader.colorLimits, copyData3DHeader.colorLimits );

   EXPECT_EQ( originalData3DHeader.pointFields.cartesianXField,
              copyData3DHeader.pointFields.cartesianXField );
   EXPECT_EQ( originalData3DHeader.pointFields.cartesianYField,
              copyData3DHeader.pointFields.cartesianYField );
   EXPECT_EQ( originalData3DHeader.pointFields.cartesianZField,
              copyData3DHeader.pointFields.cartesianZField );
   EXPECT_EQ( originalData3DHeader.pointFields.cartesianInvalidStateField,
              copyData3DHeader.pointFields.cartesianInvalidStateField );

   EXPECT_EQ( originalData3DHeader.pointFields.sphericalRangeField,
              copyData3DHeader.pointFields.sphericalRangeField );
   EXPECT_EQ( originalData3DHeader.pointFields.sphericalAzimuthField,
              copyData3DHeader.pointFields.sphericalAzimuthField );
   EXPECT_EQ( originalData3DHeader.pointFields.sphericalElevationField,
              copyData3DHeader.pointFields.sphericalElevationField );
   EXPECT_EQ( originalData3DHeader.pointFields.sphericalInvalidStateField,
              copyData3DHeader.pointFields.sphericalInvalidStateField );

   EXPECT_EQ( originalData3DHeader.pointFields.pointRangeMinimum,
              copyData3DHeader.pointFields.pointRangeMinimum );
   EXPECT_EQ( originalData3DHeader.pointFields.pointRangeMaximum,
              copyData3DHeader.pointFields.pointRangeMaximum );

   EXPECT_EQ( originalData3DHeader.pointFields.pointRangeNodeType,
              copyData3DHeader.pointFields.pointRangeNodeType );
   EXPECT_EQ( originalData3DHeader.pointFields.pointRangeScale,
              copyData3DHeader.pointFields.pointRangeScale );

   EXPECT_EQ( originalData3DHeader.pointFields.angleMinimum,
              copyData3DHeader.pointFields.angleMinimum );
   EXPECT_EQ( originalData3DHeader.pointFields.angleMaximum,
              copyData3DHeader.pointFields.angleMaximum );
   EXPECT_EQ( originalData3DHeader.pointFields.angleScale,
              copyData3DHeader.pointFields.angleScale );

   EXPECT_EQ( originalData3DHeader.pointFields.rowIndexField,
              copyData3DHeader.pointFields.rowIndexField );
   EXPECT_EQ( originalData3DHeader.pointFields.rowIndexMaximum,
              copyData3DHeader.pointFields.rowIndexMaximum );

   EXPECT_EQ( originalData3DHeader.pointFields.columnIndexField,
              copyData3DHeader.pointFields.columnIndexField );
   EXPECT_EQ( originalData3DHeader.pointFields.columnIndexMaximum,
              copyData3DHeader.pointFields.columnIndexMaximum );

   EXPECT_EQ( originalData3DHeader.pointFields.returnIndexField,
              copyData3DHeader.pointFields.returnIndexField );
   EXPECT_EQ( originalData3DHeader.pointFields.returnCountField,
              copyData3DHeader.pointFields.returnCountField );
   EXPECT_EQ( originalData3DHeader.pointFields.returnMaximum,
              copyData3DHeader.pointFields.returnMaximum );

   EXPECT_EQ( originalData3DHeader.pointFields.timeStampField,
              copyData3DHeader.pointFields.timeStampField );
   EXPECT_EQ( originalData3DHeader.pointFields.isTimeStampInvalidField,
              copyData3DHeader.pointFields.isTimeStampInvalidField );
   EXPECT_EQ( originalData3DHeader.pointFields.timeMinimum,
              copyData3DHeader.pointFields.timeMinimum );
   EXPECT_EQ( originalData3DHeader.pointFields.timeMaximum,
              copyData3DHeader.pointFields.timeMaximum );

   EXPECT_EQ( originalData3DHeader.pointFields.intensityField,
              copyData3DHeader.pointFields.intensityField );
   EXPECT_EQ( originalData3DHeader.pointFields.isIntensityInvalidField,
              copyData3DHeader.pointFields.isIntensityInvalidField );
   EXPECT_EQ( originalData3DHeader.pointFields.intensityScale,
              copyData3DHeader.pointFields.intensityScale );

   EXPECT_EQ( originalData3DHeader.pointFields.colorRedField,
              copyData3DHeader.pointFields.colorRedField );
   EXPECT_EQ( originalData3DHeader.pointFields.colorGreenField,
              copyData3DHeader.pointFields.colorGreenField );
   EXPECT_EQ( originalData3DHeader.pointFields.colorBlueField,
              copyData3DHeader.pointFields.colorBlueField );
   EXPECT_EQ( originalData3DHeader.pointFields.isColorInvalidField,
              copyData3DHeader.pointFields.isColorInvalidField );

   EXPECT_EQ( originalData3DHeader.pointFields.normalXField,
              copyData3DHeader.pointFields.normalXField );
   EXPECT_EQ( originalData3DHeader.pointFields.normalYField,
              copyData3DHeader.pointFields.normalYField );
   EXPECT_EQ( originalData3DHeader.pointFields.normalZField,
              copyData3DHeader.pointFields.normalZField );

   EXPECT_EQ( originalData3DHeader.pointCount, copyData3DHeader.pointCount );

   // 5. Compare the cartesianX FloatNode data
   e57::CompressedVectorNode originalPointsStructureNode(
      originalReader->GetRawData3D().get( "/data3D/0/points" ) );
   e57::CompressedVectorNode copyPointsStructureNode(
      copyReader->GetRawData3D().get( "/data3D/0/points" ) );

   const e57::StructureNode originalProto( originalPointsStructureNode.prototype() );
   const e57::StructureNode copyProto( copyPointsStructureNode.prototype() );

   e57::FloatNode originalCartesianXNode( originalProto.get( "cartesianX" ) );
   e57::FloatNode copyCartesianXNode( copyProto.get( "cartesianX" ) );

   EXPECT_EQ( originalCartesianXNode.precision(), copyCartesianXNode.precision() );
   EXPECT_EQ( originalCartesianXNode.minimum(), copyCartesianXNode.minimum() );
   EXPECT_EQ( originalCartesianXNode.maximum(), copyCartesianXNode.maximum() );

   // 6. Cleanup
   delete originalPointsData;
   delete originalReader;
   delete copyReader;
}
