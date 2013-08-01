/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_DIALOG_DLGPROJECTINFORMATIONIMP_H
#define GUI_DIALOG_DLGPROJECTINFORMATIONIMP_H

#include "ui_DlgProjectInformation.h"

namespace App {
class Document;
}

namespace Gui {

namespace Dialog {

class DlgProjectInformationImp : public QDialog, public Ui_DlgProjectInformation
{
public:
    DlgProjectInformationImp( App::Document* doc, QWidget* parent = 0, Qt::WFlags fl = 0 );
    ~DlgProjectInformationImp();

protected:
    void accept();
    void open_url();
    App::Document* _doc;
};

} // namespace Dialog
} // namespace Gui


#endif // GUI_DIALOG_DLGPROJECTINFORMATIONIMP_H

