/***************************************************************************
 *   Copyright (c) Victor Titov (DeepSOIC)   <vv.titov@gmail.com> 2017     *
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

#ifndef CONTAINER_H
#define CONTAINER_H

#include "ContainerBase.h"

namespace App {

/**
 * @brief Container: unified interface for container objects.
 * Container objects are:
 * * Document
 * * anything with GroupExtension (and derived, incl. OriginGroupExtension)
 * * Origin
 */
class AppExport Container: public ContainerBase
{
    TYPESYSTEM_HEADER();
public:
    Container() {}
    Container(PropertyContainer* pcObject);
    virtual ~Container();

    virtual std::vector<DocumentObject*> dynamicChildren() const;
    virtual std::vector<DocumentObject*> staticChildren() const;

    virtual std::vector<PropertyContainer*> parents() const;
public:
    static bool isAContainer(PropertyContainer* object);
    static std::vector<DocumentObject*> findAllContainers(App::Document& doc);
};

}//namespace App
#endif // CONTAINERBASE_H
