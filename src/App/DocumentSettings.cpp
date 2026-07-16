// SPDX-License-Identifier: LGPL-2.1-or-later

#include "DocumentSettings.h"

#include <algorithm>
#include <sstream>
#include <utility>

#include <Base/Exception.h>
#include <Base/StringUtils.h>

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

bool isValidNamespace(const std::string& value)
{
    if (value.empty()) {
        return false;
    }

    std::string::size_type segmentBegin = 0;
    while (segmentBegin < value.size()) {
        const auto segmentEnd = value.find('.', segmentBegin);
        const auto segmentLength = segmentEnd == std::string::npos
            ? std::string::npos
            : segmentEnd - segmentBegin;
        if (!isValidIdentifier(value.substr(segmentBegin, segmentLength))) {
            return false;
        }
        if (segmentEnd == std::string::npos) {
            return true;
        }
        segmentBegin = segmentEnd + 1;
    }

    return false;
}

void validateNamespace(const std::string& value)
{
    if (isValidNamespace(value)) {
        return;
    }

    throw Base::ValueError("Document settings namespace must be dot-separated ASCII identifiers");
}

}  // namespace

DocumentSettings::DocumentSettings(Document* document, std::string namespaceName)
    : documentPtr(std::make_unique<DocumentWeakPtrT>(document))
    , namespaceName(std::move(namespaceName))
{
    if (!document) {
        throw Base::ValueError("Document settings require a document");
    }
    validateNamespace(this->namespaceName);
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
    const auto& meta = document().Meta.getValues();
    const auto value = meta.find(metaKey);
    if (value == meta.end()) {
        return defaultValue;
    }
    return value->second;
}

void DocumentSettings::setString(const std::string& key, const std::string& value)
{
    document().Meta.setValue(makeMetaKey(key), value);
}

long DocumentSettings::getInt(const std::string& key, long defaultValue) const
{
    long value = 0;
    return Base::StringUtils::parseLong(getString(key, {}), value) ? value : defaultValue;
}

void DocumentSettings::setInt(const std::string& key, long value)
{
    setString(key, std::to_string(value));
}

double DocumentSettings::getFloat(const std::string& key, double defaultValue) const
{
    double value = 0.0;
    return Base::StringUtils::parseDouble(getString(key, {}), value) ? value : defaultValue;
}

void DocumentSettings::setFloat(const std::string& key, double value)
{
    setString(key, Base::StringUtils::formatDouble(value));
}

bool DocumentSettings::getBool(const std::string& key, bool defaultValue) const
{
    bool value = false;
    return Base::StringUtils::parseBool(getString(key, {}), value) ? value : defaultValue;
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
        if (!item.first.starts_with(prefix)) {
            continue;
        }

        auto key = item.first.substr(prefix.size());
        if (isValidIdentifier(key)) {
            result.push_back(std::move(key));
        }
    }

    std::sort(result.begin(), result.end());
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
