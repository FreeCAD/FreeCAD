// SPDX-License-Identifier: LGPL-2.1-or-later

#include "NumericInputLocale.h"

#include <cstdint>
#include <limits>

#include "NumericFormatting.h"
#include "NumericInput.h"

namespace Base::NumericInputPrivate
{

bool decodeUtf8(
    const std::string_view text,
    const std::size_t position,
    UChar32& codePoint,
    std::size_t& consumedBytes
)
{
    if (position >= text.size() || text.size() > std::numeric_limits<int32_t>::max()
        || position > static_cast<std::size_t>(std::numeric_limits<int32_t>::max())) {
        return false;
    }

    auto index = static_cast<int32_t>(position);
    const auto* bytes = reinterpret_cast<const uint8_t*>(text.data());
    U8_NEXT(bytes, index, static_cast<int32_t>(text.size()), codePoint);
    if (codePoint == U_SENTINEL || index <= static_cast<int32_t>(position)) {
        return false;
    }

    consumedBytes = static_cast<std::size_t>(index) - position;
    return true;
}

bool isSpaceLike(const UChar32 codePoint)
{
    return codePoint == 0x00A0 || codePoint == 0x2007 || codePoint == 0x2009 || codePoint == 0x202F;
}

}  // namespace Base::NumericInputPrivate

bool Base::localizedDigitAt(
    const std::string_view input,
    const std::size_t position,
    const NumericLocaleContext& locale,
    int& digit,
    std::size_t& consumedBytes
)
{
    digit = 0;
    consumedBytes = 0;
    if (position >= input.size()) {
        return false;
    }

    if (input[position] >= '0' && input[position] <= '9') {
        digit = input[position] - '0';
        consumedBytes = 1;
        return true;
    }

    UChar32 zeroCodePoint = 0;
    std::size_t zeroLength = 0;
    if (locale.zeroDigit.empty()
        || !NumericInputPrivate::decodeUtf8(locale.zeroDigit, 0, zeroCodePoint, zeroLength)
        || zeroLength != locale.zeroDigit.size()) {
        return false;
    }

    UChar32 codePoint = 0;
    std::size_t inputLength = 0;
    if (!NumericInputPrivate::decodeUtf8(input, position, codePoint, inputLength)) {
        return false;
    }

    if (codePoint < zeroCodePoint || codePoint > zeroCodePoint + 9) {
        return false;
    }

    digit = static_cast<int>(codePoint - zeroCodePoint);
    consumedBytes = inputLength;
    return true;
}
