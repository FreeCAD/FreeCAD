// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <FCGlobal.h>

namespace App
{

class Document;
class DocumentWeakPtrT;

/**
 * @brief Namespaced document settings backed by a document's Meta property.
 *
 * DocumentSettings provides a typed convenience API over App::Document::Meta while
 * keeping persisted values string-backed. Keys are stored in Meta as
 * "<namespace>.<key>", where the namespace may contain dot-separated
 * identifier segments and the key is a single identifier. Updates are applied
 * to individual map entries so unrelated metadata is preserved.
 */
class AppExport DocumentSettings
{
public:
    /**
     * @brief Create a settings proxy for one document namespace.
     *
     * @param document Document whose Meta property stores the settings.
     * @param namespaceName Namespace prefix used for Meta keys.
     *
     * @throws Base::ValueError if the document is null or the namespace is invalid.
     */
    DocumentSettings(Document* document, std::string namespaceName);
    ~DocumentSettings();

    /**
     * @brief Return the namespace prefix used by this settings proxy.
     */
    const std::string& getNamespace() const noexcept;

    /**
     * @brief Return a string setting, or defaultValue when the key is missing.
     */
    std::string getString(const std::string& key, const std::string& defaultValue) const;

    /**
     * @brief Store a string setting.
     */
    void setString(const std::string& key, const std::string& value);

    /**
     * @brief Return an integer setting, or defaultValue when missing or invalid.
     */
    long getInt(const std::string& key, long defaultValue) const;

    /**
     * @brief Store an integer setting as a canonical string.
     */
    void setInt(const std::string& key, long value);

    /**
     * @brief Return a floating-point setting, or defaultValue when missing or invalid.
     */
    double getFloat(const std::string& key, double defaultValue) const;

    /**
     * @brief Store a floating-point setting as a locale-independent string.
     */
    void setFloat(const std::string& key, double value);

    /**
     * @brief Return a boolean setting, or defaultValue when missing or invalid.
     */
    bool getBool(const std::string& key, bool defaultValue) const;

    /**
     * @brief Store a boolean setting as "true" or "false".
     */
    void setBool(const std::string& key, bool value);

    /**
     * @brief Remove one key from this namespace.
     */
    void remove(const std::string& key);

    /**
     * @brief Return valid setting keys present in this namespace, sorted lexicographically.
     */
    std::vector<std::string> keys() const;

private:
    Document& document() const;
    std::string makeMetaKey(const std::string& key) const;

    std::unique_ptr<DocumentWeakPtrT> documentPtr;
    std::string namespaceName;
};

}  // namespace App
