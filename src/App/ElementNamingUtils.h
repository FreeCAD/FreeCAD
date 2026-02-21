// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>
#include <utility>
#include "FCGlobal.h"

namespace App
{

/// Return type for lookups of new and old style sub-element names
struct ElementNamePair
{
    std::string newName;
    std::string oldName;

    ElementNamePair() = default;

    ElementNamePair(std::string newNameStr, std::string oldNameStr)
        : newName(std::move(newNameStr))
        , oldName(std::move(oldNameStr))
    {}

    bool operator==(const ElementNamePair& other) const
    {
        return this->newName == other.newName && this->oldName == other.oldName;
    };

    void swap(ElementNamePair& other) noexcept
    {
        std::swap(*this, other);
    }
};

}  // namespace App

// clang-format off
namespace Data
{

/**
 * @name Element name constants
 * @ingroup ElementMapping
 * @anchor ElementNameConstants
 *
 * @{
 */

/// Special prefix to mark the beginning of a mapped sub-element name
constexpr const char* ELEMENT_MAP_PREFIX                = ";";
/// The size of the element map prefix
constexpr size_t      ELEMENT_MAP_PREFIX_SIZE           = 1;

/// Special prefix to mark a missing element
constexpr const char* MISSING_PREFIX                    = "?";

// IMPORTANT: For all the constants below, the semicolon ";"
// at the start is ELEMENT_MAP_PREFIX

/// Prefix to mark child elements.
constexpr const char* MAPPED_CHILD_ELEMENTS_PREFIX      = ";:R";

/// Special postfix to mark the following tag
constexpr const char* POSTFIX_TAG                       = ";:H";
/// The size of the postfix tag
constexpr size_t      POSTFIX_TAG_SIZE                  = 3;

/// Postfix to mark a decimal tag.
constexpr const char* POSTFIX_DECIMAL_TAG               = ";:T";
/// Postfix to mark an external tag.
constexpr const char* POSTFIX_EXTERNAL_TAG              = ";:X";
/// Postfix to mark a child element.
constexpr const char* POSTFIX_CHILD                     = ";:C";

/// Special postfix to mark the index of an array element
constexpr const char* POSTFIX_INDEX                     = ";:I";
/// Postfix to mark an element higher in the hierarcy.
constexpr const char* POSTFIX_UPPER                     = ";:U";
/// Postfix to mark an element lower in the hierarcy.
constexpr const char* POSTFIX_LOWER                     = ";:L";
/// Postfix to mark an element as being modified.
constexpr const char* POSTFIX_MOD                       = ";:M";
/// Postfix to mark an element as being generated.
constexpr const char* POSTFIX_GEN                       = ";:G";
/// Postfix to mark an element as being modified and generated.
constexpr const char* POSTFIX_MODGEN                    = ";:MG";
/// Postfix to mark a duplicate element.
constexpr const char* POSTFIX_DUPLICATE                 = ";D";
/// Label to use for element index in element mapping.
constexpr const char* ELEMENT_MAP_INDEX                 = "_";

/// @}

/// Check if a subname contains missing element
AppExport bool hasMissingElement(const char *subname);

/** Check if the name starts with elementMapPrefix()
 *
 * @param name: input name
 * @return Returns the name stripped with elementMapPrefix(), or 0 if not
 * start with the prefix
 */
AppExport const char *isMappedElement(const char *name);

/// Strip out the trailing element name if there is mapped element name precedes it.
AppExport std::string newElementName(const char *name);

/// Strip out the mapped element name if there is one.
AppExport std::string oldElementName(const char *name);

/// Strip out the old and new element name if there is one.
AppExport std::string noElementName(const char *name);

/// Find the start of an element name in a subname
AppExport const char *findElementName(const char *subname);

AppExport const char *hasMappedElementName(const char *subname);

AppExport const std::string indexSuffix(int index, const char *label=ELEMENT_MAP_INDEX);

}  // namespace Data
// clang-format on
