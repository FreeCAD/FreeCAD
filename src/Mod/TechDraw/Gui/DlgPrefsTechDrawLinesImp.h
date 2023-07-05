 /**************************************************************************
 *   Copyright (c) 2023 Benjamin Bræstrup Sayoc <benj5378@outlook.com>     *
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

#ifndef DRAWINGGUI_DLGPREFSTECHDRAWIMPLINES_H
#define DRAWINGGUI_DLGPREFSTECHDRAWIMPLINES_H

#include <memory>
#include <QTableWidgetItem>

#include <Gui/PropertyPage.h>
#include <Mod/TechDraw/TechDrawGlobal.h>


namespace TechDrawGui {
class Ui_DlgPrefsTechDrawLinesImp;

class DlgPrefsTechDrawLinesImp : public Gui::Dialog::PreferencePage
{
    Q_OBJECT

public:
    explicit DlgPrefsTechDrawLinesImp( QWidget* parent = nullptr );
    ~DlgPrefsTechDrawLinesImp() override;

    static QIcon iconOfLineStyle(QString dashArray);
    static QIcon iconOfLineStyle(QVector<double> dashArray) ;

public Q_SLOTS:
    void onLineGroupChanged(int);

protected:
    void saveSettings() override;
    void loadSettings() override;
    void changeEvent(QEvent *e) override;
    void cellEdited(QTableWidgetItem* item);

    void addRow() const;
    void populateCellsWithPreview();

private:
    std::unique_ptr<Ui_DlgPrefsTechDrawLinesImp> ui;
};

} // namespace TechDrawGui

#endif // DRAWINGGUI_DLGPREFSTECHDRAWIMPLINES_H
