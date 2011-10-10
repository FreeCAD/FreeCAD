/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef PARTGUI_DLGPRIMITIVES_H
#define PARTGUI_DLGPRIMITIVES_H

#include <QPointer>
#include <Gui/InputVector.h>
#include "ui_DlgPrimitives.h"

class SoEventCallback;

namespace PartGui {

class DlgPrimitives : public Gui::LocationDialogComp<Ui_DlgPrimitives>
{
    Q_OBJECT

public:
    DlgPrimitives(QWidget* parent = 0, Qt::WFlags fl = 0);
    ~DlgPrimitives();
    void accept();
    QString toPlacement() const;

private Q_SLOTS:
    void on_viewPositionButton_clicked();

private:
    static void pickCallback(void * ud, SoEventCallback * n);
    QPointer<QWidget> activeView;
};

} // namespace PartGui

#endif // PARTGUI_DLGPRIMITIVES_H
