/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_DIALOG_DLGGENERALIMP_H
#define GUI_DIALOG_DLGGENERALIMP_H

#include "ui_DlgGeneral.h"
#include "PropertyPage.h"

class QTabWidget;

namespace Gui {
namespace Dialog {

/** This class implements the settings for the application.
 *  You can change window style, size of pixmaps, size of recent file list and so on
 *  \author Werner Mayer
 */
class DlgGeneralImp : public PreferencePage, public Ui_DlgGeneral
{
    Q_OBJECT

public:
    DlgGeneralImp( QWidget* parent = 0 );
    ~DlgGeneralImp();

    void saveSettings();
    void loadSettings();
    bool eventFilter ( QObject* o, QEvent* e );

protected:
    void changeEvent(QEvent *e);

private:
    void setRecentFileSize();
    QTabWidget* watched;
    QString selectedStyleSheet;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGGENERALIMP_H
