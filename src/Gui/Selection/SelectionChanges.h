// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2011 Juergen Riegel <juergen.riegel@web.de>
// SPDX-FileCopyrightText: 2011 Werner Mayer <wmayer[at]users.sourceforge.net>
// SPDX-FileCopyrightText: 2026 FreeCAD Project Association
// SPDX-FileNotice: Part of the FreeCAD project.
/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <string>
#include <utility>

#include <App/DocumentObject.h>
#include <App/DocumentObserver.h>
#include <FCGlobal.h>

namespace Gui
{

/** Transport the changes of the Selection
 * This class transports closer information what was changed in the
 * selection. It's an optional information and not all commands set this
 * information. If not set all observer of the selection assume a full change
 * and update everything (e.g 3D view). This is not a very good idea if, e.g. only
 * a small parameter has changed. Therefore one can use this class and make the
 * update of the document much faster!
 * @see Base::Observer
 */
class GuiExport SelectionChanges
{
public:
    enum MsgType
    {
        AddSelection,
        RmvSelection,
        SetSelection,
        ClrSelection,
        SetPreselect,  // to signal observer the preselect has changed
        RmvPreselect,
        SetPreselectSignal,  // to request 3D view to change preselect
        PickedListChanged,
        ShowSelection,       // to show a selection
        HideSelection,       // to hide a selection
        RmvPreselectSignal,  // to request 3D view to remove preselect
        MovePreselect,       // to signal observer the mouse movement when preselect
    };
    enum class MsgSource
    {
        Any = 0,
        Internal = 1,
        TreeView = 2
    };

    SelectionChanges(
        MsgType type = ClrSelection,
        const char* docName = nullptr,
        const char* objName = nullptr,
        const char* subName = nullptr,
        const char* typeName = nullptr,
        float x = 0,
        float y = 0,
        float z = 0,
        MsgSource subtype = MsgSource::Any
    )
        : Type(type)
        , SubType(subtype)
        , x(x)
        , y(y)
        , z(z)
        , Object(docName, objName, subName)
    {
        pDocName = Object.getDocumentName().c_str();
        pObjectName = Object.getObjectName().c_str();
        pSubName = Object.getSubName().c_str();
        if (typeName) {
            TypeName = typeName;
        }
        pTypeName = TypeName.c_str();
    }  // explicit bombs

    SelectionChanges(
        MsgType type,
        const std::string& docName,
        const std::string& objName,
        const std::string& subName,
        const std::string& typeName = std::string(),
        float x = 0,
        float y = 0,
        float z = 0,
        MsgSource subtype = MsgSource::Any
    )
        : Type(type)
        , SubType(subtype)
        , x(x)
        , y(y)
        , z(z)
        , Object(docName.c_str(), objName.c_str(), subName.c_str())
        , TypeName(typeName)
    {
        pDocName = Object.getDocumentName().c_str();
        pObjectName = Object.getObjectName().c_str();
        pSubName = Object.getSubName().c_str();
        pTypeName = TypeName.c_str();
    }

    SelectionChanges(const SelectionChanges& other)
    {
        *this = other;
    }

    SelectionChanges& operator=(const SelectionChanges& other)
    {
        Type = other.Type;
        SubType = other.SubType;
        x = other.x;
        y = other.y;
        z = other.z;
        Object = other.Object;
        TypeName = other.TypeName;
        pDocName = Object.getDocumentName().c_str();
        pObjectName = Object.getObjectName().c_str();
        pSubName = Object.getSubName().c_str();
        pTypeName = TypeName.c_str();
        pOriginalMsg = other.pOriginalMsg;
        return *this;
    }

    SelectionChanges(SelectionChanges&& other)
    {
        *this = std::move(other);
    }

    SelectionChanges& operator=(SelectionChanges&& other)
    {
        Type = other.Type;
        SubType = other.SubType;
        x = other.x;
        y = other.y;
        z = other.z;
        Object = std::move(other.Object);
        TypeName = std::move(other.TypeName);
        pDocName = Object.getDocumentName().c_str();
        pObjectName = Object.getObjectName().c_str();
        pSubName = Object.getSubName().c_str();
        pTypeName = TypeName.c_str();
        pOriginalMsg = other.pOriginalMsg;
        return *this;
    }

    MsgType Type;
    MsgSource SubType;

    const char* pDocName;
    const char* pObjectName;
    const char* pSubName;
    const char* pTypeName;
    float x;
    float y;
    float z;

    App::SubObjectT Object;
    std::string TypeName;

    // Original selection message in case resolve!=0
    const SelectionChanges* pOriginalMsg = nullptr;
};

}  // namespace Gui
