/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <qlineedit.h>
#endif

#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>
#include "DlgPartImportStepImp.h"

using namespace PartGui;

/* TRANSLATOR PartGui::DlgPartImportStepImp */

/* 
 *  Constructs a DlgPartImportStep which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgPartImportStepImp::DlgPartImportStepImp( QWidget* parent, Qt::WindowFlags fl )
    : QDialog( parent, fl )
{
    this->setupUi(this);
}

/*  
 *  Destroys the object and frees any allocated resources
 */
DlgPartImportStepImp::~DlgPartImportStepImp()
{
    // no need to delete child widgets, Qt does it all for us
}

/* 
 * public slot
 */
void DlgPartImportStepImp::OnApply()
{
    qWarning( "DlgPartImportStepImp::OnApply() not yet implemented!" ); 
}

void DlgPartImportStepImp::onChooseFileName()
{
    QString fn = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(), QString::null, QString::null,
        QString::fromLatin1("%1 (*.stp *.step);;%2 (*.*)"))
        .arg(tr("STEP"),
             tr("All Files"));
    if (!fn.isEmpty()) {
        FileName->setText(fn);
    }
}


#include "moc_DlgPartImportStepImp.cpp"
