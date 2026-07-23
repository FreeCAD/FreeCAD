// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <FCGlobal.h>

namespace Base::StringUtils
{

/**
 * @brief Return a copy of value without leading or trailing C-locale whitespace.
 */
BaseExport std::string trimmed(std::string_view value);

/**
 * @brief Return a copy of value with ASCII uppercase letters converted to lowercase.
 *
 * Non-ASCII bytes are preserved unchanged.
 */
BaseExport std::string lowercaseAscii(std::string_view value);

/**
 * @brief ASCII case-insensitive comparison of two strings for equality.
 *
 * Only characters in the ASCII range are case-folded; bytes are compared otherwise.
 */
BaseExport bool iequals(std::string_view lhs, std::string_view rhs);

/**
 * @brief ASCII case-insensitive test whether str begins with prefix.
 *
 * Only characters in the ASCII range are case-folded; bytes are compared otherwise.
 */
BaseExport bool istarts_with(std::string_view str, std::string_view prefix);

/**
 * @brief ASCII case-insensitive test whether str ends with suffix.
 *
 * Only characters in the ASCII range are case-folded; bytes are compared otherwise.
 */
BaseExport bool iends_with(std::string_view str, std::string_view suffix);

/**
 * @brief Split value on every occurrence of delimiter, keeping empty tokens.
 *
 * A string with no delimiter yields a single-element vector containing the whole
 * input; adjacent or trailing delimiters produce empty tokens, matching the
 * behaviour of boost::split with boost::is_any_of on a single character.
 */
BaseExport std::vector<std::string> split(std::string_view value, char delimiter);

/**
 * @brief Replace every occurrence of from with to in value, in place.
 *
 * Does nothing if from is empty. Equivalent to boost::replace_all.
 */
BaseExport void replaceAll(std::string& value, std::string_view from, std::string_view to);

/**
 * @brief Parse a whole string as a long integer using locale-independent C/POSIX rules.
 *
 * Leading and trailing whitespace is ignored. The parse fails if any non-whitespace text remains.
 */
BaseExport bool parseLong(std::string_view value, long& result);

/**
 * @brief Parse a whole string as a double using locale-independent C/POSIX rules.
 *
 * Uses "." as the decimal separator regardless of the user's system locale.
 * Leading and trailing whitespace is ignored. The parse fails if any non-whitespace text remains.
 */
BaseExport bool parseDouble(std::string_view value, double& result);

/**
 * @brief Parse a boolean value, ignoring leading/trailing whitespace and ASCII case.
 *
 * Accepted true values are "1", "true", "yes", and "on". Accepted false values are
 * "0", "false", "no", and "off".
 */
BaseExport bool parseBool(std::string_view value, bool& result);

/**
 * @brief Format a double using locale-independent C/POSIX rules.
 *
 * The returned string uses "." as the decimal separator, is the shortest representation that
 * can be passed to parseDouble() to recover the original finite value exactly.
 */
BaseExport std::string formatDouble(double value);

}  // namespace Base::StringUtils
