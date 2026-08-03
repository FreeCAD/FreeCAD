// SPDX-License-Identifier: Zlib

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
#pragma once

#include <array>
#include <string>

#include "FCGlobal.h"

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic,
// cppcoreguidelines-pro-bounds-constant-array-index, cppcoreguidelines-avoid-magic-numbers,
// readability-magic-numbers)

namespace Base
{

static constexpr size_t base64DecodeTableSize {256};

/// Returns the max bytes of a encoded base64 string
inline std::size_t base64_encode_size(std::size_t len)
{
    return 4 * ((len + 2) / 3);
}

/// Returns the max bytes of a decoded base64 binary string
inline std::size_t base64_decode_size(std::size_t len)
{
    return len / 4 * 3;
}

/** Encode input binary with base64
 * @param out: output buffer with minimum size of base64_encode(len)
 * appending new data.
 * @param in: input binary data
 * @param len: input length
 * @return The character count written to output.
 */
BaseExport std::size_t base64_encode(char* out, void const* in, std::size_t len);

/** Return the internal base64 decoding table
 *
 * The table maps from any 8-bit character to the decoded binary bits.
 * Valid base64 characters are mapped to the corresponding 6-bit binary
 * data. White space (space, tab, vtab, CR and LF) characters are mapped
 * to -2. Other invalid characters are mapped to -1.
 */
BaseExport std::array<const signed char, base64DecodeTableSize> base64_decode_table();

/** Decode the input base64 string into binary data
 * @param out: output buffer with minimum size of base64_encode(len)
 * appending new data.
 * @param in: input binary data
 * @param len: input length
 * @return Return a pair of output size and input read size. Compare the
 * read size to input size to check for error.
 */
BaseExport std::pair<std::size_t, std::size_t> base64_decode(void* out, char const*, std::size_t len);

/** Encode input binary into base64 string
 * @param out: output string. Note that the string is not cleared before
 *             adding new content.
 * @param in: input binary data
 * @param len: input length
 */
inline void base64_encode(std::string& out, void const* in, std::size_t len)
{
    std::size_t size = out.size();
    out.resize(size + base64_encode_size(len));
    len = base64_encode(&out[size], in, len);
    out.resize(size + len);
}

/** Encode input binary into base64 string
 * @param in: input binary data
 * @param len: input length
 * @return Return the base64 string.
 */
inline std::string base64_encode(void const* in, std::size_t len)
{
    std::string out;
    base64_encode(out, in, len);
    return out;
}

/** Decode base64 string into binary data
 * @param out: output binary data. Note that the data is not cleared before
 *             adding new content.
 * @param in: input base64 string
 * @param len: input length
 * @return Return the processed input length. Compare this with the
 * argument \c len to check for error.
 */
template<typename T>
inline std::size_t base64_decode(T& out, char const* in, std::size_t len)
{
    std::size_t size = out.size();
    out.resize(size + base64_decode_size(len));
    std::pair<std::size_t, std::size_t> res = base64_decode(&out[size], in, len);
    out.resize(size + res.first);
    return res.second;
}

/** Decode base64 string into binary data
 * @param out: output binary data. Note that the data is not cleared before
 *             adding new content.
 * @param str: input base64 string
 * @return Return the processed input length. Compare this with the
 * argument \c len to check for error.
 */
template<typename T>
inline std::size_t base64_decode(T& out, std::string const& str)
{
    return base64_decode(out, str.c_str(), str.size());
}

/** Decode base64 string into binary data
 * @param out  adding new content.
 * @param str: input base64 string
 * @return Return the decoded binary data.
 */
inline std::string base64_decode(std::string const& str)
{
    std::string out;
    base64_decode(out, str.c_str(), str.size());
    return out;
}

}  // namespace Base

// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic,
// cppcoreguidelines-pro-bounds-constant-array-index, cppcoreguidelines-avoid-magic-numbers,
// readability-magic-numbers)
