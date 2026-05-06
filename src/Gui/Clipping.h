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


#pragma once

#include <QDialog>
#include <FCGlobal.h>

namespace Gui
{
class View3DInventor;
namespace Dialog
{

/**
 * @author Werner Mayer
 */
class GuiExport Clipping: public QDialog
{
    Q_OBJECT

public:
    static Clipping* makeDockWidget(Gui::View3DInventor*);
    Clipping(Gui::View3DInventor* view, QWidget* parent = nullptr);
    ~Clipping() override;

protected:
    void setupConnections();
    void onGroupBoxXToggled(bool);
    void onGroupBoxYToggled(bool);
    void onGroupBoxZToggled(bool);
    void onClipXValueChanged(double);
    void onClipYValueChanged(double);
    void onClipZValueChanged(double);
    void onFlipClipXClicked();
    void onFlipClipYClicked();
    void onFlipClipZClicked();
    void onGroupBoxViewToggled(bool);
    void onClipViewValueChanged(double);
    void onFromViewClicked();
    void onAdjustViewdirectionToggled(bool);
    void onDirXValueChanged(double);
    void onDirYValueChanged(double);
    void onDirZValueChanged(double);

public:
    void reject() override;

private:
    class Private;
    Private* d;
};

}  // namespace Dialog
}  // namespace Gui
