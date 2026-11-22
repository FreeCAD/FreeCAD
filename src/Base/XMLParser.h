#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <map>

#include <FCGlobal.h>

namespace fs = std::filesystem;

namespace Base
{

/**
 * Internal structure storing a XML hierarchy,
 * hiding the actual XML parsing library to internal consumers.
 *
 * This structure is recursive, children elements are owned by their parent.
 * Implicit copies are disabled, use the `clone` function for a deep copy of the structure.
 */
struct BaseExport XMLElement
{
    XMLElement() = default;
    ~XMLElement() = default;

    std::string tag;
    std::map<std::string, std::string> attrs;
    std::vector<std::unique_ptr<XMLElement>> children;
    std::string content;

    // Disable costly recursive copies
    FC_DISABLE_COPY(XMLElement)

    // Accept move semantics
    FC_DEFAULT_MOVE(XMLElement)

    // Explicit deep copy
    std::unique_ptr<XMLElement> clone() const;
};

/**
 * Parse an XML document stored on disk.
 * Throws `XMLBaseException` on error.
 */
BaseExport std::unique_ptr<XMLElement> ParseXMLFile(const fs::path& path);

/**
 * Save an XML document to disk.
 * Throws `XMLBaseException` on error.
 */
BaseExport void SaveXMLFile(const fs::path& path, const XMLElement& xmlTree);

/**
 * Verify the conformance of an XML document to an XSD schema.
 * Return std::nullopt on success, or a list of errors on failure.
 */
BaseExport std::optional<std::vector<std::string>> CheckXMLDocument(
    const XMLElement& xmlTree,
    const std::string& xsdString
);

}  // namespace Base
