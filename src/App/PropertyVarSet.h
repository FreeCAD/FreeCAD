/****************************************************************************
 *   Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
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

#ifndef APP_PROPERTY_VARSET_H
#define APP_PROPERTY_VARSET_H

#include "PropertyLinks.h"

namespace App {

class VarSet;


/**
 * @brief A Property class that stores external links to VarSets
 */
class AppExport PropertyVarSet : public PropertyXLink
{

    TYPESYSTEM_HEADER_WITH_OVERRIDE();
    
public:

    /**
     * @brief Create a PropertyVarSet
     */
    PropertyVarSet();

    void setValue(DocumentObject* obj) override;

    /**
     * @brief Get the value of this Property
     *
     * @return The VarSet that this property links to or nullptr if there is no
     * value.
     */
    VarSet *getValue() const;
};

}

#endif
