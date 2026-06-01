// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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

#include <App/Property.h>

#include "Constraint3D.h"

namespace Sketcher3D
{

/// Property that stores a vector of Constraint3D values.
class Sketcher3DExport PropertyConstraint3DList: public App::PropertyLists
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyConstraint3DList();
    ~PropertyConstraint3DList() override;

    void setSize(int newSize) override;
    int getSize() const override;

    /// Reset the list, optionally setting single constraint.
    void setValue(const Constraint3D* lValue = nullptr);

    void setConstraints(const std::vector<Constraint3D>& v);
    const std::vector<Constraint3D>& getConstraints() const
    {
        return _constraints;
    }

    void setConstraintAt(int idx, const Constraint3D& c);
    void setConstraint(const Constraint3D& c)
    {
        _constraints.clear();
        _constraints.push_back(c);
    }

    // Persistence
    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    // Copy
    App::Property* Copy() const override;
    void Paste(const App::Property& from) override;
    unsigned int getMemSize() const override;


    // PyObject* getPyObject() override;
    // void setPyObject(PyObject* value) override;

private:
    std::vector<Constraint3D> _constraints;
};

}  // namespace Sketcher3D
