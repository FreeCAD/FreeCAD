 /**************************************************************************
 *   Copyright (c) 2019 Franck Jullien <franck.jullien@gmail.com>          *
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


#ifndef DRAWINGGUI_DLGBALLOON_H
#define DRAWINGGUI_DLGBALLOON_H

#include <QDialog>
#include <QString>

#include <Mod/TechDraw/Gui/ui_DlgBalloon.h>

namespace TechDrawGui {

class DlgBalloon : public QDialog, public Ui_dlgBalloon
{
    Q_OBJECT

public:
    DlgBalloon( QWidget *parent = nullptr );
    virtual ~DlgBalloon() = default;

    void setValue(std::string value);
    QString getValue(void);
    void setScale(double value);
    double getScale(void);
    void populateComboBox(QComboBox *box, const char **values, const char *setVal);

public Q_SLOTS:
    void accept();
    void reject();

protected:
    void changeEvent(QEvent *e);
};

} // namespace TechDrawGui

#endif // DRAWINGGUI_DLGBALLOON_H
