// libE57Format testing Copyright © 2025 Andy Maloney <asmaloney@gmail.com>
// SPDX-License-Identifier: BSL-1.0

#include <cstring>
#include <memory>

#include "gtest/gtest.h"

#include "E57Format.h"

#include "Helpers.h"
#include "TestData.h"

namespace
{
   // Checks that Data3D exists and is the correct type
   // outVectorNode contains the data3D vector node if successful
   void CheckData3DExists( const e57::StructureNode &inRootNode, e57::VectorNode &outVectorNode )
   {
      SCOPED_TRACE( __func__ );

      ASSERT_TRUE( inRootNode.isDefined( "data3D" ) ) << "root should contain 'data3D'";

      e57::Node data3DNode = inRootNode.get( "data3D" );
      ASSERT_EQ( data3DNode.type(), e57::TypeVector ) << "'data3D' should be a VectorNode";

      outVectorNode = static_cast<e57::VectorNode>( data3DNode );
   }

   // Checks that at least one scan exists
   // outScan0 contains the first scan if successful
   void CheckDataFirstScan( const e57::VectorNode &inData3DNode, e57::StructureNode &outScan0 )
   {
      SCOPED_TRACE( __func__ );

      ASSERT_GT( inData3DNode.childCount(), 0 ) << "'data3D' vector should not be empty";

      e57::Node scanNode = inData3DNode.get( 0 );
      ASSERT_EQ( scanNode.type(), e57::TypeStructure ) << "data3D[0] should be a StructureNode";

      outScan0 = static_cast<e57::StructureNode>( scanNode );

      ASSERT_TRUE( outScan0.isDefined( "points" ) ) << "Scan 0 should have 'points' defined";
   }

   // Checks that the scan's point structure has the correct types
   // outPrototype contains the scan's point prototype structure
   void CheckScanPoints( const e57::StructureNode &inScanNode, e57::StructureNode &outPrototype )
   {
      SCOPED_TRACE( __func__ );

      e57::Node pointsNode = inScanNode.get( "points" );
      ASSERT_EQ( pointsNode.type(), e57::TypeCompressedVector )
         << "'points' should be a CompressedVectorNode";

      e57::CompressedVectorNode points = static_cast<e57::CompressedVectorNode>( pointsNode );

      e57::Node prototypeNode = points.prototype();
      ASSERT_EQ( prototypeNode.type(), e57::TypeStructure )
         << "Prototype should be a StructureNode";

      outPrototype = static_cast<e57::StructureNode>( prototypeNode );
   }
}

// Checks StructureNode::isDefined() return values & exception handling
// See: https://github.com/asmaloney/libE57Format/issues/330
TEST( ImageFile, StructureNodeIsDefined )
{
   if ( !TestData::Exists() )
   {
      GTEST_SKIP_( "(test data not available)" );
   }

   std::unique_ptr<e57::ImageFile> imf;

   const std::string cFileName = TestData::Path() + "/reference/bunnyDouble.e57";
   E57_ASSERT_NO_THROW( imf = std::make_unique<e57::ImageFile>( cFileName, "r" ) );
   ASSERT_TRUE( imf->isOpen() ) << "Failed to open: " << cFileName;

   e57::VectorNode data3D( *imf );

   bool defined = false;

   // 1. Check that a request before we have done anything with the ImageFile returns false
   SCOPED_TRACE( "(1)" );
   defined = data3D.isDefined( "/foo/bar" );
   EXPECT_FALSE( defined );

   e57::StructureNode root = imf->root();
   e57::StructureNode scan0( *imf );
   e57::StructureNode prototype( *imf );

   SCOPED_TRACE( "(setup)" );
   CheckData3DExists( root, data3D );
   CheckDataFirstScan( data3D, scan0 );
   CheckScanPoints( scan0, prototype );

   // 2. Check that a request for a non-existent path returns false
   SCOPED_TRACE( "(2)" );
   defined = data3D.isDefined( "/foo/blat" );
   EXPECT_FALSE( defined );

   // 3. Check that a request for a non-existent path using an extension does not throw & returns
   // false
   SCOPED_TRACE( "(3)" );
   E57_ASSERT_NO_THROW( defined = prototype.isDefined( "nor:normalX" ) );
   EXPECT_FALSE( defined );

   // 4. Check that an empty path throws an exception
   SCOPED_TRACE( "(4)" );
   E57_ASSERT_THROW( defined = prototype.isDefined( "" ) );

   // 5. Check that a malformed path throws an exception
   SCOPED_TRACE( "(5)" );
   E57_ASSERT_THROW( defined = prototype.isDefined( "a:b:c:d" ) );
}

// Checks for out-of-bounds read when parsing a truncated in-memory E57 buffer
// See: https://github.com/asmaloney/libE57Format/pull/352
TEST( ImageFile, InvalidBufferLength )
{
   // 48 bytes: valid "ASTM-E57" magic followed by a header that ends early.
   static const unsigned char sInput[] = {
      0x41, 0x53, 0x54, 0x4d, 0x2d, 0x45, 0x35, 0x37, 0x01, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x4c, 0x69,
      0x62, 0x72, 0x61, 0x72, 0x79, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e,
      0x20, 0x74, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x92, 0xe4, 0x13,
   };

   // Exact-sized data allocation so the sanitizer can see the overrun
   char *data = new char[sizeof( sInput )];
   std::memcpy( data, sInput, sizeof( sInput ) );

   std::unique_ptr<e57::ImageFile> imf;

   E57_ASSERT_THROW( imf = std::make_unique<e57::ImageFile>( data, sizeof( sInput ) ) );

   delete[] data;
}

// Check that we throw if the file size is not an integral multiple of 1024 bytes
TEST( ImageFile, InvalidFileLength )
{
   if ( !TestData::Exists() )
   {
      GTEST_SKIP_( "(test data not available)" );
   }

   std::unique_ptr<e57::ImageFile> imf;

   const std::string cFileName = TestData::Path() + "/self/InvalidFileLength.e57";
   E57_ASSERT_THROW( imf = std::make_unique<e57::ImageFile>( cFileName, "r" ) );
}
