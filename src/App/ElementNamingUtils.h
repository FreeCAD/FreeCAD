#pragma once

#include <string>


namespace Data
{

/// Special prefix to mark the beginning of a mapped sub-element name
const std::string ELEMENT_MAP_PREFIX                = ";";

/// Special prefix to mark a missing element
const std::string MISSING_PREFIX                    = "?";

const std::string MAPPED_CHILD_ELEMENTS_PREFIX      = ELEMENT_MAP_PREFIX + ":R";

/// Special postfix to mark the following tag
const std::string POSTFIX_TAG                       = ELEMENT_MAP_PREFIX + ":H";
const std::string POSTFIX_DECIMAL_TAG               = ELEMENT_MAP_PREFIX + ":T";
const std::string POSTFIX_EXTERNAL_TAG              = ELEMENT_MAP_PREFIX + ":X";
const std::string POSTFIX_CHILD                     = ELEMENT_MAP_PREFIX + ":C";

/// Special postfix to mark the index of an array element
const std::string POSTFIX_INDEX                     = ELEMENT_MAP_PREFIX + ":I";
const std::string POSTFIX_UPPER                     = ELEMENT_MAP_PREFIX + ":U";
const std::string POSTFIX_LOWER                     = ELEMENT_MAP_PREFIX + ":L";
const std::string POSTFIX_MOD                       = ELEMENT_MAP_PREFIX + ":M";
const std::string POSTFIX_GEN                       = ELEMENT_MAP_PREFIX + ":G";
const std::string POSTFIX_MODGEN                    = ELEMENT_MAP_PREFIX + ":MG";
const std::string POSTFIX_DUPLICATE                 = ELEMENT_MAP_PREFIX + "D";


/// Check if a subname contains missing element
bool hasMissingElement(const char *subname);

/** Check if the name starts with elementMapPrefix()
 *
 * @param name: input name
 * @return Returns the name stripped with elementMapPrefix(), or 0 if not
 * start with the prefix
 */
const char *isMappedElement(const char *name);

/// Strip out the trailing element name if there is mapped element name preceeds it.
std::string newElementName(const char *name);

/// Strip out the mapped element name if there is one.
std::string oldElementName(const char *name);

/// Strip out the old and new element name if there is one.
std::string noElementName(const char *name);

/// Find the start of an element name in a subname
const char *findElementName(const char *subname);

const char *hasMappedElementName(const char *subname);


}// namespace Data