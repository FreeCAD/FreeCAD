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
# include <QInputDialog>
# include <QHeaderView>
# include <QMessageBox>
#endif

#include "DlgMacroExecuteImp.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "MainWindow.h"
#include "FileDialog.h"
#include "Macro.h"
#include "Document.h"
#include "EditorView.h"
#include "PythonEditor.h"

#include <App/Application.h>
#include <App/Document.h>

using namespace Gui;
using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgMacroExecuteImp */

/**
 *  Constructs a DlgMacroExecuteImp which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
DlgMacroExecuteImp::DlgMacroExecuteImp( QWidget* parent, Qt::WFlags fl )
    : QDialog( parent, fl ), WindowParameter( "Macro" )
{
    this->setupUi(this);
    // retrieve the macro path from parameter or use the user data as default
    std::string path = getWindowParameter()->GetASCII("MacroPath",
        App::Application::getUserAppDataDir().c_str());
    this->macroPath = QString::fromUtf8(path.c_str());
    fileChooser->setFileName(this->macroPath);

    // Fill the List box
    QStringList labels; labels << tr("Macros");
    macroListBox->setHeaderLabels(labels);
    macroListBox->header()->hide();
    fillUpList();
}

/** 
 *  Destroys the object and frees any allocated resources
 */
DlgMacroExecuteImp::~DlgMacroExecuteImp()
{
    // no need to delete child widgets, Qt does it all for us
}

/**
 * Fills up the list with all macro files found in the specified location.
 */
void DlgMacroExecuteImp::fillUpList(void)
{
    // lists all files in macro path
    QDir dir(this->macroPath, QLatin1String("*.FCMacro *.py"));
  
    // fill up with the directory
    macroListBox->clear();
    for (unsigned int i=0; i<dir.count(); i++ ) {
        QTreeWidgetItem* item = new QTreeWidgetItem(macroListBox);
        item->setText(0, dir[i]);
    }
}

/** 
 * Selects a macro file in the list view.
 */
void DlgMacroExecuteImp::on_macroListBox_currentItemChanged(QTreeWidgetItem* item)
{
    LineEditMacroName->setText(item->text(0));
    executeButton->setEnabled(true);
    editButton->setEnabled(true);
    deleteButton->setEnabled(true);
}

/**
 * Executes the selected macro file.
 */
void DlgMacroExecuteImp::accept()
{
    QTreeWidgetItem* item = macroListBox->currentItem();
    if (!item) return;
    
    QDialog::accept();
    QDir dir(this->macroPath);
    QFileInfo fi(dir, item->text(0));
    Application::Instance->macroManager()->run(Gui::MacroManager::File, fi.filePath().toUtf8());
    // after macro run recalculate the document
    if (Application::Instance->activeDocument())
        Application::Instance->activeDocument()->getDocument()->recompute();
}

/**
 * Specify the location of your macro files. The default location is FreeCAD's home path.
 */
void DlgMacroExecuteImp::on_fileChooser_fileNameChanged(const QString& fn)
{
    if (!fn.isEmpty())
    {
        // save the path in the parameters
        this->macroPath = fn;
        getWindowParameter()->SetASCII("MacroPath",fn.toUtf8());
        // fill the list box
        fillUpList();
    }
}

/** 
 * Opens the macro file in an editor.
 */
void DlgMacroExecuteImp::on_editButton_clicked()
{
    QTreeWidgetItem* item = macroListBox->currentItem();
    if (!item) return;

    QDir dir(this->macroPath);
    QString file = QString::fromAscii("%1/%2").arg(dir.absolutePath()).arg(item->text(0));

    Application::Instance->open(file.toUtf8(), "FreeCADGui");
    close();
}

/** Creates a new macro file. */
void DlgMacroExecuteImp::on_createButton_clicked()
{
    // query file name
    QString fn = QInputDialog::getText(this, tr("Macro file"), tr("Enter a file name, please:"),
        QLineEdit::Normal, QString::null, 0);
    if (!fn.isEmpty())
    {
        QString suffix = QFileInfo(fn).suffix().toLower();
        if (suffix != QLatin1String("fcmacro") && suffix != QLatin1String("py"))
            fn += QLatin1String(".FCMacro");
        QDir dir(this->macroPath);
        QFileInfo fi(dir, fn);
        if (fi.exists() && fi.isFile())
        {
            QMessageBox::warning(this, tr("Existing file"),
                tr("'%1'.\nThis file already exists.").arg(fi.fileName()));
        }
        else
        {
            QFile file(fi.absoluteFilePath());
            if (!file.open(QFile::WriteOnly)) {
                QMessageBox::warning(this, tr("Cannot create file"),
                    tr("Creation of file '%1' failed.").arg(fi.absoluteFilePath()));
                return;
            }
            file.close();
            PythonEditor* editor = new PythonEditor();
            editor->setWindowIcon(Gui::BitmapFactory().pixmap("python_small"));
            PythonEditorView* edit = new PythonEditorView(editor, getMainWindow());
            edit->open(fi.absoluteFilePath());
            edit->setWindowTitle(fn);
            edit->resize(400, 300);
            getMainWindow()->addWindow(edit);
            close();
        }
    }
}

/** Deletes the selected macro file from your harddisc. */
void DlgMacroExecuteImp::on_deleteButton_clicked()
{
    QTreeWidgetItem* item = macroListBox->currentItem();
    if (!item) return;

    QString fn = item->text(0);
    int ret = QMessageBox::question(this, tr("Delete macro"),
        tr("Do you really want to delete the macro '%1'?").arg( fn ),
        QMessageBox::Yes, QMessageBox::No|QMessageBox::Default|QMessageBox::Escape);
    if (ret == QMessageBox::Yes)
    {
        QDir dir(this->macroPath);
        dir.remove(fn);
        int index = macroListBox->indexOfTopLevelItem(item);
        macroListBox->takeTopLevelItem(index);
        delete item;
    }
}

#include "moc_DlgMacroExecuteImp.cpp"
