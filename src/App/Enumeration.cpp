/***************************************************************************
 *   Copyright (c) 2015 Ian Rees <ian.rees@gmail.com>                      *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <cassert>
# include <cstring>
# include <cstdlib>
#endif

#include <Base/Exception.h>
#include "Enumeration.h"

using namespace App;

Enumeration::Enumeration()
    : _EnumArray(NULL), _ownEnumArray(false), _index(0), _maxVal(-1)
{
}

Enumeration::Enumeration(const Enumeration &other)
{
    if (other._ownEnumArray) {
        setEnums(other.getEnumVector());
    } else {
        _EnumArray = other._EnumArray;
    }

    _ownEnumArray = other._ownEnumArray;
    _index = other._index;
    _maxVal = other._maxVal;
}

Enumeration::Enumeration(const char *valStr)
    : _ownEnumArray(true), _index(0), _maxVal(0)
{
    _EnumArray = new const char*[2];
#if defined (_MSC_VER)
     _EnumArray[0] = _strdup(valStr);
#else
     _EnumArray[0] = strdup(valStr);
#endif
     _EnumArray[1] = NULL;
}

Enumeration::Enumeration(const char **list, const char *valStr)
    : _EnumArray(list), _ownEnumArray(false)
{
    findMaxVal();
    setValue(valStr);
}

Enumeration::~Enumeration()
{
    if (_ownEnumArray) {
        if (_EnumArray != NULL) {
            tearDown();
        }
    }
}

void Enumeration::tearDown(void)
{
    // Ugly...
    char **plEnums = (char **)_EnumArray;

    // Delete C Strings first
    while (*(plEnums++) != NULL) {
        free(*plEnums);
    }

    delete [] _EnumArray;

    _EnumArray = NULL;
    _ownEnumArray = false;
    _maxVal = -1;
}

void Enumeration::setEnums(const char **plEnums)
{
    std::string oldValue;
    bool preserve = (isValid() && plEnums != NULL);
    if (preserve) {
        const char* str = getCStr();
        if (str)
            oldValue = str;
    }

    // set _ownEnumArray
    if (isValid() && _ownEnumArray) {
        tearDown();
    }

    // set...
    _EnumArray = plEnums;

    // set _maxVal
    findMaxVal();

    // set _index
    _index = 0;
    if (preserve) {
        setValue(oldValue);
    }
}

void Enumeration::setEnums(const std::vector<std::string> &values)
{
    std::string oldValue;
    bool preserve = isValid();
    if (preserve) {
        const char* str = getCStr();
        if (str)
            oldValue = str;
    }

    if (isValid() && _ownEnumArray) {
        tearDown();
    }

    _EnumArray = new const char*[values.size() + 1];
    int i = 0;
    for (std::vector<std::string>::const_iterator it = values.begin(); it != values.end(); ++it) {
#if defined (_MSC_VER)
        _EnumArray[i++] = _strdup(it->c_str());
#else
        _EnumArray[i++] = strdup(it->c_str());
#endif
    }

    _EnumArray[i] = 0; // null termination

    // Other state variables
    _maxVal = values.size() - 1;
    _ownEnumArray = true;
    _index = 0;

    if (preserve) {
        setValue(oldValue);
    }
}

void Enumeration::setValue(const char *value)
{
    // using string methods without set, use setEnums(const char** plEnums) first!
    //assert(_EnumArray);

    if (!_EnumArray) {
        _index = 0;
        return;
    }

    int i = 0;
    const char **plEnums = _EnumArray;

    // search for the right entry
    while (1) {
        // end of list? set zero
        if (*plEnums == NULL) {
            _index = 0;
            break;
        }
        if (strcmp(*plEnums, value) == 0) {
            _index = i;
            break;
        }
        ++plEnums;
        ++i;
    }
}

void Enumeration::setValue(long value, bool checkRange)
{
    if (value >= 0 && value <= _maxVal) {
        _index = value;
    } else {
        if (checkRange) {
            throw Base::ValueError("Out of range");
        } else {
            _index = value;
        }
    }
}

bool Enumeration::isValue(const char *value) const
{
    // using string methods without set, use setEnums(const char** plEnums) first!
    //assert(_EnumArray);

    int i = getInt();

    if (i == -1) {
        return false;
    } else {
        return strcmp(_EnumArray[i], value) == 0;
    }
}

bool Enumeration::contains(const char *value) const
{
    // using string methods without set, use setEnums(const char** plEnums) first!
    //assert(_EnumArray);

    if (!getEnums()) {
        return false;
    }

    const char **plEnums = _EnumArray;

    // search for the right entry
    while (1) {
        // end of list?
        if (*plEnums == NULL)
            return false;
        if (strcmp(*plEnums, value) == 0)
            return true;
        ++plEnums;
    }
}

const char * Enumeration::getCStr(void) const
{
    // using string methods without set, use setEnums(const char** plEnums) first!
    //assert(_EnumArray);

    if (!isValid() || _index < 0 || _index > _maxVal) {
        return NULL;
    }

    return _EnumArray[_index];
}

int Enumeration::getInt(void) const
{
    if (!isValid() || _index < 0 || _index > _maxVal) {
        return -1;
    }

    return _index;
}

std::vector<std::string> Enumeration::getEnumVector(void) const
{
    // using string methods without set, use setEnums(const char** plEnums) first!
    if (!_EnumArray)
        return std::vector<std::string>();

    std::vector<std::string> result;
    const char **plEnums = _EnumArray;

    // end of list?
    while (*plEnums != NULL) {
        result.push_back(*plEnums);
        ++plEnums;
    }

    return result;
}

const char ** Enumeration::getEnums(void) const
{
    return _EnumArray;
}

bool Enumeration::isValid(void) const
{
    return (_EnumArray != NULL && _index >= 0 && _index <= _maxVal);
}

Enumeration & Enumeration::operator=(const Enumeration &other)
{
    if (other._ownEnumArray) {
        setEnums(other.getEnumVector());
    } else {
        _EnumArray = other._EnumArray;
    }

    _ownEnumArray = other._ownEnumArray;
    _index = other._index;
    _maxVal = other._maxVal;

    return *this;
}

bool Enumeration::operator==(const Enumeration &other) const
{
    if (getCStr() == NULL || other.getCStr() == NULL) {
        return false;
    }
    return (strcmp(getCStr(), other.getCStr()) == 0);
}

bool Enumeration::operator==(const char *other) const
{
    if (getCStr() == NULL) {
        return false;
    }

    return (strcmp(getCStr(), other) == 0);
}

void Enumeration::findMaxVal(void)
{
    if (_EnumArray == NULL) {
        _maxVal = -1;
        return;
    }

    const char **plEnums = _EnumArray;

    // the NULL terminator doesn't belong to the range of
    // valid values
    int i = -1;
    while (*(plEnums++) != NULL) {
        ++i;
        // very unlikely to have enums with more then 5000 entries!
        assert(i < 5000);
    }

    _maxVal = i;
}

