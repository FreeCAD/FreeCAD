/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_DIALOG_CLIPPING_H
#define GUI_DIALOG_CLIPPING_H

#include <QDialog>

namespace Gui {
class View3DInventor;
namespace Dialog {

/**
 * @author Werner Mayer
 */
class GuiExport Clipping : public QDialog
{
    Q_OBJECT

public:
    static Clipping* makeDockWidget(Gui::View3DInventor*);
    Clipping(Gui::View3DInventor* view, QWidget* parent = nullptr);
    ~Clipping();

protected Q_SLOTS:
    void on_groupBoxX_toggled(bool);
    void on_groupBoxY_toggled(bool);
    void on_groupBoxZ_toggled(bool);
    void on_clipX_valueChanged(double);
    void on_clipY_valueChanged(double);
    void on_clipZ_valueChanged(double);
    void on_flipClipX_clicked();
    void on_flipClipY_clicked();
    void on_flipClipZ_clicked();
    void on_groupBoxView_toggled(bool);
    void on_clipView_valueChanged(double);
    void on_fromView_clicked();
    void on_adjustViewdirection_toggled(bool);
    void on_dirX_valueChanged(double);
    void on_dirY_valueChanged(double);
    void on_dirZ_valueChanged(double);

public:
    void reject();

private:
    class Private;
    Private* d;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_CLIPPING_H
