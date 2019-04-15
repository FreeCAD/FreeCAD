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
#include "Command.h"
#include "MainWindow.h"
#include "FileDialog.h"
#include "Macro.h"
#include "Document.h"
#include "EditorView.h"
#include "PythonEditor.h"

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Interpreter.h>

using namespace Gui;
using namespace Gui::Dialog;

namespace Gui {
    namespace Dialog {
        class MacroItem : public QTreeWidgetItem
        {
        public:
            MacroItem(QTreeWidget * widget, bool systemwide)
            : QTreeWidgetItem(widget),
            systemWide(systemwide){}

            ~MacroItem(){}

            bool systemWide;
        };
    }
}


/* TRANSLATOR Gui::Dialog::DlgMacroExecuteImp */

/**
 *  Constructs a DlgMacroExecuteImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgMacroExecuteImp::DlgMacroExecuteImp( QWidget* parent, Qt::WindowFlags fl )
    : QDialog( parent, fl ), WindowParameter( "Macro" )
{
    this->setupUi(this);
    // retrieve the macro path from parameter or use the user data as default
    std::string path = getWindowParameter()->GetASCII("MacroPath",
        App::Application::getUserMacroDir().c_str());
    this->macroPath = QString::fromUtf8(path.c_str());
    fileChooser->setFileName(this->macroPath);

    // Fill the List box
    QStringList labels; labels << tr("Macros");
    userMacroListBox->setHeaderLabels(labels);
    userMacroListBox->header()->hide();
    systemMacroListBox->setHeaderLabels(labels);
    systemMacroListBox->header()->hide();
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
    userMacroListBox->clear();
    for (unsigned int i=0; i<dir.count(); i++ ) {
        MacroItem* item = new MacroItem(userMacroListBox,false);
        item->setText(0, dir[i]);
    }

    QString dirstr = QString::fromUtf8(App::GetApplication().getHomePath()) + QString::fromUtf8("Macro");
    dir = QDir(dirstr, QLatin1String("*.FCMacro *.py"));

    systemMacroListBox->clear();
    if (dir.exists()) {
        for (unsigned int i=0; i<dir.count(); i++ ) {
            MacroItem* item = new MacroItem(systemMacroListBox,true);
            item->setText(0, dir[i]);
        }
    }
}

/** 
 * Selects a macro file in the list view.
 */
void DlgMacroExecuteImp::on_userMacroListBox_currentItemChanged(QTreeWidgetItem* item)
{
    if (item) {
        LineEditMacroName->setText(item->text(0));

        executeButton->setEnabled(true);
        deleteButton->setEnabled(true);
        createButton->setEnabled(true);
        editButton->setEnabled(true);
        renameButton->setEnabled(true);
        duplicateButton->setEnabled(true);
    }
    else {
        executeButton->setEnabled(false);
        deleteButton->setEnabled(false);
        createButton->setEnabled(true);
        editButton->setEnabled(false);
        renameButton->setEnabled(false);
        duplicateButton->setEnabled(false);
    }
}

void DlgMacroExecuteImp::on_systemMacroListBox_currentItemChanged(QTreeWidgetItem* item)
{
    if (item) {
        LineEditMacroName->setText(item->text(0));

        executeButton->setEnabled(true);
        deleteButton->setEnabled(false);
        createButton->setEnabled(false);
        editButton->setEnabled(true); //look but don't touch
        renameButton->setEnabled(false);
        duplicateButton->setEnabled(false);
    }
    else {
        executeButton->setEnabled(false);
        deleteButton->setEnabled(false);
        createButton->setEnabled(false);
        editButton->setEnabled(false);
        renameButton->setEnabled(false);
        duplicateButton->setEnabled(false);
    }
}

void DlgMacroExecuteImp::on_tabMacroWidget_currentChanged(int index)
{
    QTreeWidgetItem* item;

    if (index == 0) { //user-specific
        item = userMacroListBox->currentItem();
        if (item) {
            executeButton->setEnabled(true);
            deleteButton->setEnabled(true);
            createButton->setEnabled(true);
            editButton->setEnabled(true);
            renameButton->setEnabled(true);
            duplicateButton->setEnabled(true);
        }
        else {
            executeButton->setEnabled(false);
            deleteButton->setEnabled(false);
            createButton->setEnabled(true);
            editButton->setEnabled(false);
            renameButton->setEnabled(false);
            duplicateButton->setEnabled(false);
        }
    }
    else { //index==1 system-wide
        item = systemMacroListBox->currentItem();

        if (item) {
            executeButton->setEnabled(true);
            deleteButton->setEnabled(false);
            createButton->setEnabled(false);
            editButton->setEnabled(true); //but you can't save it
            renameButton->setEnabled(false);
            duplicateButton->setEnabled(false);
        }
        else {
            executeButton->setEnabled(false);
            deleteButton->setEnabled(false);
            createButton->setEnabled(false);
            editButton->setEnabled(false);
            renameButton->setEnabled(false);
            duplicateButton->setEnabled(false);
        }
    }

    if (item) {
        LineEditMacroName->setText(item->text(0));
    }
    else {
        LineEditMacroName->clear();
    }
}

