/*
Copyright (C) 2004-2008 René Nyffenegger

This source code is provided 'as-is', without any express or implied
warranty. In no event will the author be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this source code must not be misrepresented; you must not
   claim that you wrote the original source code. If you use this source code
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original source code.

3. This notice may not be removed or altered from any source distribution.

René Nyffenegger rene.nyffenegger@adp-gmbh.ch

NOTICE: This test has been modified from the original code to remove output to stdout, and to split
the tests into individual parts.

*/

#include "Base/Base64.h"

#include <gtest/gtest.h>

using namespace Base;

// NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)

TEST(Base64, encode)
{
    const std::string str = "René Nyffenegger\n"
                            "http://www.renenyffenegger.ch\n"
                            "passion for data\n";

    auto encoded = base64_encode(reinterpret_cast<const unsigned char*>(str.c_str()), str.length());
    auto decoded = base64_decode(std::string(encoded));

    ASSERT_EQ(decoded, str);
}

TEST(Base64, exactlyFourBytes)
{
    // Test all possibilities of fill bytes (none, one =, two ==)
    // References calculated with: https://www.base64encode.org/

    std::string rest0_original = "abc";
    // std::string rest0_reference = "YWJj";

    std::string rest0_encoded =
        base64_encode(reinterpret_cast<const unsigned char*>(rest0_original.c_str()),
                      rest0_original.length());
    std::string rest0_decoded = base64_decode(rest0_encoded);

    ASSERT_EQ(rest0_decoded, rest0_original);
}

TEST(Base64, twoEqualsSignsPadding)
{
    std::string rest1_original = "abcd";
    // std::string rest1_reference = "YWJjZA==";

    std::string rest1_encoded =
        base64_encode(reinterpret_cast<const unsigned char*>(rest1_original.c_str()),
                      rest1_original.length());
    std::string rest1_decoded = base64_decode(rest1_encoded);

    ASSERT_EQ(rest1_decoded, rest1_original);
}

TEST(Base64, oneEqualsSignPadding)
{
    std::string rest2_original = "abcde";
    // std::string rest2_reference = "YWJjZGU=";

    std::string rest2_encoded =
        base64_encode(reinterpret_cast<const unsigned char*>(rest2_original.c_str()),
                      rest2_original.length());
    std::string rest2_decoded = base64_decode(rest2_encoded);

    ASSERT_EQ(rest2_decoded, rest2_original);
}

// NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
