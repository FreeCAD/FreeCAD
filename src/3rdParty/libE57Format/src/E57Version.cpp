// SPDX-License-Identifier: MIT
// Copyright 2020 Andy Maloney <asmaloney@gmail.com>

#include <sstream>

#include "ASTMVersion.h"
#include "E57Version.h"

namespace e57
{
   // REVISION_ID should be passed from compiler command line
#ifndef REVISION_ID
#error "Need to specify REVISION_ID on command line"
#endif

   std::string Version::astm()
   {
      std::ostringstream stringStream;
      stringStream << E57_FORMAT_MAJOR << "." << E57_FORMAT_MINOR;
      return stringStream.str();
   }

   uint32_t Version::astmMajor()
   {
      return E57_FORMAT_MAJOR;
   }

   uint32_t Version::astmMinor()
   {
      return E57_FORMAT_MINOR;
   }

   std::string Version::library()
   {
      return REVISION_ID;
   }

   void Version::get( uint32_t &astmMajor, uint32_t &astmMinor, std::string &libraryId )
   {
      astmMajor = E57_FORMAT_MAJOR;
      astmMinor = E57_FORMAT_MINOR;
      libraryId = REVISION_ID;
   }

   namespace Utilities
   {
      void getVersions( int &astmMajor, int &astmMinor, std::string &libraryId )
      {
         astmMajor = E57_FORMAT_MAJOR;
         astmMinor = E57_FORMAT_MINOR;
         libraryId = REVISION_ID;
      }
   }

}
