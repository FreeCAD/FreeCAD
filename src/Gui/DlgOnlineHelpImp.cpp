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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <QMessageBox>
# include <QDir>
#endif

#include "DlgOnlineHelpImp.h"
#include "PrefWidgets.h"

#include <Base/Parameter.h>
#include <App/Application.h>

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgOnlineHelpImp */

/**
 *  Constructs a DlgOnlineHelpImp which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
DlgOnlineHelpImp::DlgOnlineHelpImp( QWidget* parent )
  : PreferencePage(parent)
{
    this->setupUi(this);

    prefStartPage->setFilter( tr("HTML files (*.html *.htm)") );
    if (prefStartPage->fileName().isEmpty()) {
        prefStartPage->setFileName(getStartpage());
    }
}

/** 
 *  Destroys the object and frees any allocated resources
 */
DlgOnlineHelpImp::~DlgOnlineHelpImp()
{
}

/**
 * Returns the start page for the HelpView. If none is defined the default 
 * start page "<FreeCADHome>/doc/free-cad.sourceforge.net/wiki/index.php.html" 
 * is returned.
 * \remark It is not checked if the returned page really exists.
 */
QString DlgOnlineHelpImp::getStartpage()
{
    QDir docdir = QDir(QString::fromUtf8(App::Application::getHelpDir().c_str()));
    return docdir.absoluteFilePath(QString::fromUtf8("Start_Page.html"));
}

void DlgOnlineHelpImp::saveSettings()
{
    prefStartPage->onSave();
}

void DlgOnlineHelpImp::loadSettings()
{
    prefStartPage->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgOnlineHelpImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgOnlineHelpImp::on_lineEditDownload_fileNameSelected( const QString& url )
{
    QDir dir(url);
    if (dir.exists() && dir.count() == 0) {
        QMessageBox::critical(this, tr("Access denied"), tr("Access denied to '%1'\n\n"
            "Specify another directory, please.").arg(url));
    }
}

#include "moc_DlgOnlineHelpImp.cpp"
