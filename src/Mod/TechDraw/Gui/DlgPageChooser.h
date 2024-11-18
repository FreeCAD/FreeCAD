/****************************************************************************
 *   Copyright (c) 2021 Wanderer Fan <wandererfan@gmail.com>                *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/
#ifndef GUI_DLGPAGECHOOSER_H
#define GUI_DLGPAGECHOOSER_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QDialog>

namespace TechDrawGui {

class Ui_DlgPageChooser;
class TechDrawGuiExport DlgPageChooser : public QDialog
{
    Q_OBJECT

public:
    DlgPageChooser(const std::vector<std::string> labels,
                   const std::vector<std::string> names,
                   QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgPageChooser() override;

    std::string getSelection() const;
    void accept() override;
    void reject() override;

private Q_SLOTS:

private:
    void fillList(std::vector<std::string> labels, std::vector<std::string> names);

    Ui_DlgPageChooser* ui;
};

} // namespace Gui


#endif // GUI_DLGPAGECHOOSER_H

