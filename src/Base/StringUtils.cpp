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

bool Base::StringUtils::iends_with(std::string_view str, std::string_view suffix)
{
    return str.size() >= suffix.size() && iequals(str.substr(str.size() - suffix.size()), suffix);
}

std::vector<std::string> Base::StringUtils::split(std::string_view value, char delimiter)
{
    std::vector<std::string> tokens;
    std::string_view::size_type start = 0;
    while (true) {
        const auto pos = value.find(delimiter, start);
        tokens.emplace_back(value.substr(start, pos - start));
        if (pos == std::string_view::npos) {
            break;
        }
        start = pos + 1;
    }
    return tokens;
}

void Base::StringUtils::replaceAll(std::string& value, std::string_view from, std::string_view to)
{
    if (from.empty()) {
        return;
    }
    std::string::size_type pos = 0;
    while ((pos = value.find(from, pos)) != std::string::npos) {
        value.replace(pos, from.size(), to);
        pos += to.size();
    }
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
