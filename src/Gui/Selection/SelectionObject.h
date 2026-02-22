// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
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

#include <Base/BaseClass.h>
#include <Base/Vector3D.h>
#include <FCGlobal.h>
#include <string>

namespace App
{
class DocumentObject;
}

namespace Gui
{

class SelectionChanges;

/**
 * The Selection object class
 */
class GuiExport SelectionObject: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /** Constructs a SelectionObject object. */
    SelectionObject();
    /*! Constructs a SelectionObject from the SelectionChanges structure.
     */
    explicit SelectionObject(const SelectionChanges& msg);
    explicit SelectionObject(const App::DocumentObject*);
    ~SelectionObject() override;
    /**
     * The default implementation returns an instance of @ref SelectionObjectPy.
     */
    PyObject* getPyObject() override;

    /// get the SubElement name of this SelectionObject
    inline const std::vector<std::string>& getSubNames() const
    {
        return SubNames;
    }
    /// are there any SubNames selected
    bool hasSubNames() const
    {
        return !SubNames.empty();
    }
    /// get the name of the Document of this SelctionObject
    inline const char* getDocName() const
    {
        return DocName.c_str();
    }
    /// get the name of the Document Object of this SelectionObject
    inline const char* getFeatName() const
    {
        return FeatName.c_str();
    }
    /// get the Type of the selected Object
    inline const char* getTypeName() const
    {
        return TypeName.c_str();
    }
    /// get the selection points
    inline const std::vector<Base::Vector3d> getPickedPoints() const
    {
        return SelPoses;
    }

    /// returns the selected DocumentObject or NULL if the object is already deleted
    const App::DocumentObject* getObject() const;
    /// returns the selected DocumentObject or NULL if the object is already deleted
    template<class T>
    const T* getObject() const
    {
        return freecad_cast<T*>(getObject());
    };
    /// returns the selected DocumentObject or NULL if the object is already deleted
    App::DocumentObject* getObject();
    /// returns the selected DocumentObject if it is of T type or null otherwise
    template<class T>
    T* getObject()
    {
        return freecad_cast<T*>(getObject());
    }

    /// check the selected object is a special type or derived of
    bool isObjectTypeOf(const Base::Type& typeId) const;

    /// returns python expreasion sutably for assigning to a LinkSub property
    std::string getAsPropertyLinkSubString() const;

    friend class SelectionSingleton;

protected:
    std::vector<std::string> SubNames;
    std::string DocName;
    std::string FeatName;
    std::string TypeName;
    std::vector<Base::Vector3d> SelPoses;

private:
    /// to make sure no duplicates of subnames
    std::set<std::string> _SubNameSet;
};


}  // namespace Gui
