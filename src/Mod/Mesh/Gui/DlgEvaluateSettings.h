/***************************************************************************
 *   Copyright (c) 2016 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef MESHGUI_DLG_EVALUATE_SETTINGS_H
#define MESHGUI_DLG_EVALUATE_SETTINGS_H

#include <QDialog>

namespace MeshGui
{

class Ui_DlgEvaluateSettings;

/**
 * \author Werner Mayer
 */
class DlgEvaluateSettings: public QDialog
{
    Q_OBJECT

public:
    explicit DlgEvaluateSettings(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgEvaluateSettings() override;

    void setNonmanifoldPointsChecked(bool);
    bool isNonmanifoldPointsChecked() const;

    void setFoldsChecked(bool);
    bool isFoldsChecked() const;

    void setDegeneratedFacetsChecked(bool);
    bool isDegeneratedFacetsChecked() const;

private:
    Ui_DlgEvaluateSettings* ui;
};

}  // namespace MeshGui

#endif  // MESHGUI_DLG_EVALUATE_SETTINGS_H
