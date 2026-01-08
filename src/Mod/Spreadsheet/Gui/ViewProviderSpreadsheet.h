// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2011 Juergen Riegel <juergen.riegel@web.de>                            *
 *   © 2015 Eivind Kvedalen <eivind@kvedalen.name>                            *
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#ifndef SPREADSHEET_ViewProviderImagePlane_H
#define SPREADSHEET_ViewProviderImagePlane_H

#include <QPointer>

#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/ViewProviderFeaturePython.h>
#include <Mod/Spreadsheet/SpreadsheetGlobal.h>
#include <Mod/Spreadsheet/Gui/SpreadsheetView.h>


namespace SpreadsheetGui
{

class SpreadsheetGuiExport ViewProviderSheet: public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(SpreadsheetGui::ViewProviderSheet);

public:
    /// constructor.
    ViewProviderSheet();

    /// destructor.
    ~ViewProviderSheet() override;

    bool useNewSelectionModel() const override
    {
        return false;
    }

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

    void showSheetMdi();

    void exportAsFile();

protected:
    SheetView* showSpreadsheetView();
    void updateData(const App::Property* prop) override;

private:
    QPointer<SheetView> view;
};

using ViewProviderSheetPython = Gui::ViewProviderFeaturePythonT<ViewProviderSheet>;

}  // namespace SpreadsheetGui


#endif  // SPREADSHEET_ViewProviderSpreadsheet_H