/**
 * Executes the selected macro file.
 */
void DlgMacroExecuteImp::accept()
{
    QTreeWidgetItem* item;

    int index = tabMacroWidget->currentIndex();
    if (index == 0) { //user-specific
        item = userMacroListBox->currentItem();
    }
    else {
        //index == 1 system-wide
        item = systemMacroListBox->currentItem();
    }
    if (!item)
        return;

    QDialog::accept();

    MacroItem * mitem = static_cast<MacroItem *>(item);

    QDir dir;

    if (!mitem->systemWide){
        dir =QDir(this->macroPath);
    }
    else {
        QString dirstr = QString::fromUtf8(App::GetApplication().getHomePath()) + QString::fromUtf8("Macro");
        dir = QDir(dirstr);
    }

    QFileInfo fi(dir, item->text(0));
    try {
        Application::Instance->macroManager()->run(Gui::MacroManager::File, fi.filePath().toUtf8());
        // after macro run recalculate the document
        if (Application::Instance->activeDocument())
            Application::Instance->activeDocument()->getDocument()->recompute();
    }
    catch (const Base::SystemExitException&) {
        // handle SystemExit exceptions
        Base::PyGILStateLocker locker;
        Base::PyException e;
        e.ReportException();
    }
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
    QDir dir;
    QTreeWidgetItem* item = 0;

    int index = tabMacroWidget->currentIndex();
    if (index == 0) { //user-specific
        item = userMacroListBox->currentItem();
        dir.setPath(this->macroPath);
    }
    else {
        //index == 1 system-wide
        item = systemMacroListBox->currentItem();
        dir.setPath(QString::fromUtf8(App::GetApplication().getHomePath()) + QString::fromUtf8("Macro"));
    }

    if (!item)
        return;

    MacroItem * mitem = static_cast<MacroItem *>(item);

    QString file = QString::fromLatin1("%1/%2").arg(dir.absolutePath(), item->text(0));
    PythonEditor* editor = new PythonEditor();
    editor->setWindowIcon(Gui::BitmapFactory().iconFromTheme("applications-python"));
    PythonEditorView* edit = new PythonEditorView(editor, getMainWindow());
    edit->open(file);
    edit->resize(400, 300);
    getMainWindow()->addWindow(edit);

    if (mitem->systemWide) {
        editor->setReadOnly(true);
        QString shownName;
        shownName = QString::fromLatin1("%1[*] - [%2]").arg(item->text(0), tr("Read-only"));
        edit->setWindowTitle(shownName);
    }

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
        // create the macroPath if nonexistent
        if (!dir.exists()) {
            dir.mkpath(this->macroPath);
        }
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
            editor->setWindowIcon(Gui::BitmapFactory().iconFromTheme("applications-python"));
            PythonEditorView* edit = new PythonEditorView(editor, getMainWindow());
            edit->open(fi.absoluteFilePath());
            edit->setWindowTitle(QString::fromLatin1("%1[*]").arg(fn));
            edit->resize(400, 300);
            getMainWindow()->addWindow(edit);
            close();
        }
    }
}

/** Deletes the selected macro file from your harddisc. */
void DlgMacroExecuteImp::on_deleteButton_clicked()
{
    QTreeWidgetItem* item = userMacroListBox->currentItem();
    if (!item)
        return;

    MacroItem * mitem = static_cast<MacroItem *>(item);

    if (mitem->systemWide) {
        QMessageBox::critical(qApp->activeWindow(), QObject::tr("Delete macro"),
            QObject::tr("Not allowed to delete system-wide macros"));
        return;
    }

    QString fn = item->text(0);
    int ret = QMessageBox::question(this, tr("Delete macro"),
        tr("Do you really want to delete the macro '%1'?").arg( fn ),
        QMessageBox::Yes, QMessageBox::No|QMessageBox::Default|QMessageBox::Escape);
    if (ret == QMessageBox::Yes)
    {
        QDir dir(this->macroPath);
        dir.remove(fn);
        int index = userMacroListBox->indexOfTopLevelItem(item);
        userMacroListBox->takeTopLevelItem(index);
        delete item;
    }
}

/**
 * renames the selected macro
 */
