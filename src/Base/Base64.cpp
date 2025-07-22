/*
base64.cpp and base64.h

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

NOTICE: The source code here has been altered from the original to use a provided character buffer
rather than returning a new string for each call.
These modifications are Copyright (c) 2019 Zheng Lei (realthunder.dev@gmail.com)

*/
#include "PreCompiled.h"

#ifndef _PreComp_
#include <array>
#endif

#include "Base64.h"

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic,
// cppcoreguidelines-pro-bounds-constant-array-index, cppcoreguidelines-avoid-magic-numbers,
// readability-magic-numbers)

static const std::array<char, 65> base64_chars {"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                "abcdefghijklmnopqrstuvwxyz"
                                                "0123456789+/"};


std::array<const signed char, Base::base64DecodeTableSize> Base::base64_decode_table()
{
    static std::array<const signed char, Base::base64DecodeTableSize> _table = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -2, -2, -2, -1, -2, -1, -1,  //   0-15
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  //  16-31
        -2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,  //  32-47
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -2, -1, -1,  //  48-63
        -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,  //  64-79
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,  //  80-95
        -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,  //  96-111
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,  // 112-127
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 128-143
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 144-159
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 160-175
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 176-191
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 192-207
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 208-223
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 224-239
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1   // 240-255
    };
    return _table;
}

std::size_t Base::base64_encode(char* out, void const* in, std::size_t in_len)
{
    char* ret = out;
    auto const* bytes_to_encode = reinterpret_cast<unsigned char const*>(in);  // NOLINT
    int char3 {0};
    int char4 {};
    std::array<unsigned char, 3> char_array_3 {};
    std::array<unsigned char, 4> char_array_4 {};

    while ((in_len--) != 0U) {
        char_array_3[char3++] = *(bytes_to_encode++);
        if (char3 == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (char3 = 0; (char3 < 4); char3++) {
                *ret++ = base64_chars[char_array_4[char3]];
            }
            char3 = 0;
        }
    }

    if (char3 != 0) {
        for (char4 = char3; char4 < 3; char4++) {
            char_array_3[char4] = '\0';
        }

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (char4 = 0; (char4 < char3 + 1); char4++) {
            *ret++ = base64_chars[char_array_4[char4]];
        }

        while ((char3++ < 3)) {
            *ret++ = '=';
        }
    }

    return ret - out;
}

std::pair<std::size_t, std::size_t>
Base::base64_decode(void* _out, char const* in, std::size_t in_len)
{
    auto* out = reinterpret_cast<unsigned char*>(_out);  // NOLINT
    unsigned char* ret = out;
    char const* input = in;
    int byteCounter1 {0};
    int byteCounter2 {};
    std::array<unsigned char, 4> char_array_4 {};
    std::array<unsigned char, 3> char_array_3 {};

    static auto table = base64_decode_table();

    while (((in_len--) != 0U) && *in != '=') {
        const signed char lookup = table[static_cast<unsigned char>(*in++)];
        if (lookup < 0) {
            break;
        }

        char_array_4[byteCounter1++] = (unsigned char)lookup;
        if (byteCounter1 == 4) {
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (byteCounter1 = 0; (byteCounter1 < 3); byteCounter1++) {
                *ret++ = char_array_3[byteCounter1];
            }
            byteCounter1 = 0;
        }
    }

    if (byteCounter1 != 0) {
        for (byteCounter2 = byteCounter1; byteCounter2 < 4; byteCounter2++) {
            char_array_4[byteCounter2] = 0;
        }

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (byteCounter2 = 0; (byteCounter2 < byteCounter1 - 1); byteCounter2++) {
            *ret++ = char_array_3[byteCounter2];
        }
    }

    return std::make_pair((std::size_t)(ret - out), (std::size_t)(in - input));
}

// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic,
// cppcoreguidelines-pro-bounds-constant-array-index, cppcoreguidelines-avoid-magic-numbers,
// readability-magic-numbers)
