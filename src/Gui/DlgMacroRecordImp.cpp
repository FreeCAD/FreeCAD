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
# include <QDir>
# include <QFile>
# include <QFileInfo>
# include <QMessageBox>
#endif

#include "DlgMacroRecordImp.h"
#include "ui_DlgMacroRecord.h"
#include "Application.h"
#include "FileDialog.h"
#include "Macro.h"
#include "MainWindow.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgMacroRecordImp */

/**
 *  Constructs a DlgMacroRecordImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgMacroRecordImp::DlgMacroRecordImp( QWidget* parent, Qt::WindowFlags fl )
    : QDialog(parent, fl)
    , WindowParameter("Macro")
    , ui(new Ui_DlgMacroRecord)
{
    ui->setupUi(this);
    setupConnections();

    // get the macro home path
    this->macroPath = QString::fromUtf8(getWindowParameter()->GetASCII("MacroPath",
        App::Application::getUserMacroDir().c_str()).c_str());
    this->macroPath = QDir::toNativeSeparators(QDir(this->macroPath).path() + QDir::separator());

    // set the edit fields
    ui->lineEditMacroPath->setText(macroPath);

    // get a pointer to the macro manager
    this->macroManager = Application::Instance->macroManager();

    // check if a macro recording is in progress
    this->macroManager->isOpen() ? ui->buttonStart->setEnabled(false) : ui->buttonStop->setEnabled(false);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgMacroRecordImp::~DlgMacroRecordImp() = default;

void DlgMacroRecordImp::setupConnections()
{
    connect(ui->buttonStart, &QPushButton::clicked,
            this, &DlgMacroRecordImp::onButtonStartClicked);
    connect(ui->buttonStop, &QPushButton::clicked,
            this, &DlgMacroRecordImp::onButtonStopClicked);
    connect(ui->buttonClose, &QPushButton::clicked,
            this, &DlgMacroRecordImp::onButtonCloseClicked);
    connect(ui->pushButtonChooseDir, &QPushButton::clicked,
            this, &DlgMacroRecordImp::onButtonChooseDirClicked);
    connect(ui->lineEditMacroPath, &QLineEdit::textChanged,
            this, &DlgMacroRecordImp::onMacroPathTextChanged);
}

/**
 * Starts the record of the macro.
 */
void DlgMacroRecordImp::onButtonStartClicked()
{
    // test if the path already set
    if (ui->lineEditPath->text().isEmpty()) {
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
    QString fn = this->macroPath + ui->lineEditPath->text();
    if (!fn.endsWith(QLatin1String(".FCMacro"))) {
        fn += QLatin1String(".FCMacro");
    }

    QFileInfo fi(fn);
    if (fi.isFile() && fi.exists()) {
        if (QMessageBox::question(this, tr("Existing macro"),
                tr("The macro '%1' already exists. Do you want to overwrite?").arg(fn),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No) == QMessageBox::No)
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

    ui->buttonStart->setEnabled(false);
    ui->buttonStop->setEnabled(true);
    ui->buttonClose->setEnabled(false);
    QDialog::accept();
}

/**
 * Abort the macro.
 */
void DlgMacroRecordImp::onButtonCloseClicked()
{
    if (this->macroManager->isOpen()) {
        this->macroManager->cancel();
    }

    QDialog::reject();
}

/**
 * Stops the record of the macro and save to the file.
 */
void DlgMacroRecordImp::onButtonStopClicked()
{
    if (this->macroManager->isOpen()) {
        // ends the macrorecording and save the file...
        this->macroManager->commit();
    }

    ui->buttonStart->setEnabled(true);
    ui->buttonStop->setEnabled(false);
    ui->buttonClose->setEnabled(true);
    QDialog::accept();
}

void DlgMacroRecordImp::onButtonChooseDirClicked()
{
    QString newDir = QFileDialog::getExistingDirectory(nullptr,tr("Choose macro directory"),macroPath);
    if (!newDir.isEmpty()) {
        macroPath = QDir::toNativeSeparators(newDir + QDir::separator());
        ui->lineEditMacroPath->setText(macroPath);
        getWindowParameter()->SetASCII("MacroPath",macroPath.toUtf8());
    }
}

void DlgMacroRecordImp::onMacroPathTextChanged (const QString & newDir)
{
    macroPath = newDir;
}


#include "moc_DlgMacroRecordImp.cpp"

