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
#endif

#include <Base/Exception.h>
#include "Enumeration.h"
#include <string_view>

using namespace App;

namespace {
struct StringCopy : public Enumeration::Object {
    explicit StringCopy(const char* str) : d(str) {
    }
    const char* data() const override {
        return d.data();
    }
    bool isEqual(const char* str) const override {
        return d == str;
    }
    bool isCustom() const override {
        return true;
    }

private:
    std::string d;
};

struct StringView : public Enumeration::Object {
    explicit StringView(const char* str) : d(str) {
    }
    const char* data() const override {
        return d.data();
    }
    bool isEqual(const char* str) const override {
        return d == str;
    }
    bool isCustom() const override {
        return false;
    }

private:
    std::string_view d;
};
}

Enumeration::Enumeration()
    : _index(0)
{
}

Enumeration::Enumeration(const Enumeration &other)
{
    enumArray = other.enumArray;
    _index = other._index;
}

Enumeration::Enumeration(const char *valStr)
    : _index(0)
{
    enumArray.push_back(std::make_shared<StringCopy>(valStr));
    setValue(valStr);
}

Enumeration::Enumeration(const char **list, const char *valStr)
    : _index(0)
{
    while (list && *list) {
        enumArray.push_back(std::make_shared<StringView>(*list));
        list++;
    }
    setValue(valStr);
}

Enumeration::~Enumeration()
{
    enumArray.clear();
}

void Enumeration::setEnums(const char **plEnums)
{
    std::string oldValue;
    bool preserve = (isValid() && plEnums != nullptr);
    if (preserve) {
        const char* str = getCStr();
        if (str)
            oldValue = str;
    }

    enumArray.clear();
    while (plEnums && *plEnums) {
        enumArray.push_back(std::make_shared<StringView>(*plEnums));
        plEnums++;
    }

    // set _index
    if (_index < 0)
        _index = 0;
    if (preserve) {
        setValue(oldValue);
    }
}

void Enumeration::setEnums(const std::vector<std::string> &values)
{
    if (values.empty()) {
        setEnums(nullptr);
        return;
    }

    std::string oldValue;
    bool preserve = isValid();
    if (preserve) {
        const char* str = getCStr();
        if (str)
            oldValue = str;
    }

    enumArray.clear();
    for (const auto & it : values) {
        enumArray.push_back(std::make_shared<StringCopy>(it.c_str()));
    }

    // set _index
    if (_index < 0)
        _index = 0;
    if (preserve) {
        setValue(oldValue);
    }
}

void Enumeration::setValue(const char *value)
{
    _index = 0;
    for (std::size_t i = 0; i < enumArray.size(); i++) {
        if (enumArray[i]->isEqual(value)) {
            _index = static_cast<int>(i);
            break;
        }
    }
}

void Enumeration::setValue(long value, bool checkRange)
{
    if (value >= 0 && value < countItems()) {
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
    int i = getInt();

    if (i == -1) {
        return false;
    } else {
        return enumArray[i]->isEqual(value);
    }
}

bool Enumeration::contains(const char *value) const
{
    if (!isValid()) {
        return false;
    }

    for (const auto& it : enumArray) {
        if (it->isEqual(value))
            return true;
    }

    return false;
}

const char * Enumeration::getCStr() const
{
    if (!isValid() || _index < 0 || _index >= countItems()) {
        return nullptr;
    }

    return enumArray[_index]->data();
}

int Enumeration::getInt() const
{
    if (!isValid() || _index < 0 || _index >= countItems()) {
        return -1;
    }

    return _index;
}

std::vector<std::string> Enumeration::getEnumVector() const
{
    std::vector<std::string> list;
    for (const auto& it : enumArray)
        list.emplace_back(it->data());
    return list;
}

bool Enumeration::hasEnums() const
{
    return (!enumArray.empty());
}

bool Enumeration::isValid() const
{
    return (!enumArray.empty() && _index >= 0 && _index < countItems());
}

int Enumeration::maxValue() const
{
    int num = -1;
    if (!enumArray.empty())
        num = static_cast<int>(enumArray.size()) - 1;
    return num;
}

bool Enumeration::isCustom() const
{
    for (const auto& it : enumArray) {
        if (it->isCustom())
            return true;
    }
    return false;
}

Enumeration & Enumeration::operator=(const Enumeration &other)
{
    if (this == &other)
        return *this;

    enumArray = other.enumArray;
    _index = other._index;

    return *this;
}

bool Enumeration::operator==(const Enumeration &other) const
{
    if (_index != other._index || enumArray.size() != other.enumArray.size()) {
        return false;
    }
    for (size_t i = 0; i < enumArray.size(); ++i) {
        if (enumArray[i]->data() == other.enumArray[i]->data())
            continue;
        if (!enumArray[i]->data() || !other.enumArray[i]->data())
            return false;
        if (!enumArray[i]->isEqual(other.enumArray[i]->data()))
            return false;
    }
    return true;
}

bool Enumeration::operator==(const char *other) const
{
    if (!getCStr()) {
        return false;
    }

    return (strcmp(getCStr(), other) == 0);
}

int Enumeration::countItems() const
{
    return static_cast<int>(enumArray.size());
}
