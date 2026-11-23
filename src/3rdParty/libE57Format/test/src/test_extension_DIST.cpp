// libE57Format testing Copyright © 2022 Andy Maloney <asmaloney@gmail.com>
// SPDX-License-Identifier: BSL-1.0

#include <fstream>

#include "gtest/gtest.h"

#include "E57SimpleReader.h"
#include "E57SimpleWriter.h"

#include "Helpers.h"
#include "TestData.h"

TEST( Extension_DIST, ReadPinholeImageWithDistortionParameters )
{
   e57::Reader *reader = nullptr;

   E57_ASSERT_NO_THROW(
      reader = new e57::Reader( TestData::Path() + "/self/PinholeImageWithDistortionParameters.e57",
                                {} ) );

   ASSERT_TRUE( reader->IsOpen() );
   EXPECT_EQ( reader->GetImage2DCount(), 3 );
   EXPECT_EQ( reader->GetData3DCount(), 0 );

   e57::E57Root fileHeader;
   ASSERT_TRUE( reader->GetE57Root( fileHeader ) );

   TestHelper::CheckFileHeader( fileHeader );
   EXPECT_EQ( fileHeader.guid, "Pinhole image with distortion parameters GUID" );

   {
      // test reading image with distortion parameters set
      e57::Image2D image2DHeader;
      ASSERT_TRUE( reader->ReadImage2D( 0, image2DHeader ) );

      EXPECT_EQ( image2DHeader.pinholeRepresentation.imageWidth, 1 );
      EXPECT_EQ( image2DHeader.pinholeRepresentation.imageHeight, 1 );

      const e57::Extension::PinholeCameraDistortion *pcd =
         image2DHeader.pinholeCameraDistortionExt.get();
      EXPECT_TRUE( pcd );
      EXPECT_EQ( pcd->cameraNumber, 1 );
      EXPECT_EQ( pcd->type, "Testing type" );
      EXPECT_FLOAT_EQ( pcd->CV_K1, 1.01 );
      EXPECT_FLOAT_EQ( pcd->CV_K2, 2.02 );
      EXPECT_FLOAT_EQ( pcd->CV_K3, 3.03 );
      EXPECT_FLOAT_EQ( pcd->CV_K4, 4.04 );
      EXPECT_FLOAT_EQ( pcd->CV_K5, 5.05 );
      EXPECT_FLOAT_EQ( pcd->CV_K6, 6.06 );
      EXPECT_FLOAT_EQ( pcd->CV_P1, 11.11 );
      EXPECT_FLOAT_EQ( pcd->CV_P2, 12.12 );
      EXPECT_FLOAT_EQ( pcd->CV_CX, 21.21 );
      EXPECT_FLOAT_EQ( pcd->CV_CY, 22.22 );
      EXPECT_FLOAT_EQ( pcd->CV_FX, 31.31 );
      EXPECT_FLOAT_EQ( pcd->CV_FY, 32.32 );
      EXPECT_EQ( pcd->CV_HEIGHT, 1 );
      EXPECT_EQ( pcd->CV_WIDTH, 1 );
   }

   {
      // test reading image with some distortion parameters omitted
      e57::Image2D image2DHeader;
      EXPECT_TRUE( reader->ReadImage2D( 1, image2DHeader ) );

      EXPECT_EQ( image2DHeader.pinholeRepresentation.imageWidth, 1 );
      EXPECT_EQ( image2DHeader.pinholeRepresentation.imageHeight, 1 );

      const e57::Extension::PinholeCameraDistortion *pcd =
         image2DHeader.pinholeCameraDistortionExt.get();
      EXPECT_TRUE( pcd );
      EXPECT_EQ( pcd->cameraNumber, 2 );
      EXPECT_EQ( pcd->type, "" );
      EXPECT_FLOAT_EQ( pcd->CV_K1, 1.01 );
      EXPECT_FLOAT_EQ( pcd->CV_K2, 2.02 );
      EXPECT_FLOAT_EQ( pcd->CV_K3, 0 );
      EXPECT_FLOAT_EQ( pcd->CV_K4, 0 );
      EXPECT_FLOAT_EQ( pcd->CV_K5, 0 );
      EXPECT_FLOAT_EQ( pcd->CV_K6, 0 );
      EXPECT_FLOAT_EQ( pcd->CV_P1, 0 );
      EXPECT_FLOAT_EQ( pcd->CV_P2, 0 );
      EXPECT_FLOAT_EQ( pcd->CV_CX, 0 );
      EXPECT_FLOAT_EQ( pcd->CV_CY, 0 );
      EXPECT_FLOAT_EQ( pcd->CV_FX, 0 );
      EXPECT_FLOAT_EQ( pcd->CV_FY, 0 );
      EXPECT_EQ( pcd->CV_HEIGHT, 1 );
      EXPECT_EQ( pcd->CV_WIDTH, 1 );
   }

   {
      // test reading image without distortion header
      e57::Image2D image2DHeader;
      EXPECT_TRUE( reader->ReadImage2D( 2, image2DHeader ) );

      EXPECT_EQ( image2DHeader.pinholeRepresentation.imageWidth, 1 );
      EXPECT_EQ( image2DHeader.pinholeRepresentation.imageHeight, 1 );

      EXPECT_FALSE( image2DHeader.pinholeCameraDistortionExt );
   }

   delete reader;
}

