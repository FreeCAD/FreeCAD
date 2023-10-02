/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <sstream>
#include <QDir>
#include <QMessageBox>
#endif

#include <App/Document.h>

#include "DlgProjectUtility.h"
#include "ui_DlgProjectUtility.h"
#include "Application.h"
#include "Command.h"


using namespace Gui::Dialog;


/* TRANSLATOR Gui::Dialog::DlgProjectUtility */

DlgProjectUtility::DlgProjectUtility(QWidget* parent, Qt::WindowFlags fl)
  : QDialog(parent, fl)
  , ui(new Ui_DlgProjectUtility)
{
    ui->setupUi(this);
    connect(ui->extractButton, &QPushButton::clicked, this, &DlgProjectUtility::extractButton);
    connect(ui->createButton, &QPushButton::clicked, this, &DlgProjectUtility::createButton);
    ui->extractSource->setFilter(QString::fromLatin1("%1 (*.FCStd)").arg(tr("Project file")));
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgProjectUtility::~DlgProjectUtility() = default;

void DlgProjectUtility::extractButton()
{
    QString source = ui->extractSource->fileName();
    QString dest = ui->extractDest->fileName();
    if (source.isEmpty()) {
        QMessageBox::critical(this, tr("Empty source"), tr("No source is defined."));
        return;
    }

    if (dest.isEmpty()) {
        QMessageBox::critical(this, tr("Empty destination"), tr("No destination is defined."));
        return;
    }

    tryExtractArchive(source, dest);
}

void DlgProjectUtility::createButton()
{
    QString source = ui->createSource->fileName();
    QString dest = ui->createDest->fileName();
    if (source.isEmpty()) {
        QMessageBox::critical(this, tr("Empty source"), tr("No source is defined."));
        return;
    }
    if (dest.isEmpty()) {
        QMessageBox::critical(this, tr("Empty destination"), tr("No destination is defined."));
        return;
    }

    dest = QDir(dest).absoluteFilePath(QString::fromUtf8("project.fcstd"));

    bool openFile = ui->checkLoadProject->isChecked();
    tryCreateArchive(source, dest, openFile);
}

void DlgProjectUtility::tryExtractArchive(const QString& source, const QString& target)
{
    try {
        std::stringstream str;
        str << "from freecad import project_utility\n";
        str << "project_utility.extractDocument(\"" << (const char*)source.toUtf8()
            << "\", \"" << (const char*)target.toUtf8() << "\")";
        Gui::Command::runCommand(Gui::Command::App, str.str().c_str());
    }
    catch (const Base::Exception& e) {
        QMessageBox::critical(this, tr("Failed to extract project"), QString::fromLatin1(e.what()));
    }
}

void DlgProjectUtility::tryCreateArchive(const QString& source, const QString& target, bool openFile)
{
    try {
        std::stringstream str;
        str << "from freecad import project_utility\n";
        str << "project_utility.createDocument(\"" << (const char*)source.toUtf8()
            << "\", \"" << (const char*)target.toUtf8() << "\")";
        Gui::Command::runCommand(Gui::Command::App, str.str().c_str());
        if (openFile) {
            Application::Instance->open((const char*)target.toUtf8(),"FreeCAD");
        }
    }
    catch (const Base::Exception& e) {
        QMessageBox::critical(this, tr("Failed to create project"), QString::fromLatin1(e.what()));
    }
}

#include "moc_DlgProjectUtility.cpp"
