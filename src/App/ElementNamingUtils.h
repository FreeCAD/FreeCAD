// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <utility>
#include "FCGlobal.h"

namespace App
{

enum HistoryAlgorithm : int {
    V1 = 0,
    V2 = 1
};

AppExport HistoryAlgorithm getSelectedHistoryAlgorithm();
AppExport HistoryAlgorithm getDefaultHistoryAlgorithm();
AppExport HistoryAlgorithm getHistoryAlgorithm(int fromUnderlyingInteger);
AppExport int getSelectedUnderlyingHistoryAlgorithm();

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
/// Postfix to mark an element higher in the hierarchy.
constexpr const char* POSTFIX_UPPER                     = ";:U";
/// Postfix to mark an element lower in the hierarchy.
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
/// Empty sub-section value for V2.
constexpr const char* EMPTY_VALUE                       = "_";
/// Sub-section list deliminator.
constexpr const char* SUB_SECTION_LIST_DELIMINATOR      = ",";
/// Section sub deliminator.
constexpr const char* SECTION_SUB_DELIMINATOR           = ";";
/// Name section deliminator.
constexpr const char* NAME_SECTION_DELIMINATOR          = "|";
/// Escape character for mapped names in sub-sections.
constexpr const char* SUB_SECTION_ESCAPE_CHAR           = "^";

/// Mapper flag that designates that an element is mapped by its subelements.
constexpr const char* MAPPER_FLAG_LOWER                 = "LOW";
/// Mapper flag that designates that an element is mapped by other, higher-level, connected elements.
constexpr const char* MAPPER_FLAG_UPPER                 = "UPP";
/// Mapper flag that designates that an element is mapped with the `Generated` history method.
constexpr const char* MAPPER_FLAG_GENERATED             = "GEN";
/// Mapper flag that designates that an element is mapped with the `Modified` history method.
constexpr const char* MAPPER_FLAG_MODIFIED              = "MOD";
/// Mapper flag that designates that an element is mapped with `TopoDS_Shape`'s `IsPartner` method.
constexpr const char* MAPPER_FLAG_PARTNER               = "PTN";
/// Mapper flag that designates that an element is a source of `ReferenceIDs`.
constexpr const char* MAPPER_FLAG_SOURCE                = "SRC";
/// Mapper flag that designates that an element relies on unreliable, unstable, `IndexedName` mapping.
constexpr const char* MAPPER_FLAG_INDEX                 = "IDX";
/// Mapper flag that designates that an element's `LinkedNames` do not share common entries with other names that also have this flag.
constexpr const char* MAPPER_FLAG_NON_DUPLICATE         = "NDU";

// Placement indexes of data in sections of MappedNames used by the V2 Topological Naming System.
// DO NOT CHANGE THESE VALUES EVER!!! Data should only be added to sections, not removed or otherwise altered.

/// `Reference IDs` entry index.
constexpr const int   SECTION_REFERENCE_ID_INDEX        = 0;
/// `Reference Names` entry index.
constexpr const int   SECTION_LINKED_NAME_INDEX         = 1;
/// `Iteration Tag` entry index.
constexpr const int   SECTION_ITERATION_TAG_INDEX       = 2;
/// `OpCode` entry index.
constexpr const int   SECTION_OPCODE_INDEX              = 3;
/// `Index` entry index. (index index index)
constexpr const int   SECTION_INDEX_NUM_INDEX           = 4;
/// `Element Type` entry index.
constexpr const int   SECTION_ELEMENT_TYPE_INDEX        = 5;
/// `Duplicate count` entry index.
constexpr const int   SECTION_DUPLICATE_COUNT_INDEX     = 6;
/// `Mapper Flags` entry index.
constexpr const int   SECTION_MAPPER_FLAGS_INDEX        = 7;
/// `Connect Elements` entry index.
constexpr const int   SECTION_CONNECTED_ELEMENTS_INDEX  = 8;

/// Size of sections used in MappedNames by the V2 Topological Naming System
constexpr const int   SECTION_SIZE                      = 9;

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

/// Given a string and a prefix, return the integer index that follows that prefix, or 0 if not found
AppExport int indexOfElement(std::string_view s, std::string_view prefix);

}  // namespace Data
// clang-format on
