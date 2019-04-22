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


#ifndef APP_TRANSACTIONALOBJECT_H
#define APP_TRANSACTIONALOBJECT_H

#include <App/ExtensionContainer.h>

namespace App
{

class Document;
class TransactionObject;

/** Base class of transactional objects
 */
class AppExport TransactionalObject : public App::ExtensionContainer
{
    PROPERTY_HEADER(App::TransactionalObject);

public:
    /// Constructor
    TransactionalObject(void);
    virtual ~TransactionalObject();
    virtual bool isAttachedToDocument() const;
    virtual const char* detachFromDocument();

    virtual App::Property* addDynamicProperty(
          const char*, const char* = 0,
          const char* = 0, const char* = 0,
          short = 0, bool = false, bool = false);
    virtual bool removeDynamicProperty(const char*);

    /// get called when a property status has changed
    virtual void onPropertyStatusChanged(const Property &prop, unsigned long oldStatus) override;

protected:
    void onBeforeChangeProperty(Document *doc, const Property *prop);
};

} //namespace App


#endif // APP_TRANSACTIONALOBJECT_H
