#pragma once
// Copyright © 2022 Andy Maloney <asmaloney@gmail.com>
// SPDX-License-Identifier: MIT

/// @file E57Version.h ASTM & libE57Format version information.

#include <string>

#include "E57Export.h"

namespace e57
{
   namespace Version
   {
      /*!
      @brief Get the version of the ASTM E57 standard that libE57Format supports.

      @returns The version as a string (e.g. "1.0")
      */
      E57_DLL std::string astm();

      /*!
      @brief Get the major version of the ASTM E57 standard that libE57Format supports.

      @returns The major version
      */
      E57_DLL uint32_t astmMajor();

      /*!
      @brief Get the minor version of the ASTM E57 standard that libE57Format supports.

      @returns The minor version
      */
      E57_DLL uint32_t astmMinor();

      /*!
      @brief Get the version of libE57Format library.

      @returns The version as a string (e.g. "E57Format-3.0.0-x86_64-darwin-AppleClang140").
      */
      E57_DLL std::string library();

      /*!
      @brief Get the version of ASTM E57 standard that the API implementation supports, and library
      id string.

      @param [out] astmMajor The major version number of the ASTM E57 standard supported.
      @param [out] astmMinor The minor version number of the ASTM E57 standard supported.
      @param [out] libraryId A string identifying the implementation of the API.

      @details
      Since the E57 implementation may be dynamically linked underneath the API, the version string
      for the implementation and the ASTM version that it supports can't be determined at
      compile-time. This function returns these identifiers from the underlying implementation.

      @throw No E57Exceptions.
      */
      E57_DLL void get( uint32_t &astmMajor, uint32_t &astmMinor, std::string &libraryId );
   }

   namespace Utilities
   {
      /*!
      @copydetails Version::get()
      @deprecated Will be removed in 4.0. Use Version::get() or other functions in the Version
      namespace.
      */
      [[deprecated( "Will be removed in 4.0. Use Version::get() or other functions in the Version "
                    "namespace." )]] E57_DLL void
         getVersions( int &astmMajor, int &astmMinor, std::string &libraryId );
   }
}
