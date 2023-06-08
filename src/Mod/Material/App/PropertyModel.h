/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
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

#ifndef MATERIAL_PROPERTYMODEL_H
#define MATERIAL_PROPERTYMODEL_H

#include <App/Property.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "Model.h"

using namespace App;

namespace Materials {

class MaterialsExport PropertyModel : public Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyModel();
    ~PropertyModel() override;

    void setValue(const Model& value);
    const Model& getValue() const;

    PyObject *getPyObject() override;
    void setPyObject(PyObject *) override;

    const std::string getBase() const;
    const std::string& getName() const;
    Model::ModelType getType() const;
    const std::string getDirectory() const;
    const std::string& getUUID() const;

    void setName(const std::string& name);
    void setType(Model::ModelType type);
    void setDirectory(const std::string& directory);
    void setUUID(const std::string& uuid);

    void Save (Base::Writer &writer) const override;
    void Restore(Base::XMLReader &reader) override;

    const char* getEditorName() const override;

    Property *Copy() const override;
    void Paste(const Property &from) override;

    unsigned int getMemSize () const override{return sizeof(_model);}
    
    bool isSame(const Property &other) const override {
        if (&other == this)
            return true;
        return getTypeId() == other.getTypeId()
            && getValue() == static_cast<decltype(this)>(&other)->getValue();
    }

private:
    Model _model;
};

} // namespace Materials

#endif // MATERIAL_PROPERTYMODEL_H
