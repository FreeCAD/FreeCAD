// SPDX-License-Identifier: LGPL-2.1-or-later

#include "StringUtils.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>

std::string Base::StringUtils::trimmed(std::string_view value)
{
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
                         return std::isspace(ch) != 0;
                     }).base();

    if (begin >= end) {
        return {};
    }
    return {begin, end};
}

std::string Base::StringUtils::lowercaseAscii(std::string_view value)
{
    std::string result(value);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
        if (ch >= 'A' && ch <= 'Z') {
            return static_cast<char>(ch - 'A' + 'a');
        }
        return static_cast<char>(ch);
    });
    return result;
}

bool Base::StringUtils::iequals(std::string_view lhs, std::string_view rhs)
{
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), [](char l, char r) {
        return std::tolower(static_cast<unsigned char>(l))
            == std::tolower(static_cast<unsigned char>(r));
    });
}

bool Base::StringUtils::istarts_with(std::string_view str, std::string_view prefix)
{
    return str.size() >= prefix.size() && iequals(str.substr(0, prefix.size()), prefix);
}

bool Base::StringUtils::parseLong(std::string_view value, long& result)
{
    std::istringstream stream(trimmed(value));
    stream.imbue(std::locale::classic());
    stream >> result;
    return !stream.fail() && stream.eof();
}

bool Base::StringUtils::parseDouble(std::string_view value, double& result)
{
    std::istringstream stream(trimmed(value));
    stream.imbue(std::locale::classic());
    stream >> result;
    return !stream.fail() && stream.eof();
}

bool Base::StringUtils::parseBool(std::string_view value, bool& result)
{
    const auto lowered = lowercaseAscii(trimmed(value));
    if (lowered == "1" || lowered == "true" || lowered == "yes" || lowered == "on") {
        result = true;
        return true;
    }
    if (lowered == "0" || lowered == "false" || lowered == "no" || lowered == "off") {
        result = false;
        return true;
    }
    return false;
}

std::string Base::StringUtils::formatDouble(double value)
{
    std::array<char, 64> buffer;
    const auto [end, error] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    if (error == std::errc {}) {
        return {buffer.data(), end};
    }

    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
    return stream.str();
}
