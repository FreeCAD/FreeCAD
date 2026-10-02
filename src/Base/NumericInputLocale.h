// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <cstddef>
#include <string_view>

#include <unicode/utf8.h>

namespace Base::NumericInputPrivate
{

bool decodeUtf8(std::string_view text, std::size_t position, UChar32& codePoint, std::size_t& consumedBytes);

bool isSpaceLike(UChar32 codePoint);

}  // namespace Base::NumericInputPrivate
