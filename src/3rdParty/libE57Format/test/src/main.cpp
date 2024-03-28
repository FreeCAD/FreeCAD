// libE57Format testing Copyright © 2022 Andy Maloney <asmaloney@gmail.com>
// SPDX-License-Identifier: MIT

#include "gtest/gtest.h"

#include "E57Version.h"

#include "RandomNum.h"
#include "TestData.h"

// Handle non-clang compilers
#if !defined( __has_feature )
#define __has_feature( feature ) 0
#endif

// Address Sanitizer
// There seems to be a false positive with std::vector, so turn off container overflow detection.
//   https://github.com/google/sanitizers/wiki/AddressSanitizerContainerOverflow#false-positives
//   https://github.com/google/sanitizers/wiki/AddressSanitizerFlags#run-time-flags
extern "C" const char *__asan_default_options()
{
   return "detect_container_overflow=false";
}

int main( int argc, char **argv )
{
   Random::seed( 42 );

   ::testing::FLAGS_gtest_color = "yes";
   ::testing::InitGoogleTest( &argc, argv );

   // IF our data path doesn't exist, then exclude some tests.
   if ( !TestData::Exists() )
   {
      ::testing::GTEST_FLAG( filter ) = "-*Data.*";
   }

   std::cout << "e57Format version: " << e57::Version::library() << std::endl;
   std::cout << "ASTM version: " << e57::Version::astm() << std::endl;

#if __has_feature( address_sanitizer ) || defined( __SANITIZE_ADDRESS__ )
   std::cout << " - Address Sanitizer (ASan) is active" << std::endl;
#endif

   // Note: There is currently no way to check for UBSan on gcc...
#if __has_feature( undefined_behavior_sanitizer )
   std::cout << " - Undefined Behaviour Sanitizer (UBSan) is active" << std::endl;
#endif

   int result = RUN_ALL_TESTS();

   return result;
}
