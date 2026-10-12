// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2025 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#include <App/Application.h>
#include "SheetParameter.h"

using namespace Spreadsheet;

void SheetParameter::setup()
{
    // NOLINTBEGIN
    addParameter("ImportExportDelimiter", String {"tab"});
    addParameter("ImportExportQuoteCharacter", String {"\""});
    addParameter("ImportExportEscapeCharacter", String {"\\"});
    addParameter("AliasedCellBackgroundColor", String {"#feff9e"});
    addParameter("TextColor", String {"#000000"});
    addParameter("PositiveNumberColor", String {"#000000"});
    addParameter("NegativeNumberColor", String {"#000000"});
    addParameter("DisplayAliasFormatString", String {"%V = %A"});
    addParameter("showAliasName", Bool {false});
    addParameter("SwitchToWB", Bool {true});
    addParameter("DefaultZoomLevel", Int {100});
    addParameter("MaximumRowCount", Int {1024});
    addParameter("MaximumColumnCount", Int {26});
    // NOLINTEND
}

SheetParameter::SheetParameter()
{
    attachToParameter(
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Spreadsheet")
    );
    setup();
    initParameters();
}

SheetParameter* SheetParameter::instance()
{
    static SheetParameter param;
    return &param;
}

FC_PARAM_GETSET_IMP(SheetParameter, ImportExportDelimiter, std::string)        // NOLINT
FC_PARAM_GETSET_IMP(SheetParameter, ImportExportQuoteCharacter, std::string)   // NOLINT
FC_PARAM_GETSET_IMP(SheetParameter, ImportExportEscapeCharacter, std::string)  // NOLINT
FC_PARAM_GETSET_IMP(SheetParameter, AliasedCellBackgroundColor, std::string)   // NOLINT
FC_PARAM_GETSET_IMP(SheetParameter, TextColor, std::string)                    // NOLINT
FC_PARAM_GETSET_IMP(SheetParameter, PositiveNumberColor, std::string)          // NOLINT
FC_PARAM_GETSET_IMP(SheetParameter, NegativeNumberColor, std::string)          // NOLINT
FC_PARAM_GETSET_IMP(SheetParameter, DisplayAliasFormatString, std::string)     // NOLINT
FC_PARAM_GETSET_IMP(SheetParameter, DefaultZoomLevel, long)
FC_PARAM_GETSET_IMP(SheetParameter, MaximumRowCount, long)
FC_PARAM_GETSET_IMP(SheetParameter, MaximumColumnCount, long)

bool SheetParameter::getShowAliasName() const
{
    return getValue<bool>("showAliasName");
}

void SheetParameter::setShowAliasName(bool v)
{
    setValue("showAliasName", v);
}

bool SheetParameter::getSwitchToWorkbench() const
{
    return getValue<bool>("SwitchToWB");
}

void SheetParameter::setSwitchToWorkbench(bool v)
{
    setValue("SwitchToWB", v);
}
