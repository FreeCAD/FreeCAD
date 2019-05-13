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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <sstream>
#endif

#include <Base/Writer.h>
#include <Base/Exception.h>

#include "Document.h"
#include "TransactionalObject.h"

using namespace App;


PROPERTY_SOURCE_ABSTRACT(App::TransactionalObject, App::ExtensionContainer)

TransactionalObject::TransactionalObject(void)
{
}

TransactionalObject::~TransactionalObject(void)
{
}

bool TransactionalObject::isAttachedToDocument() const
{
    return false;
}

const char* TransactionalObject::detachFromDocument()
{
    return "";
}

void TransactionalObject::onBeforeChangeProperty(Document *doc, const Property *prop)
{
    doc->onBeforeChangeProperty(this, prop);
}

