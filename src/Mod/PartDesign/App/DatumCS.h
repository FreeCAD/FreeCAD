/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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


#ifndef PARTDESIGN_DATUMCS_H
#define PARTDESIGN_DATUMCS_H

#include <Mod/Part/App/DatumFeature.h>

namespace PartDesign
{

class PartDesignExport CoordinateSystem : public Part::Datum
{
    PROPERTY_HEADER(PartDesign::CoordinateSystem);

public:
    CoordinateSystem();
    virtual ~CoordinateSystem();

    const char* getViewProviderName(void) const {
        return "PartDesignGui::ViewProviderDatumCoordinateSystem";
    }

    Base::Vector3d getXAxis();
    Base::Vector3d getYAxis();
    Base::Vector3d getZAxis();

    virtual App::DocumentObject *getSubObject(const char *subname, 
        PyObject **pyObj, Base::Matrix4D *pmat, bool transform, int depth) const override;
};

} //namespace PartDesign


#endif // PARTDESIGN_DATUMCS_H
