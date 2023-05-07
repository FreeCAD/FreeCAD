/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#ifndef TECHDRAW_COSMETICEXTENSION_H
#define TECHDRAW_COSMETICEXTENSION_H

#include <App/DocumentObjectExtension.h>
#include <App/ExtensionPython.h>
#include <Base/Vector3D.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include "CenterLine.h"
#include "Geometry.h"
#include "PropertyCosmeticList.h"
#include "PropertyCosmeticVertexList.h"


namespace TechDraw {
class DrawViewPart;
class GeometryObject;

class TechDrawExport CosmeticExtension : public App::DocumentObjectExtension {
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::CosmeticObject);

public:
    CosmeticExtension();
    ~CosmeticExtension() override;

    TechDraw::PropertyCosmeticVertexList CosmeticVertexes;
    TechDraw::PropertyCosmeticList       Cosmetics; // to be renamed!!

    virtual std::string     addCosmeticVertex(Base::Vector3d pos);
    virtual CosmeticVertex* getCosmeticVertexBySelection(std::string name) const;
    virtual CosmeticVertex* getCosmeticVertexBySelection(int i) const;
    virtual CosmeticVertex* getCosmeticVertex(std::string id) const;
    virtual void            removeCosmeticVertex(std::string tag);
    virtual void            removeCosmeticVertex(std::vector<std::string> delTags);

    virtual std::string     addCosmeticEdge(Base::Vector3d start, Base::Vector3d end);

    virtual std::string     addCenterLine(Base::Vector3d start, Base::Vector3d end);

    virtual GeomFormat*     getGeomFormatBySelection(std::string name) const;
    virtual GeomFormat*     getGeomFormatBySelection(int i) const;

    template<typename T>
    std::string             addCosmetic(BaseGeomPtr bg);
    template<typename T>
    std::string             addCosmetic(T* cosmetic);
    void                    removeCosmetic(std::string tag);
    void                    removeCosmetic(std::vector<std::string> tag);
    template<typename T>
    T                       getCosmeticByName(std::string name) const;
    template<typename T>
    T                       getCosmeticByName(int i) const;
    PyObject* getExtensionPyObject() override;

protected:
/*    virtual void extHandleChangedPropertyName(Base::XMLReader &reader, */
/*                                              const char* TypeName, */
/*                                              const char* PropName);*/

private:

};

using CosmeticExtensionPython = App::ExtensionPythonT<CosmeticExtension>;

} //end namespace TechDraw

#endif //TECHDRAW_COSMETICEXTENSION_H
