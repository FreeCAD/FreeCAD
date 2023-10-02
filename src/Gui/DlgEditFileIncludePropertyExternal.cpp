/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include "DlgEditFileIncludePropertyExternal.h"
#include "Application.h"
#include "FileDialog.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgEditFileIncludePropertyExternal */

/**
 *  Constructs a DlgEditFileIncludePropertyExternal which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgEditFileIncludePropertyExternal::
DlgEditFileIncludePropertyExternal(App::PropertyFileIncluded& Prop,
                                   QWidget* parent, Qt::WindowFlags fl)
    : DlgRunExternal(parent, fl), Prop(Prop)
{

}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgEditFileIncludePropertyExternal::~DlgEditFileIncludePropertyExternal() = default;

int DlgEditFileIncludePropertyExternal::processFile()
{
    QFileInfo file(QString::fromUtf8(Prop.getValue()));
    assert(file.exists());

    QDir tmp = QString::fromUtf8(App::Application::getUserCachePath().c_str());
    QString TempFile = tmp.absoluteFilePath(file.fileName());
    QFile::remove(TempFile);

    QFile::copy(file.absoluteFilePath(), TempFile);

    addArgument(TempFile);

    int ret = DlgRunExternal::runProcess();

    if (ret == QDialog::Accepted)
        Prop.setValue(TempFile.toUtf8());

    QFile::remove(TempFile);
    return ret;
}

#include "moc_DlgEditFileIncludePropertyExternal.cpp"
