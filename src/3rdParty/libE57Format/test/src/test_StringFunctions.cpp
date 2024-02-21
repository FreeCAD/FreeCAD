// libE57Format testing Copyright © 2022 Andy Maloney <asmaloney@gmail.com>
// SPDX-License-Identifier: MIT

#include <clocale>

#include "gtest/gtest.h"

#include "StringFunctions.h"

TEST( StringFunctions, FloatToStrConversion )
{
   const auto converted = e57::floatingPointToStr<float>( 3.14159265f, 7 );

   ASSERT_EQ( converted, "3.1415927" );
}

// https://github.com/asmaloney/libE57Format/issues/147
TEST( StringFunctions, FloatToStrTrimDecimal )
{
   const auto converted = e57::floatingPointToStr<float>( 42.0f, 5 );

   ASSERT_EQ( converted, "4.2e+01" );
}

// https://github.com/asmaloney/libE57Format/issues/147
TEST( StringFunctions, FloatToStrTrimExponent )
{
   const auto converted = e57::floatingPointToStr<float>( 4.0f, 5 );

   ASSERT_EQ( converted, "4" );
}

TEST( StringFunctions, DoubleToStrConversion )
{
   const auto converted = e57::floatingPointToStr<double>( 3.141592653589793238, 17 );

   ASSERT_EQ( converted, "3.14159265358979312" );
}

TEST( StringFunctions, FloatToStrConversion2 )
{
   const auto converted = e57::floatingPointToStr<float>( 123456.0f, 7 );

   ASSERT_EQ( converted, "1.23456e+05" );
}

// Related to https://github.com/asmaloney/libE57Format/issues/172
// Floating point should always use '.'.
TEST( StringFunctions, FloatToStrLocale )
{
   try
   {
      std::locale::global( std::locale( "fr_FR" ) );
   }
   catch ( std::exception &err )
   {
      GTEST_SKIP() << "fr_FR locale not available: " << err.what();
   }

   std::wcout.imbue( std::locale() );

   const auto converted = e57::floatingPointToStr<float>( 123456.0f, 7 );

   ASSERT_EQ( converted, "1.23456e+05" );

   std::locale::global( std::locale::classic() );
}
