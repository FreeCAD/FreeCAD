/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef PART_PRISM_EXTENSION_H
#define PART_PRISM_EXTENSION_H

#include <App/DocumentObjectExtension.h>
#include <App/PropertyUnits.h>
#include <TopoDS_Face.hxx>

namespace Part
{

class PartExport PrismExtension : public App::DocumentObjectExtension
{
    EXTENSION_PROPERTY_HEADER(Part::PrismExtension);
public:
    PrismExtension();
    virtual ~PrismExtension();


    App::PropertyAngle FirstAngle;
    App::PropertyAngle SecondAngle;

    TopoDS_Shape makePrism(double height, const TopoDS_Face& face) const;

    virtual short int extensionMustExecute(void);
    virtual App::DocumentObjectExecReturn *extensionExecute(void);

protected:
    virtual void extensionOnChanged(const App::Property* /*prop*/);
};

} // namespace Part

#endif // PART_PRISM_EXTENSION_H
