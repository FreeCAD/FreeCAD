// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2025 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#include <string_view>

#include "ParameterObserver.h"

using namespace Base;

ParameterObserver::ParameterObserver() = default;

ParameterObserver::~ParameterObserver()
{
    if (handle) {
        handle->Detach(this);
    }
}

void ParameterObserver::attachToParameter(ParameterGrp::handle parameter)
{
    handle = parameter;
    handle->Attach(this);
}

void ParameterObserver::initParameters()
{
    for (const auto& it : parameters) {
        this->OnChange(*handle, it.first);
    }
}

void ParameterObserver::OnChange(Base::Subject<const char*>& subject, const char* sReason)
{
    std::ignore = subject;
    if (!sReason) {
        return;
    }
    auto it = parameters.find(sReason);
    if (it == parameters.end()) {
        return;
    }
    it->second.fetch(handle, sReason);
}

void ParameterObserver::addParameter(const char* key, const Object& value)
{
    parameters.emplace(key, value);
}

void ParameterObserver::setBoolean(const char* key, bool value)
{
    setValue(key, value);
}

bool ParameterObserver::getBoolean(const char* key) const
{
    return getValue<bool>(key);
}

bool ParameterObserver::getDefaultBoolean(const char* key) const
{
    return getDefault<bool>(key);
}

void ParameterObserver::setInt(const char* key, long value)
{
    setValue(key, value);
}

long ParameterObserver::getInt(const char* key) const
{
    return getValue<long>(key);
}

long ParameterObserver::getDefaultInt(const char* key) const
{
    return getDefault<long>(key);
}

void ParameterObserver::setUnsigned(const char* key, unsigned long value)
{
    setValue(key, value);
}

unsigned long ParameterObserver::getUnsigned(const char* key) const
{
    return getValue<unsigned long>(key);
}

unsigned long ParameterObserver::getDefaultUnsigned(const char* key) const
{
    return getDefault<unsigned long>(key);
}

void ParameterObserver::setFloat(const char* key, double value)
{
    setValue(key, value);
}

double ParameterObserver::getFloat(const char* key) const
{
    return getValue<double>(key);
}

double ParameterObserver::getDefaultFloat(const char* key) const
{
    return getDefault<double>(key);
}

void ParameterObserver::setString(const char* key, const char* value)
{
    setValue<std::string>(key, value);
}

std::string ParameterObserver::getString(const char* key) const
{
    return getValue<std::string>(key);
}

std::string ParameterObserver::getDefaultString(const char* key) const
{
    return getDefault<std::string>(key);
}
