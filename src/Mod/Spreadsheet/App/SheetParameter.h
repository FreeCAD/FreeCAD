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

#ifndef SHEETPARAMETER_H
#define SHEETPARAMETER_H


#include <Base/ParameterObserver.h>
#include <Mod/Spreadsheet/SpreadsheetGlobal.h>

namespace Spreadsheet
{

/** Convenient class to obtain sheet related parameters
 *
 * The parameters are under group "User parameter:BaseApp/Preferences/Mod/Spreadsheet"
 */
class SpreadsheetExport SheetParameter: public Base::ParameterObserver
{
public:
    SheetParameter();
    static SheetParameter* instance();

    std::string getImportExportDelimiter() const;
    void setImportExportDelimiter(std::string v);

    std::string getImportExportQuoteCharacter() const;
    void setImportExportQuoteCharacter(std::string v);

    std::string getImportExportEscapeCharacter() const;
    void setImportExportEscapeCharacter(std::string v);

    std::string getAliasedCellBackgroundColor() const;
    void setAliasedCellBackgroundColor(std::string v);

    std::string getTextColor() const;
    void setTextColor(std::string v);

    std::string getPositiveNumberColor() const;
    void setPositiveNumberColor(std::string v);

    std::string getNegativeNumberColor() const;
    void setNegativeNumberColor(std::string v);

    std::string getDisplayAliasFormatString() const;
    void setDisplayAliasFormatString(std::string v);

    bool getShowAliasName() const;
    void setShowAliasName(bool v);

    bool getSwitchToWorkbench() const;
    void setSwitchToWorkbench(bool v);

    long getDefaultZoomLevel() const;
    void setDefaultZoomLevel(long);

    long getMaximumRowCount() const;
    void setMaximumRowCount(long);

    long getMaximumColumnCount() const;
    void setMaximumColumnCount(long);

private:
    void setup();
};

}  // namespace Spreadsheet

#endif  // SHEETPARAMETER_H