TEST( Extension_DIST, WritePinholeImageWithDistortionParameters )
{
   e57::WriterOptions options;
   options.guid = "Pinhole image with distortion parameters GUID";

   e57::Writer *writer = nullptr;

   E57_ASSERT_NO_THROW(
      writer = new e57::Writer( "./PinholeImageWithDistortionParameters.e57", options ) );

   std::ifstream image( TestData::Path() + "/images/dummyImage.jpg",
                        std::ifstream::ate | std::ifstream::binary );

   ASSERT_EQ( image.rdstate(), std::ios_base::goodbit );

   const int64_t cImageSize = image.tellg();

   image.clear();
   image.seekg( 0 );

   auto imageBuffer = new char[cImageSize];

   image.read( imageBuffer, cImageSize );

   ASSERT_EQ( image.rdstate(), std::ios_base::goodbit );

   {
      // test writing image with all parameters set
      e57::Image2D image2DHeader;
      image2DHeader.name = "JPEG Image Test 1";
      image2DHeader.guid = "Pinhole Image 1 - JPEG Image GUID";
      image2DHeader.description = "JPEG image test 1 - distortion with all parameters specified";
      image2DHeader.pinholeRepresentation.imageWidth = 1;
      image2DHeader.pinholeRepresentation.imageHeight = 1;
      image2DHeader.pinholeRepresentation.jpegImageSize = cImageSize;

      auto &pcd = image2DHeader.pinholeCameraDistortionExt;
      pcd = std::make_unique<e57::Extension::PinholeCameraDistortion>();

      pcd->cameraNumber = 1;
      pcd->type = "Testing type";
      pcd->CV_K1 = 1.01;
      pcd->CV_K2 = 2.02;
      pcd->CV_K3 = 3.03;
      pcd->CV_K4 = 4.04;
      pcd->CV_K5 = 5.05;
      pcd->CV_K6 = 6.06;
      pcd->CV_P1 = 11.11;
      pcd->CV_P2 = 12.12;
      pcd->CV_CX = 21.21;
      pcd->CV_CY = 22.22;
      pcd->CV_FX = 31.31;
      pcd->CV_FY = 32.32;
      pcd->CV_HEIGHT = 1;
      pcd->CV_WIDTH = 1;

      std::size_t bytesWritten = 0;
      E57_ASSERT_NO_THROW( bytesWritten = writer->WriteImage2DData( image2DHeader, e57::ImageJPEG,
                                                                    e57::ProjectionPinhole, 0,
                                                                    imageBuffer, cImageSize ); );

      ASSERT_EQ( bytesWritten, cImageSize );
   }

   {
      // test writing image with some parameters omitted
      e57::Image2D image2DHeader;
      image2DHeader.name = "JPEG Image Test 2";
      image2DHeader.guid = "Pinhole Image 2 - JPEG Image GUID";
      image2DHeader.description = "JPEG image test 2 - distortion with some parameters omitted";
      image2DHeader.pinholeRepresentation.imageWidth = 1;
      image2DHeader.pinholeRepresentation.imageHeight = 1;
      image2DHeader.pinholeRepresentation.jpegImageSize = cImageSize;

      auto &pcd = image2DHeader.pinholeCameraDistortionExt;
      pcd = std::make_unique<e57::Extension::PinholeCameraDistortion>();
      pcd->cameraNumber = 2;
      pcd->CV_K1 = 1.01;
      pcd->CV_K2 = 2.02;
      pcd->CV_HEIGHT = 1;
      pcd->CV_WIDTH = 1;

      std::size_t bytesWritten = 0;
      E57_ASSERT_NO_THROW( bytesWritten = writer->WriteImage2DData( image2DHeader, e57::ImageJPEG,
                                                                    e57::ProjectionPinhole, 0,
                                                                    imageBuffer, cImageSize ); );

      ASSERT_EQ( bytesWritten, cImageSize );
   }

   {
      // test writing image without distortion header
      e57::Image2D image2DHeader;
      image2DHeader.name = "JPEG Image Test 3";
      image2DHeader.guid = "Pinhole Image 3 - JPEG Image GUID";
      image2DHeader.description = "JPEG image test 3 - without distortion header";
      image2DHeader.pinholeRepresentation.imageWidth = 1;
      image2DHeader.pinholeRepresentation.imageHeight = 1;
      image2DHeader.pinholeRepresentation.jpegImageSize = cImageSize;

      std::size_t bytesWritten = 0;
      E57_ASSERT_NO_THROW( bytesWritten = writer->WriteImage2DData( image2DHeader, e57::ImageJPEG,
                                                                    e57::ProjectionPinhole, 0,
                                                                    imageBuffer, cImageSize ); );

      ASSERT_EQ( bytesWritten, cImageSize );
   }

   delete[] imageBuffer;

   delete writer;
}
