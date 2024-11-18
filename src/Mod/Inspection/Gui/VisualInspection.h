/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
