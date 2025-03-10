/****************************************************************************
 *   Copyright (c) 2022 Wanderer Fan <wandererfan@gmail.com>                *
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

#ifndef GUI_DLGEDITABLETEXT_H
#define GUI_DLGEDITABLETEXT_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QDialog>
#include <QPlainTextEdit>

class QListWidgetItem;

namespace TechDrawGui
{

class Ui_DlgStringListEditor;
class TechDrawGuiExport DlgStringListEditor: public QDialog
{
    Q_OBJECT

public:
    explicit DlgStringListEditor(const std::vector<std::string> texts,
                                 QWidget* parent = nullptr,
                                 Qt::WindowFlags fl = Qt::WindowFlags());

    std::vector<std::string> getTexts() const;
    void accept() override;
    void reject() override;

private:
    void fillList(std::vector<std::string> texts);
    QPlainTextEdit* textEdit;
    std::vector<std::string> texts;
};

}  // namespace TechDrawGui


#endif  // GUI_DLGEDITABLETEXT_H
