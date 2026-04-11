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

#pragma once

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <QDialog>

class SoGroup;
class SoTexture2;
class SoTextureCoordinateEnvironment;

namespace Gui
{
namespace Dialog
{

class Ui_TextureMapping;
class GuiExport TextureMapping: public QDialog
{
    Q_OBJECT

public:
    explicit TextureMapping(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~TextureMapping() override;
    void accept() override;
    void reject() override;

private:
    void onFileChooserFileNameSelected(const QString&);
    void onCheckEnvToggled(bool);

protected:
    void changeEvent(QEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;

private:
    SoGroup* grp;
    SoTexture2* tex;
    SoTextureCoordinateEnvironment* env;
    QString fileName;
    Ui_TextureMapping* ui;
};

class TaskTextureMapping: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskTextureMapping();
    ~TaskTextureMapping() override;

public:
    bool accept() override;
    bool reject() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Close;
    }

private:
    TextureMapping* dialog;
};

}  // namespace Dialog
}  // namespace Gui
