// SPDX-License-Identifier: LGPL-2.1-or-later

#include "DocumentSettings.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <utility>

#include <Base/Exception.h>

#include "Document.h"
#include "DocumentObserver.h"

using namespace App;

namespace
{

bool isIdentifierChar(unsigned char ch)
{
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')
        || (ch >= '0' && ch <= '9') || ch == '_';
}

bool isValidIdentifier(const std::string& value)
{
    return !value.empty()
        && std::all_of(value.begin(), value.end(), [](unsigned char ch) {
               return isIdentifierChar(ch);
           });
}

void validateIdentifier(const std::string& value, const char* kind)
{
    if (isValidIdentifier(value)) {
        return;
    }

    std::ostringstream error;
    error << "Document settings " << kind
          << " must be non-empty and contain only ASCII letters, digits, and underscores";
    throw Base::ValueError(error.str());
}

std::string trim(const std::string& value)
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

std::string lowercaseAscii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool parseLong(const std::string& value, long& result)
{
    std::istringstream stream(trim(value));
    stream.imbue(std::locale::classic());
    stream >> result;
    return !stream.fail() && stream.eof();
}

bool parseDouble(const std::string& value, double& result)
{
    std::istringstream stream(trim(value));
    stream.imbue(std::locale::classic());
    stream >> result;
    return !stream.fail() && stream.eof();
}

bool parseBool(const std::string& value, bool& result)
{
    const auto lowered = lowercaseAscii(trim(value));
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

std::string formatDouble(double value)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::setprecision(std::numeric_limits<double>::digits10) << value;
    return stream.str();
}

bool startsWith(const std::string& value, const std::string& prefix)
{
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

}  // namespace

DocumentSettings::DocumentSettings(Document* document, std::string namespaceName)
    : documentPtr(std::make_unique<DocumentWeakPtrT>(document))
    , namespaceName(std::move(namespaceName))
{
    if (!document) {
        throw Base::ValueError("Document settings require a document");
    }
    validateIdentifier(this->namespaceName, "namespace");
}

DocumentSettings::~DocumentSettings() = default;

const std::string& DocumentSettings::getNamespace() const noexcept
{
    return namespaceName;
}

std::string DocumentSettings::getString(const std::string& key,
                                        const std::string& defaultValue) const
{
    const auto metaKey = makeMetaKey(key);
    const char* value = document().Meta.getValue(metaKey.c_str());
    if (!value) {
        return defaultValue;
    }
    return value;
}

void DocumentSettings::setString(const std::string& key, const std::string& value)
{
    document().Meta.setValue(makeMetaKey(key), value);
}

long DocumentSettings::getInt(const std::string& key, long defaultValue) const
{
    long value = 0;
    return parseLong(getString(key, {}), value) ? value : defaultValue;
}

void DocumentSettings::setInt(const std::string& key, long value)
{
    setString(key, std::to_string(value));
}

double DocumentSettings::getFloat(const std::string& key, double defaultValue) const
{
    double value = 0.0;
    return parseDouble(getString(key, {}), value) ? value : defaultValue;
}

void DocumentSettings::setFloat(const std::string& key, double value)
{
    setString(key, formatDouble(value));
}

bool DocumentSettings::getBool(const std::string& key, bool defaultValue) const
{
    bool value = false;
    return parseBool(getString(key, {}), value) ? value : defaultValue;
}

void DocumentSettings::setBool(const std::string& key, bool value)
{
    setString(key, value ? "true" : "false");
}

void DocumentSettings::remove(const std::string& key)
{
    const auto metaKey = makeMetaKey(key);
    document().Meta.setValue(metaKey.c_str(), nullptr);
}

std::vector<std::string> DocumentSettings::keys() const
{
    std::vector<std::string> result;
    const auto prefix = namespaceName + ".";

    for (const auto& item : document().Meta.getValues()) {
        if (!startsWith(item.first, prefix)) {
            continue;
        }

        auto key = item.first.substr(prefix.size());
        if (isValidIdentifier(key)) {
            result.push_back(std::move(key));
        }
    }

    return result;
}

Document& DocumentSettings::document() const
{
    if (documentPtr->expired()) {
        throw Base::ReferenceError("Document settings reference a closed document");
    }
    auto* doc = documentPtr->operator*();
    return *doc;
}

std::string DocumentSettings::makeMetaKey(const std::string& key) const
{
    validateIdentifier(key, "key");
    return namespaceName + "." + key;
}
