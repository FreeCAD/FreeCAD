/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef APP_PROPERTYVECTORLIST_H
#define APP_PROPERTYVECTORLIST_H

#include <Base/Vector3D.h>

#include "Property.h"

namespace Base
{
class Writer;
class XMLReader;
}

namespace App {

class AppExport PropertyVectorList: public PropertyListsT<Base::Vector3d>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

    using inherited = PropertyListsT<Base::Vector3d>;

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyVectorList();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyVectorList() override;

    void setValue(double x, double y, double z);
    using inherited::setValue;

    PyObject* getPyObject() override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    unsigned int getMemSize() const override;
    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyVectorListItem";
    }

protected:
    Base::Vector3d getPyValue(PyObject*) const override;
};

}  // namespace App

#endif  // APP_PROPERTYGEO_H
