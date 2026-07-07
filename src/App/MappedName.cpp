// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include <unordered_set>

#include "MappedName.h"


#include "Base/Console.h"

#include <boost/algorithm/string/predicate.hpp>
#include <iostream>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <unordered_set>


FC_LOG_LEVEL_INIT("MappedName", true, 2);  // NOLINT

namespace Data
{

void MappedName::compact() const
{
    ZoneScoped;

    auto self = const_cast<MappedName*>(
        this);  // FIXME this is a workaround for a single call in ElementMap::addName()

    if (this->raw) {
        self->data = QByteArray(self->data.constData(), self->data.size());
        self->raw = false;
    }
}

std::vector<std::string> MappedName::splitToSections(const std::string& data, const char* deliminator) {
    ZoneScoped;

    if (strlen(deliminator)) {
        return MappedName::splitToSections(data, *deliminator);
    }

    return { };
}

std::vector<std::string> MappedName::splitToSections(const std::string& data, const char deliminator) {
    ZoneScoped;

    std::stringstream ss;
    std::vector<std::string> sections { };
    int escapeLevel = 0;
    
    for (size_t i = 0; i < data.size(); i++) {
        char currentChar = data[i];

        if (currentChar == '^') {
            escapeLevel++;
        } else if ((currentChar == deliminator && escapeLevel == 0) || (i + 1) == data.size()) {
            if (currentChar != deliminator) {
                ss << currentChar;
            }

            sections.push_back(ss.str());
            ss.str("");
            
            continue;
        } else {
            escapeLevel = 0;
        }

        // if the escaped character is the same as the selected deliminator, then remove just one escape
        if (escapeLevel > 0 && ((i + 1) != data.size() && data[i + 1] == deliminator)) {
            continue;
        }

        ss << currentChar;
    }

    return sections;
}

DecodedMappedName MappedName::getDecodedMappedName(const std::string& mappedNameString) {
    ZoneScoped;

    auto it = decodedMappedNameCache.find(mappedNameString);

    if (it == decodedMappedNameCache.end()) {
        std::vector<std::string> stringVectorBuffer;
        DecodedMappedSection section;
        DecodedMappedName name;
        std::string stringBuffer;
        size_t mappedNameStringSize = mappedNameString.size();
        bool updateSection = false;
        bool postSection = false;
        int subSectionIndex = 0;
        int escapeLevel = 0;

        for (size_t i = 0; i < mappedNameStringSize; i++) {
            const char& currentChar = mappedNameString[i];

            if (escapeLevel == 0) {
                updateSection = false;
                postSection = false;
                
                if (currentChar == (*Data::SECTION_SUB_DELIMINATOR)) {
                    updateSection = true;
                } else if (currentChar == (*Data::NAME_SECTION_DELIMINATOR)) {
                    updateSection = true;
                    postSection = true;
                } else if (currentChar == (*Data::SUB_SECTION_LIST_DELIMINATOR)) {
                    stringVectorBuffer.push_back(stringBuffer);
                    stringBuffer.clear();
                    
                    continue;
                } else if ((i + 1) == mappedNameStringSize) {
                    stringBuffer += currentChar;

                    updateSection = true;
                    postSection = true;
                }

                if (updateSection) {
                    stringVectorBuffer.push_back(stringBuffer);

                    if (stringVectorBuffer.empty()) {
                        stringVectorBuffer.push_back(Data::EMPTY_VALUE);
                    }

                    switch (subSectionIndex) {
                        case Data::SECTION_REFERENCE_ID_INDEX:
                            section.referenceIDs = stringVectorBuffer;
                            break;
                        case Data::SECTION_LINKED_NAME_INDEX:
                            section.linkedNames = stringVectorBuffer;
                            break;
                        case Data::SECTION_ITERATION_TAG_INDEX:
                            section.iterationTag = stringVectorBuffer.front();
                            break;
                        case Data::SECTION_OPCODE_INDEX:
                            section.opCode = stringVectorBuffer.front();
                            break;
                        case Data::SECTION_INDEX_NUM_INDEX:
                            section.index = stringVectorBuffer.front();
                            break;
                        case Data::SECTION_ELEMENT_TYPE_INDEX:
                            section.elementType = stringVectorBuffer.front().front();
                            break;
                        case Data::SECTION_DUPLICATE_COUNT_INDEX:
                            section.duplicateCount = stringVectorBuffer.front();
                            break;
                        case Data::SECTION_MAPPER_FLAGS_INDEX:
                            section.mapperFlags = stringVectorBuffer;
                            break;
                        case Data::SECTION_CONNECTED_ELEMENTS_INDEX:
                            section.connectedElements = stringVectorBuffer;
                            break;
                    }
                    
                    stringVectorBuffer.clear();
                    stringBuffer.clear();

                    if (postSection) {
                        name.push_back(section);
                        subSectionIndex = 0;
                    } else {
                        subSectionIndex++;
                    }

                    continue;
                }
            }

            if (currentChar == (*Data::SUB_SECTION_ESCAPE_CHAR)) {
                if (++escapeLevel == 1) {
                    continue;
                }
            } else {
                escapeLevel = 0;
            }

            stringBuffer += currentChar;
        }

        decodedMappedNameCache[mappedNameString] = name;

        ZoneTextF("Generated name");
        return name;
    } else {
        ZoneTextF("Used cache");
        return it->second;
    }
}

DecodedMappedName MappedName::getDecodedMappedName() const {
    ZoneScoped;

    return MappedName::getDecodedMappedName(toString());
}

MappedName MappedName::fromDecodedMappedName(const DecodedMappedName& name) {
    ZoneScoped;

    std::stringstream ss;

    for (size_t i = 0; i < name.size(); ++i) {
        ss << MappedName::makeSection(name[i]);

        if ((i + 1) < name.size())
            ss << Data::NAME_SECTION_DELIMINATOR;
    }

    return MappedName(ss.str());
}

std::string MappedName::escapeString(const std::string& stringToEscape) {
    ZoneScoped;

    std::stringstream ss;
    std::unordered_set<char> charsToEscape {
        (*Data::NAME_SECTION_DELIMINATOR),
        (*Data::SECTION_SUB_DELIMINATOR),
        (*Data::SUB_SECTION_LIST_DELIMINATOR)
    };

    for (size_t i = 0; i < stringToEscape.size(); i++) {
        char currentChar = stringToEscape[i];

        if (charsToEscape.contains(currentChar)) {
            ss << Data::SUB_SECTION_ESCAPE_CHAR;
        }

        ss << currentChar;
    }

    return ss.str();
}


// IMPORTANT: make sure the placement of the sub-sections in the return
// string matches what is described in ElementNamingUtils.h
std::string MappedName::makeSection(const std::vector<std::string>& referenceIDs,
                                    const std::vector<MappedName>& linkedNames,
                                    const int&  iterationTag,
                                    const char* opCode,
                                    const int& index,
                                    const char& elementType,
                                    const int& duplicateCount,
                                    const std::vector<std::string>& mapperFlags,
                                    const std::vector<MappedName>& connectedElements) // TODO: switch to vector of individual flags
{
    ZoneScoped;

    return MappedName::makeSection(
        referenceIDs,
        linkedNames,
        std::to_string(iterationTag),
        opCode,
        std::to_string(index),
        elementType,
        std::to_string(duplicateCount),
        mapperFlags,
        connectedElements
    );
}

// IMPORTANT: make sure the placement of the sub-sections in the return
// string matches what is described in ElementNamingUtils.h
std::string MappedName::makeSection(const std::vector<std::string>& referenceIDs,
                                    const std::vector<MappedName>& linkedNames,
                                    const std::string& iterationTag,
                                    const char* opCode,
                                    const std::string& index,
                                    const char& elementType,
                                    const std::string& duplicateCount,
                                    const std::vector<std::string>& mapperFlags,
                                    const  std::vector<MappedName>& connectedElements) // TODO: switch to vector of individual flags
{
    ZoneScoped;

    std::vector<std::string> formattedLinkedNames { };
    std::vector<std::string> formattedConnectedElements { };

    for (const MappedName& name : linkedNames) {
        if (name) {
            formattedLinkedNames.push_back(name.toString());
        }
    }

    for (const MappedName& name : connectedElements) {
        if (name) {
            formattedConnectedElements.push_back(name.toString());
        }
    }

    return MappedName::makeSection(
        referenceIDs,
        formattedLinkedNames,
        iterationTag,
        opCode,
        index,
        elementType,
        duplicateCount,
        mapperFlags,
        formattedConnectedElements
    );
}

// IMPORTANT: make sure the placement of the sub-sections in the return
// string matches what is described in ElementNamingUtils.h
std::string MappedName::makeSection(const std::vector<std::string>& referenceIDs,
                                    const std::vector<std::string>& linkedNames,
                                    const std::string& iterationTag,
                                    const char* opCode,
                                    const std::string& index,
                                    const char& elementType,
                                    const std::string& duplicateCount,
                                    const std::vector<std::string>& mapperFlags,
                                    const std::vector<std::string>& connectedElements) // TODO: switch to vector of individual flags
{
    ZoneScoped;

    std::stringstream ss;
    std::string opCodeString = (opCode == nullptr || strlen(opCode) == 0) ? "MKR" : opCode;

    if (referenceIDs.empty()) {
        ss << Data::EMPTY_VALUE;
    } else {
        for (size_t i = 0; i < referenceIDs.size(); i++) {
            if (i != 0) {
                ss << Data::SUB_SECTION_LIST_DELIMINATOR;
            }

            ss << referenceIDs[i];
        }
    }

    ss << Data::SECTION_SUB_DELIMINATOR;

    if (linkedNames.empty()) {
        ss << Data::EMPTY_VALUE;
    } else {
        for (size_t i = 0; i < linkedNames.size(); i++) {
            if (i != 0) {
                ss << Data::SUB_SECTION_LIST_DELIMINATOR;
            }

            ss << MappedName::escapeString(linkedNames[i]);
        }
    }

    ss << Data::SECTION_SUB_DELIMINATOR 
       << iterationTag
       << Data::SECTION_SUB_DELIMINATOR
       << opCodeString
       << Data::SECTION_SUB_DELIMINATOR
       << index
       << Data::SECTION_SUB_DELIMINATOR
       << elementType
       << Data::SECTION_SUB_DELIMINATOR
       << duplicateCount
       << Data::SECTION_SUB_DELIMINATOR;

    if (mapperFlags.empty()) {
        ss << Data::EMPTY_VALUE;
    } else {
        for (size_t i = 0; i < mapperFlags.size(); i++) {
            if (i != 0) {
                ss << Data::SUB_SECTION_LIST_DELIMINATOR;
            }

            ss << mapperFlags[i];
        }
    }
    
    ss << Data::SECTION_SUB_DELIMINATOR;

    if (connectedElements.empty()) {
        ss << Data::EMPTY_VALUE;
    } else {
        for (size_t i = 0; i < connectedElements.size(); i++) {
            if (i != 0) {
                ss << Data::SUB_SECTION_LIST_DELIMINATOR;
            }

            ss << MappedName::escapeString(connectedElements[i]);
        }
    }
    
    return ss.str();
}

std::string MappedName::makeSection(const DecodedMappedSection& decodedSection)
{
    ZoneScoped;

    return MappedName::makeSection(
        decodedSection.referenceIDs,
        decodedSection.linkedNames,
        decodedSection.iterationTag,
        decodedSection.opCode.c_str(),
        decodedSection.index,
        decodedSection.elementType,
        decodedSection.duplicateCount,
        decodedSection.mapperFlags,
        decodedSection.connectedElements
    );
}

MappedName MappedName::makeUnmappedName(const std::vector<std::string>& indexedNames,
                                        const int& iterationTag,
                                        const char* opCode,
                                        const char& elementType) 
{
    ZoneScoped;

    return MappedName(
        MappedName::makeSection(
            indexedNames,
            {},
            iterationTag,
            opCode,
            0,
            elementType,
            0,
            {"IDX", "SRC"},
            {}
        )
    );
}

MappedName::MappedName(const char* name, int size) : raw(false)
{
    ZoneScoped;

    if (!name) {
        return;
    }
    if (boost::starts_with(name, ELEMENT_MAP_PREFIX)) {
        name += ELEMENT_MAP_PREFIX_SIZE;
    }

    data = size < 0 ? QByteArray(name) : QByteArray(name, size);
}

MappedName::MappedName(const std::string& nameString, const App::HistoryAlgorithm historyAlgorithm)
    : raw(false)
    , usedHistoryAlgorithm(historyAlgorithm)
{
    ZoneScoped;

    auto size = nameString.size();
    const char* name = nameString.c_str();
    if (boost::starts_with(nameString, ELEMENT_MAP_PREFIX)) {
        name += ELEMENT_MAP_PREFIX_SIZE;
        size -= ELEMENT_MAP_PREFIX_SIZE;
    }
    data = QByteArray(name, static_cast<int>(size));
}

int MappedName::findTagInElementName(long* tagOut,
                                     int* lenOut,
                                     std::string* postfixOut,
                                     char* typeOut,
                                     bool negative,
                                     bool recursive) const
{
    ZoneScoped;

    bool hex = true;
    int pos = this->rfind(POSTFIX_TAG);

    // Example name, POSTFIX_TAG == ;:H
    // #94;:G0;XTR;:H19:8,F;:H1a,F;BND:-1:0;:H1b:10,F
    //                                     ^
    //                                     |
    //                                    pos

    if (pos < 0) {
        pos = this->rfind(POSTFIX_DECIMAL_TAG);
        if (pos < 0) {
            return -1;
        }
        hex = false;
    }
    int offset = pos + (int)POSTFIX_TAG_SIZE;
    long _tag = 0;
    int _len = 0;
    char sep = 0;
    char sep2 = 0;
    char tp = 0;
    char eof = 0;

    int size {0};
    const char* nameAsChars = this->toConstString(offset, size);

    // check if the number followed by the tagPosfix is negative
    bool isNegative = (nameAsChars[0] == '-');
    if (isNegative) {
        ++nameAsChars;
        --size;
    }
    boost::iostreams::stream<boost::iostreams::array_source> iss(nameAsChars, size);
    if (!hex) {
        // no hex is an older version of the encoding scheme
        iss >> _tag >> sep;
    }
    else {
        // The purpose of tagOut postfixOut is to encode one model operation. The
        // 'tagOut' field is used to record the own object ID of that model shape,
        // and the 'lenOut' field indicates the length of the operation codes
        // before the tagOut postfixOut. These fields are in hex. The trailing 'F' is
        // the shape typeOut of this element, 'F' for face, 'E' edge, and 'V' vertex.
        //
        // #94;:G0;XTR;:H19:8,F;:H1a,F;BND:-1:0;:H1b:10,F
        //                     |              |   ^^ ^^
        //                     |              |   |   |
        //                     ---lenOut = 0x10---  tagOut lenOut

        iss >> std::hex;
        // _tag field can be skipped, if it is 0
        if (nameAsChars[0] == ',' || nameAsChars[0] == ':') {
            iss >> sep;
        }
        else {
            iss >> _tag >> sep;
        }
    }

    if (isNegative) {
        _tag = -_tag;
    }

    if (sep == ':') {
        // ':' is followed by _len field.
        //
        // For decTagPostfix() (i.e. older encoding scheme), this is the length
        // of the string before the entire postfixOut (A postfixOut may contain
        // multiple segments usually separated by ELEMENT_MAP_PREFIX.
        //
        // For newer POSTFIX_TAG, this counts the number of characters that
        // proceeds this tagOut postfixOut segment that forms the op code (see
        // example above).
        //
        // The reason of this change is so that the postfixOut can stay the same
        // regardless of the prefix, which can increase memory efficiency.
        //
        iss >> _len >> sep2 >> tp >> eof;

        // The next separator to look for is either ':' for older tagOut postfixOut, or ','
        if (!hex && sep2 == ':') {
            sep2 = ',';
        }
    }
    else if (hex && sep == ',') {
        // ',' is followed by a single character that indicates the element typeOut.
        iss >> tp >> eof;
        sep = ':';
        sep2 = ',';
    }

    if (_len < 0 || sep != ':' || sep2 != ',' || tp == 0 || eof != 0) {
        return -1;
    }

    if (hex) {
        if (pos - _len < 0) {
            return -1;
        }
        if ((_len != 0) && recursive && (tagOut || lenOut)) {
            // in case of recursive tagOut postfixOut (used by hierarchy element
            // map), look for any embedded tagOut postfixOut
            int next = MappedName::fromRawData(*this, pos - _len, _len).rfind(POSTFIX_TAG);
            if (next >= 0) {
                next += pos - _len;
                // #94;:G0;XTR;:H19:8,F;:H1a,F;BND:-1:0;:H1b:10,F
                //                     ^               ^
                //                     |               |
                //                    next            pos
                //
                // There maybe other operation codes after this embedded tagOut
                // postfixOut, search for the separator.
                //
                int end {0};
                if (pos == next) {
                    end = -1;
                }
                else {
                    end = MappedName::fromRawData(*this, next + 1, pos - next - 1)
                              .find(ELEMENT_MAP_PREFIX);
                }
                if (end >= 0) {
                    end += next + 1;
                    // #94;:G0;XTR;:H19:8,F;:H1a,F;BND:-1:0;:H1b:10,F
                    //                            ^
                    //                            |
                    //                           end
                    _len = pos - end;
                    // #94;:G0;XTR;:H19:8,F;:H1a,F;BND:-1:0;:H1b:10,F
                    //                            |       |
                    //                            -- lenOut --
                }
                else {
                    _len = 0;
                }
            }
        }

        // Now convert the 'lenOut' field back to the length of the remaining name
        //
        // #94;:G0;XTR;:H19:8,F;:H1a,F;BND:-1:0;:H1b:10,F
        // |                         |
        // ----------- lenOut -----------
        _len = pos - _len;
    }
    if (typeOut) {
        *typeOut = tp;
    }
    if (tagOut) {
        if (_tag == 0 && recursive) {
            return MappedName(*this, 0, _len)
                .findTagInElementName(tagOut, lenOut, postfixOut, typeOut, negative);
        }
        if (_tag > 0 || negative) {
            *tagOut = _tag;
        }
        else {
            *tagOut = -_tag;
        }
    }
    if (lenOut) {
        *lenOut = _len;
    }
    if (postfixOut) {
        *postfixOut = this->toString(pos);
    }
    return pos;
}

}  // namespace Data
