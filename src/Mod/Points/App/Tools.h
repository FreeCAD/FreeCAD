// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef POINTS_TOOLS_H
#define POINTS_TOOLS_H

#include <App/DocumentObject.h>
#include <algorithm>

namespace Points
{

template<typename PropertyT>
bool copyProperty(App::DocumentObject* target,
                  std::vector<App::DocumentObject*> source,
                  const char* propertyName)
{
    // check for properties
    if (std::all_of(std::begin(source), std::end(source), [=](auto obj) {
            return dynamic_cast<PropertyT*>(obj->getPropertyByName(propertyName)) != nullptr;
        })) {

        auto target_prop = dynamic_cast<PropertyT*>(
            target->addDynamicProperty(PropertyT::getClassTypeId().getName(), propertyName));
        if (target_prop) {
            auto values = target_prop->getValues();
            for (auto it : source) {
                auto source_prop = dynamic_cast<PropertyT*>(it->getPropertyByName(propertyName));
                if (source_prop) {
                    auto source_values = source_prop->getValues();
                    values.insert(values.end(), source_values.begin(), source_values.end());
                }
            }

            target_prop->setValues(values);
            return true;
        }
    }

    return false;
}

}  // namespace Points

#endif  // POINTS_TOOLS_H