void DlgMacroExecuteImp::on_renameButton_clicked()
{
    QDir dir;
    QTreeWidgetItem* item = 0;

    int index = tabMacroWidget->currentIndex();
    if (index == 0) { //user-specific
        item = userMacroListBox->currentItem();
        dir.setPath(this->macroPath);
    }

    if (!item)
        return;

    QString oldName = item->text(0);
    QFileInfo oldfi(dir, oldName);
    QFile oldfile(oldfi.absoluteFilePath());

    // query new name
    QString fn = QInputDialog::getText(this, tr("Renaming Macro File"),
        tr("Enter new name:"), QLineEdit::Normal, oldName, 0);
    if (!fn.isEmpty() && fn != oldName) {
        QString suffix = QFileInfo(fn).suffix().toLower();
        if (suffix != QLatin1String("fcmacro") && suffix != QLatin1String("py"))
            fn += QLatin1String(".FCMacro");
        QFileInfo fi(dir, fn);
        // check if new name exists
        if (fi.exists()) {
            QMessageBox::warning(this, tr("Existing file"),
                tr("'%1'\n already exists.").arg(fi.absoluteFilePath()));
        }
        else if (!oldfile.rename(fi.absoluteFilePath())) {
            QMessageBox::warning(this, tr("Rename Failed"),
                tr("Failed to rename to '%1'.\nPerhaps a file permission error?").arg(fi.absoluteFilePath()));
        }
        else {
            // keep the item selected although it's not necessarily in alphabetic order
            item->setText(0, fn);
            LineEditMacroName->setText(fn);
        }
    }
}
/**Duplicates selected macro
 * New file has same name as original but with "@" and 3-digit number appended
 * Begins with "@001" and increments until available name is found
 * "MyMacro.FCMacro" becomes "MyMacro@001.FCMacro"
 * "MyMacro@002.FCMacro.py" becomes "MyMacro@003.FCMacro.py" unless there is
 * no already existing "MyMacro@001.FCMacro.py"
 */
void DlgMacroExecuteImp::on_duplicateButton_clicked()
{
    QDir dir;
    QTreeWidgetItem* item = 0;

    int index = tabMacroWidget->currentIndex();
    if (index == 0) { //user-specific
        item = userMacroListBox->currentItem();
        dir.setPath(this->macroPath);
    }

    if (!item){
        return;
    }

    QString oldName = item->text(0);
    QFileInfo oldfi(dir, oldName);
    QFile oldfile(oldfi.absoluteFilePath());
    QString completeSuffix = oldfi.completeSuffix(); //everything after the first "."
    QString baseName = oldfi.baseName(); //everything before first "."
    QString neutralSymbol = QString::fromStdString("@");
    QString last3 = baseName.right(3);
    bool ok = true; //was conversion to int successful?
    int nLast3 = last3.toInt(&ok);
    last3 = QString::fromStdString("001"); //increment beginning with 001 no matter what
    if (ok ){
        //last3 were all digits, so we strip them from the base name
        if (baseName.size()>3){ //if <= 3 leave be (e.g. 2.py becomes 2@001.py)
            baseName = baseName.left(baseName.size()-3); //strip digits
            if (baseName.endsWith(neutralSymbol)){
                baseName = baseName.left(baseName.size()-1); //trim the "@", will be added back later
            }
        }
    }
    //at this point baseName = the base name without any digits, e.g. "MyMacro"
    //neutralSymbol = "@"
    //last3 is a string representing 3 digits, always "001" at this time
    //completeSuffix = FCMacro or py or FCMacro.py or else suffix will become FCMacro below

    QString oldNameDigitized = baseName+neutralSymbol+last3+QString::fromStdString(".")+completeSuffix;
    QFileInfo fi(dir, oldNameDigitized);
    // increment until we find available name with smallest digits
    // test from "001" through "999", then give up and let user enter name of choice
    while (fi.exists()) {
        nLast3 = last3.toInt()+1;
        if (nLast3 >=1000){ //avoid infinite loop, 999 files will have to be enough
            break;
        }
        last3 = QString::number(nLast3);
        while (last3.size()<3){
            last3.prepend(QString::fromStdString("0")); //pad 0's if needed
        }
        oldNameDigitized = baseName+neutralSymbol+last3+QString::fromStdString(".")+completeSuffix;
        fi = QFileInfo(dir,oldNameDigitized);
    }

    // give user a chance to pick a different name from digitized name suggested
    QString fn = QInputDialog::getText(this, tr("Duplicate Macro"),
        tr("Enter new name:"), QLineEdit::Normal, oldNameDigitized, 0);
    if (!fn.isEmpty() && fn != oldName) {
        QString suffix = QFileInfo(fn).suffix().toLower();
        if (suffix != QLatin1String("fcmacro") && suffix != QLatin1String("py")){
            fn += QLatin1String(".FCMacro");
        }
        QFileInfo fi(dir, fn);
        // check again if new name exists in case user changed it
        if (fi.exists()) {
            QMessageBox::warning(this, tr("Existing file"),
                tr("'%1'\n already exists.").arg(fi.absoluteFilePath()));
        }
        else if (!oldfile.copy(fi.absoluteFilePath())) {
            QMessageBox::warning(this, tr("Duplicate Failed"),
                tr("Failed to duplicate to '%1'.\nPerhaps a file permission error?").arg(fi.absoluteFilePath()));
        }

        this->fillUpList(); //repopulate list to show new file
    }

}

/**
 * convenience link button to open tools -> addon manager
 * from within macro dialog
 */
void DlgMacroExecuteImp::on_addonsButton_clicked()
{
    CommandManager& rMgr=Application::Instance->commandManager();
    rMgr.runCommandByName("Std_AddonMgr");
    this->fillUpList();
}

#include "moc_DlgMacroExecuteImp.cpp"
