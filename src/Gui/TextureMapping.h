/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef GUI_TEXTUREMAPPING_H
#define GUI_TEXTUREMAPPING_H

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

class SoGroup;
class SoTexture2;
class SoTextureCoordinateEnvironment;

namespace Gui {
namespace Dialog {

class Ui_TextureMapping;
class GuiExport TextureMapping : public QDialog
{
    Q_OBJECT

public:
    TextureMapping(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~TextureMapping();
    void accept();
    void reject();

private Q_SLOTS:
    void on_fileChooser_fileNameSelected(const QString&);
    void on_checkEnv_toggled(bool);

protected:
    void changeEvent(QEvent *e);
    void keyPressEvent(QKeyEvent *e);

private:
    SoGroup* grp;
    SoTexture2* tex;
    SoTextureCoordinateEnvironment* env;
    QString fileName;
    Ui_TextureMapping* ui;
};

class TaskTextureMapping : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskTextureMapping();
    ~TaskTextureMapping();

public:
    bool accept();
    bool reject();

    virtual QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Close; }

private:
    TextureMapping* dialog;
    Gui::TaskView::TaskBox* taskbox;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_TEXTUREMAPPING_H
