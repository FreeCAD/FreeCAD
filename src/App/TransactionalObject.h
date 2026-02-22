// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2016 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#pragma once

#include <App/ExtensionContainer.h>

namespace App
{

class Document;
class TransactionObject;

/**
 * @brief Base class of transactional objects.
 *
 * A transactional object provides functionality that is inherited by its
 * children to ensure that these children objects can be targeted by a
 * transaction.
 */
class AppExport TransactionalObject: public App::ExtensionContainer
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::TransactionalObject);

public:
    TransactionalObject();
    ~TransactionalObject() override;

    /// Check if this object is attached to a document.
    virtual bool isAttachedToDocument() const;

    /**
     * @brief Deteach this object from the document.
     *
     * @return the name of the document this object was attached to.
     */
    virtual const char* detachFromDocument();

protected:
    /**
     * @brief Notify the document that a property is about to be changed.
     *
     * This function is called before the property is changed.
     *
     * @param[in,out] doc the document this object is attached to.
     * @param[in] prop the property that is about to be changed.
     */
    void onBeforeChangeProperty(Document* doc, const Property* prop);
};

}  // namespace App
