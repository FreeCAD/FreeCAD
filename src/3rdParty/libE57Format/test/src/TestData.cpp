// libE57Format testing Copyright © 2022 Andy Maloney <asmaloney@gmail.com>
// SPDX-License-Identifier: MIT

#include <fstream>
#include <sys/stat.h>

#include "gtest/gtest.h"

#include "TestData.h"

namespace
{
   bool dirExists( const std::string &inPath )
   {
      struct stat info = {};

      if ( stat( inPath.c_str(), &info ) != 0 )
      {
         return false;
      }

      return info.st_mode & S_IFDIR;
   }
}

namespace TestData
{
#ifndef TEST_DATA_PATH
#pragma message( "warning: Test data not found. Some tests will not be run." )
#define TEST_DATA_PATH "DATA_NOT_FOUND"
#endif

   static const std::string sPath( TEST_DATA_PATH );

   const std::string &Path()
   {
      return sPath;
   }

   bool Exists()
   {
      return dirExists( TestData::Path() );
   }
}

TEST( TestData, RepoExists )
{
   ASSERT_TRUE( dirExists( TestData::Path() ) ) << "Data path: " << TestData::Path();
}
