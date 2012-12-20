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
#ifndef _PreComp_
# include <QMessageBox>
# include <QDir>
# include <QFile>
# include <QFileInfo>
#endif

#include "Macro.h"
#include "Application.h"
#include "MainWindow.h"
#include "DlgMacroRecordImp.h"
#include "FileDialog.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgMacroRecordImp */

/**
 *  Constructs a DlgMacroRecordImp which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
DlgMacroRecordImp::DlgMacroRecordImp( QWidget* parent, Qt::WFlags fl )
    : QDialog(parent, fl), WindowParameter("Macro")
{
    this->setupUi(this);

    // get the macro home path
    this->macroPath = QString::fromUtf8(getWindowParameter()->GetASCII("MacroPath",
        App::Application::getUserAppDataDir().c_str()).c_str());
    this->macroPath = QDir::toNativeSeparators(QDir(this->macroPath).path() + QDir::separator());

    // set the edit fields
    this->lineEditMacroPath->setText(macroPath);

    // get a pointer to the macro manager
    this->macroManager = Application::Instance->macroManager();

    // check if a macro recording is in progress
    this->macroManager->isOpen() ? buttonStart->setEnabled(false) : buttonStop->setEnabled(false);
}

/** 
 *  Destroys the object and frees any allocated resources
 */
DlgMacroRecordImp::~DlgMacroRecordImp()
{
    // no need to delete child widgets, Qt does it all for us
}

/**
 * Starts the record of the macro.
 */
void DlgMacroRecordImp::on_buttonStart_clicked()
{
    // test if the path already set
    if (lineEditPath->text().isEmpty()) {
        QMessageBox::information(getMainWindow(), tr("Macro recorder"),
            tr("Specify first a place to save."));
        return;
    }

    QDir dir(macroPath);
    if (!dir.exists()) {
        QMessageBox::information(getMainWindow(), tr("Macro recorder"),
            tr("The macro directory doesn't exist. Please, choose another one."));
        return;
    }

    // search in the macro path first for an already existing macro
    QString fn = this->macroPath + lineEditPath->text();
    if (!fn.endsWith(QLatin1String(".FCMacro"))) fn += QLatin1String(".FCMacro");
    QFileInfo fi(fn);
    if (fi.isFile() && fi.exists()) {
        if (QMessageBox::question(this, tr("Existing macro"),
                tr("The macro '%1' already exists. Do you want to overwrite?").arg(fn),
                QMessageBox::Yes,
                QMessageBox::No|
                QMessageBox::Default|
                QMessageBox::Escape) == QMessageBox::No)
        return;
    }

    QFile file(fn);
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox::information(getMainWindow(), tr("Macro recorder"),
            tr("You have no write permission for the directory. Please, choose another one."));
        return;
    }
    file.close();

    // open the macro recording
    this->macroManager->open(MacroManager::File, fn.toUtf8().constData());
    accept();
}

/**
 * Abort the macro.
 */
void DlgMacroRecordImp::on_buttonCancel_clicked()
{
    if (this->macroManager->isOpen()) {
        this->macroManager->cancel();
    }
  
    QDialog::reject();
}

/**
 * Stops the record of the macro and save to the file.
 */
void DlgMacroRecordImp::on_buttonStop_clicked()
{
    if(this->macroManager->isOpen()) {
        // ends the macrorecording and save the file...
        this->macroManager->commit();
    }

    QDialog::accept();
}

void DlgMacroRecordImp::on_pushButtonChooseDir_clicked()
{
    QString newDir = QFileDialog::getExistingDirectory(0,tr("Choose macro directory"),macroPath);
    if (!newDir.isEmpty()) {
        macroPath = QDir::toNativeSeparators(newDir + QDir::separator());
        this->lineEditMacroPath->setText(macroPath);
        getWindowParameter()->SetASCII("MacroPath",macroPath.toUtf8());
    }
}

void DlgMacroRecordImp::on_lineEditMacroPath_textChanged (const QString & newDir)
{
    macroPath = newDir;
}


#include "moc_DlgMacroRecordImp.cpp"

