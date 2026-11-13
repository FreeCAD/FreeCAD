#include "gtest/gtest.h"

#include "Helpers.h"

namespace TestHelper
{
   void CheckFileHeader( const e57::E57Root &fileHeader )
   {
      // These are invariant.
      // See ASTM Standard Table 12.

      EXPECT_EQ( fileHeader.formatName, "ASTM E57 3D Imaging Data File" );
      EXPECT_EQ( fileHeader.versionMajor, 1 );
      EXPECT_EQ( fileHeader.versionMinor, 0 );
   }
}
