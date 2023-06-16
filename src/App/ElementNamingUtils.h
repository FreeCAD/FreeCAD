#ifndef ELEMENT_NAMING_UTILS_H
#define ELEMENT_NAMING_UTILS_H

#include <string>
#include "FCGlobal.h"


namespace Data
{

/// Special prefix to mark the beginning of a mapped sub-element name
constexpr const char* ELEMENT_MAP_PREFIX                = ";";
constexpr size_t      ELEMENT_MAP_PREFIX_SIZE           = 1;

/// Special prefix to mark a missing element
constexpr const char* MISSING_PREFIX                    = "?";

// IMPORTANT: For all the constants below, the semicolon ";"
// at the start is ELEMENT_MAP_PREFIX

constexpr const char* MAPPED_CHILD_ELEMENTS_PREFIX      = ";:R";

/// Special postfix to mark the following tag
constexpr const char* POSTFIX_TAG                       = ";:H";
constexpr size_t      POSTFIX_TAG_SIZE                  = 3;

constexpr const char* POSTFIX_DECIMAL_TAG               = ";:T";
constexpr const char* POSTFIX_EXTERNAL_TAG              = ";:X";
constexpr const char* POSTFIX_CHILD                     = ";:C";

/// Special postfix to mark the index of an array element
constexpr const char* POSTFIX_INDEX                     = ";:I";
constexpr const char* POSTFIX_UPPER                     = ";:U";
constexpr const char* POSTFIX_LOWER                     = ";:L";
constexpr const char* POSTFIX_MOD                       = ";:M";
constexpr const char* POSTFIX_GEN                       = ";:G";
constexpr const char* POSTFIX_MODGEN                    = ";:MG";
constexpr const char* POSTFIX_DUPLICATE                 = ";D";


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


}// namespace Data

#endif // ELEMENT_NAMING_UTILS_H
