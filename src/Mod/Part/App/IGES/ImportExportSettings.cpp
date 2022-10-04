/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <Interface_Static.hxx>
#endif

#include "ImportExportSettings.h"
#include "Interface.h"
#include <App/Application.h>


namespace Part {
namespace IGES {

ImportExportSettings::ImportExportSettings()
{
    pGroup = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Part/IGES");
}

Interface::Unit ImportExportSettings::getUnit() const
{
    return static_cast<Interface::Unit>(pGroup->GetInt("Unit", 0));
}

void ImportExportSettings::setUnit(Interface::Unit unit)
{
    pGroup->SetInt("Unit", static_cast<long>(unit));

    switch (unit) {
        case Interface::Unit::Meter:
            Interface_Static::SetCVal("write.step.unit","M");
            break;
        case Interface::Unit::Inch:
            Interface_Static::SetCVal("write.step.unit","INCH");
            break;
        default:
            Interface_Static::SetCVal("write.step.unit","MM");
            break;
    }
}

std::string ImportExportSettings::getCompany() const
{
    return pGroup->GetASCII("Company");
}

void ImportExportSettings::setCompany(const char* name)
{
    pGroup->SetASCII("Company", name);
}

std::string ImportExportSettings::getAuthor() const
{
    return pGroup->GetASCII("Author");
}

void ImportExportSettings::setAuthor(const char* name)
{
    pGroup->SetASCII("Author", name);
}

std::string ImportExportSettings::getProductName() const
{
    return Part::Interface::writeIgesHeaderProduct();
}

void ImportExportSettings::setProductName(const char* name)
{
    Part::Interface::writeIgesHeaderProduct(name);
}

} // namespace IGES
} // namespace Part
