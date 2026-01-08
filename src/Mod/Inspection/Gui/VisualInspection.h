// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2011 Werner Mayer <wmayer@users.sourceforge.net>                       *
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


#ifndef INSPECTIONGUI_VISUALINSPECTION_H
#define INSPECTIONGUI_VISUALINSPECTION_H

#include <QDialog>


class QTreeWidgetItem;
class QPushButton;

namespace InspectionGui
{
class Ui_VisualInspection;
class VisualInspection: public QDialog
{
    Q_OBJECT

public:
    explicit VisualInspection(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~VisualInspection() override;

    void accept() override;

protected Q_SLOTS:
    void onActivateItem(QTreeWidgetItem*);
    void loadSettings();
    void saveSettings();

private:
    Ui_VisualInspection* ui;
    QPushButton* buttonOk;
};

}  // namespace InspectionGui

#endif  // INSPECTIONGUI_VISUALINSPECTION_H
