/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <juergen.riegel@web.de>             *
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
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

#ifndef SPREADSHEET_ViewProviderImagePlane_H
#define SPREADSHEET_ViewProviderImagePlane_H

#include <QPointer>

#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/ViewProviderPythonFeature.h>
#include <Mod/Spreadsheet/SpreadsheetGlobal.h>


namespace Spreadsheet
{
class Sheet;
}

namespace SpreadsheetGui
{

class SheetView;

class SpreadsheetGuiExport ViewProviderSheet: public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(SpreadsheetGui::ViewProviderSheet);

public:
    /// constructor.
    ViewProviderSheet();

    /// destructor.
    ~ViewProviderSheet() override;

    void setDisplayMode(const char* ModeName) override;
    bool useNewSelectionModel() const override
    {
        return false;
    }
    std::vector<std::string> getDisplayModes() const override;

    bool doubleClicked() override;
    void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;

    Spreadsheet::Sheet* getSpreadsheetObject() const;

    void beforeDelete() override;

    QIcon getIcon() const override;

    bool setEdit(int ModNum) override;

    bool isShow() const override
    {
        return true;
    }

    Gui::MDIView* getMDIView() const override;

    inline SheetView* getView() const
    {
        return view;
    }

    PyObject* getPyObject() override;

protected:
    SheetView* showSpreadsheetView();
    void updateData(const App::Property* prop) override;

private:
    QPointer<SheetView> view;
};

using ViewProviderSheetPython = Gui::ViewProviderPythonFeatureT<ViewProviderSheet>;

}  // namespace SpreadsheetGui


#endif  // SPREADSHEET_ViewProviderSpreadsheet_H
