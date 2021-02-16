/***************************************************************************
 *   Copyright (c) 2005 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef APP_LEGACYPROPERTYSTATUS_H
#define APP_LEGACYPROPERTYSTATUS_H

#include "Property.h"
#include "StatusCollection.h"
#include <map>

namespace App
{

class LegacyPropertyStatus
{
    public:
    enum Value
    {
        Prop_ReadOnly    = 0, /*!< Property is read-only in the editor */
        Prop_Transient   = 1, /*!< Property content won't be saved to file, but still saves name, type and status */
        Prop_Hidden      = 2, /*!< Property won't appear in the editor */
        Prop_Output      = 3, /*!< Modified property doesn't touch its parent container */
        Prop_NoRecompute = 4, /*!< Modified property doesn't touch its container for recompute */
        Prop_NoPersist   = 5, /*!< Property won't be saved to file at all */
    };
};

inline StatusCollection<App::PropertyStatus> fromLegacyAttributes(const StatusCollection<App::LegacyPropertyStatus::Value> & attrs) {
    std::map <App::LegacyPropertyStatus::Value, App::PropertyStatus> conversion =  { 
        { App::LegacyPropertyStatus::Prop_ReadOnly, App::PropertyStatus::Prop_ReadOnly},
        { App::LegacyPropertyStatus::Prop_Transient, App::PropertyStatus::Prop_Transient},
        { App::LegacyPropertyStatus::Prop_Hidden, App::PropertyStatus::Prop_Hidden},
        { App::LegacyPropertyStatus::Prop_Output, App::PropertyStatus::Prop_Output},
        { App::LegacyPropertyStatus::Prop_NoRecompute, App::PropertyStatus::Prop_NoRecompute},
        { App::LegacyPropertyStatus::Prop_NoPersist, App::PropertyStatus::Prop_NoPersist},
    };

    StatusCollection<App::PropertyStatus> converted;

    for(const auto &pair: conversion) {
        if(attrs.test(pair.first)) {
            converted.set(pair.second);
        }
    }
    return converted;
}

} //namespace App

#endif // APP_LEGACYPROPERTYSTATUS_H
