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

#ifndef PART_STEP_IMPORTEXPORTSETTINGS_H
#define PART_STEP_IMPORTEXPORTSETTINGS_H

#include <Mod/Part/App/Interface.h>
#include <Base/Parameter.h>


namespace Part
{
namespace STEP
{

class PartExport ImportExportSettings
{
public:
    ImportExportSettings();

    void setVisibleExportDialog(bool);
    bool isVisibleExportDialog() const;

    void setWriteSurfaceCurveMode(bool);
    bool getWriteSurfaceCurveMode() const;

    std::string getScheme() const;
    void setScheme(const char*);

    Interface::Unit getUnit() const;
    void setUnit(Interface::Unit);

    std::string getCompany() const;
    void setCompany(const char*);

    std::string getAuthor() const;
    void setAuthor(const char*);

    std::string getProductName() const;
    void setProductName(const char*);

private:
    ParameterGrp::handle pGroup;
};

} //namespace STEP
} //namespace Part

#endif
