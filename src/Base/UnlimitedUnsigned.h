// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2025 Kevin Martin <kpmartin@papertrail.ca>              *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 ***************************************************************************/


#pragma once

#ifndef FC_GLOBAL_H
# include <FCGlobal.h>
#endif
#include <vector>
#include <string>
#include <stdexcept>

// ----------------------------------------------------------------------------

namespace Base
{
class UnlimitedUnsigned
{
private:
    // This should be a signed type so we can get transient negative values during calculations
    // without resorting to a larger type
    using PartType = int32_t;
    // The following must be a type that limits the value to >= 0 and < maxPartPlusOne
    using SmallDeltaType = unsigned char;
    explicit UnlimitedUnsigned(std::vector<PartType>&& partsVector)
        : parts(std::move(partsVector))
    {}
    // We take a really cheap floor(bitcount * log10(max PartType)). 3/10 is just a bit less
    // than log10(2) The value should be sizeof(PartType) * CHAR_BIT * 3 / 10 but we can't
    // calculate the corresponding maxPartPlusOne in a static const initializer, so we just wire
    // in the values for 4-byte PartType.
    static const size_t partDigitCount = 9;
    static const PartType maxPartPlusOne = 1000000000;  // (PartType)pow(10, partDigitCount); but
                                                        // can't call pow in a const ctor.

public:
    explicit UnlimitedUnsigned(SmallDeltaType value)
        : parts(std::vector<PartType>(1, value))
    {}
    // Parse a decimal digit string into an UnlimitedUnsigned. There should be no white space,
    // only digits. a zero-length string parses as zero.
    static UnlimitedUnsigned fromString(const std::string& text)
    {
        std::vector<PartType> result((text.size() + partDigitCount - 1) / partDigitCount);
        size_t minimumSize = 0;
        size_t lastStartPosition = text.size();
        // Parse chunks of digits starting with the least significant
        for (size_t i = 0; i < result.size(); i++) {
            if (partDigitCount >= lastStartPosition) {
                // partial chunk of leading digits
                result[i] = static_cast<PartType>(std::stoul(text.substr(0, lastStartPosition)));
            }
            else {
                lastStartPosition -= partDigitCount;
                result[i] = static_cast<PartType>(
                    std::stoul(text.substr(lastStartPosition, partDigitCount))
                );
            }
            if (result[i] != 0) {
                minimumSize = i + 1;
            }
        }
        result.resize(minimumSize);
        return UnlimitedUnsigned(std::move(result));
    }
    std::string toString()
    {
        std::string result;
        result.reserve(parts.size() * partDigitCount);
        // Format the chunks from most- to least-significant
        for (auto it = parts.rbegin(); it != parts.rend(); it++) {
            std::string partDigits = std::to_string(*it);
            if (!result.empty()) {
                size_t padding = partDigitCount - partDigits.size();
                if (padding > 0) {
                    result += std::string(padding, '0');
                }
            }
            result += partDigits;
        }
        return result;
    }
    // TODO: Some allocation of vector contents could be avoided by defining methods like
    // static UnlimitedUnsigned operator+(UnlimitedUnsigned&& left, const UnlimitedUnsigned&
    // right); static UnlimitedUnsigned operator+(const UnlimitedUnsigned& left,
    // UnlimitedUnsigned&& right); static UnlimitedUnsigned operator+(UnlimitedUnsigned&& left,
    // UnlimitedUnsigned&& right); which would reuse left.parts or right.parts after possibly
    // growing it. The last one would use the larger of the two buffers.
    UnlimitedUnsigned operator+(const UnlimitedUnsigned& right) const
    {
        size_t resultSize = std::max(parts.size(), right.parts.size());
        std::vector<PartType> result(resultSize);
        PartType carry = 0;
        for (size_t i = 0; i < resultSize; i++) {
            auto newPart = (i < parts.size() ? parts[i] : 0)
                + (i < right.parts.size() ? right.parts[i] : 0) + carry;
            if (newPart < maxPartPlusOne) {
                carry = 0;
            }
            else {
                carry = 1;
                newPart -= maxPartPlusOne;
            }
            result[i] = newPart;
        }
        if (carry > 0) {
            result.resize(resultSize + 1);
            result[resultSize] = carry;
        }
        return UnlimitedUnsigned(std::move(result));
    }
    UnlimitedUnsigned operator+(SmallDeltaType right) const
    {
        size_t resultSize = parts.size();
        std::vector<PartType> result(resultSize);
        PartType carry = right;
        for (size_t i = 0; i < resultSize; i++) {
            auto newPart = parts[i] + carry;
            if (newPart < maxPartPlusOne) {
                carry = 0;
            }
            else {
                carry = 1;
                newPart -= maxPartPlusOne;
            }
            result[i] = newPart;
        }
        if (carry > 0) {
            result.resize(resultSize + 1);
            result[resultSize] = carry;
        }
        return UnlimitedUnsigned(std::move(result));
    }
    UnlimitedUnsigned operator-(const UnlimitedUnsigned& right) const
    {
        size_t resultSize = std::max(parts.size(), right.parts.size());
        std::vector<PartType> result(resultSize);
        PartType borrow = 0;
        size_t lastNonZero = 0;
        for (size_t i = 0; i < resultSize; i++) {
            auto newPart = (i < parts.size() ? parts[i] : 0)
                - (i < right.parts.size() ? right.parts[i] : 0) - borrow;
            if (newPart >= 0) {
                borrow = 0;
            }
            else {
                borrow = 1;
                newPart += maxPartPlusOne;
            }
            result[i] = newPart;
            if (newPart != 0) {
                lastNonZero = i + 1;
            }
        }
        if (borrow > 0) {
            throw std::overflow_error("UnlimitedUnsigned arithmetic produced a negative result");
        }
        result.resize(lastNonZero);
        return UnlimitedUnsigned(std::move(result));
    }
    UnlimitedUnsigned operator-(SmallDeltaType right) const
    {
        size_t resultSize = parts.size();
        std::vector<PartType> result(resultSize);
        PartType borrow = right;
        for (size_t i = 0; i < resultSize; i++) {
            auto newPart = parts[i] - borrow;
            if (newPart >= 0) {
                borrow = 0;
            }
            else {
                borrow = 1;
                newPart += maxPartPlusOne;
            }
            result[i] = newPart;
        }
        if (borrow > 0) {
            throw std::overflow_error("UnlimitedUnsigned arithmetic produced a negative result");
        }
        // Note that by here resultSize > 0, otherwise the original this == 0 and we would have
        // the above exception.
        if (result[resultSize - 1] == 0) {
            result.resize(resultSize - 1);
        }
        return UnlimitedUnsigned(std::move(result));
    }
    bool operator<=(const UnlimitedUnsigned& right) const
    {
        // lexicographical_compare is logically a < operation, so we reverse the operands and
        // invert the result to get <= and we compare the most significant chunks first.
        return parts.size() < right.parts.size()
            || (parts.size() == right.parts.size()
                && !std::lexicographical_compare(
                    right.parts.rbegin(),
                    right.parts.rend(),
                    parts.rbegin(),
                    parts.rend()
                ));
    }
    bool operator>(const UnlimitedUnsigned& right) const
    {
        return !(*this <= right);
    }
    bool operator==(const UnlimitedUnsigned& right) const
    {
        return parts == right.parts;
    }
    bool operator==(SmallDeltaType right) const
    {
        return right == 0 ? parts.empty() : (parts.size() == 1 && parts[0] == right);
    }

private:
    std::vector<PartType> parts;
};
}  // namespace Base
