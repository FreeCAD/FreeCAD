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
# include <QLabel>
# include <QMessageBox>
# include <QComboBox>
# include <QSignalBlocker>
# include <QTextStream>
#endif

#include <App/Document.h>
#include <Base/Interpreter.h>

#include "DlgMacroExecuteImp.h"
#include "ui_DlgMacroExecute.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "DlgCustomizeImp.h"
#include "DlgToolbarsImp.h"
#include "Document.h"
#include "EditorView.h"
#include "Macro.h"
#include "MainWindow.h"
#include "PythonEditor.h"
#include "WaitCursor.h"


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

            ~MacroItem() override = default;

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
    : QDialog( parent, fl )
    , WindowParameter( "Macro" )
    , ui(new Ui_DlgMacroExecute)
{
    watcher = std::make_unique<PythonTracingWatcher>(this);
    ui->setupUi(this);
    setupConnections();

    // retrieve the macro path from parameter or use the user data as default
    {
        QSignalBlocker blocker(ui->fileChooser);
        std::string path = getWindowParameter()->GetASCII("MacroPath",
            App::Application::getUserMacroDir().c_str());
        this->macroPath = QString::fromUtf8(path.c_str());
        ui->fileChooser->setFileName(this->macroPath);
    }

    // Fill the List box
    QStringList labels; labels << tr("Macros");
    ui->userMacroListBox->setHeaderLabels(labels);
    ui->userMacroListBox->header()->hide();
    ui->systemMacroListBox->setHeaderLabels(labels);
    ui->systemMacroListBox->header()->hide();
    fillUpList();
    ui->LineEditFind->setFocus();
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgMacroExecuteImp::~DlgMacroExecuteImp() = default;

void DlgMacroExecuteImp::setupConnections()
{
    connect(ui->fileChooser, &FileChooser::fileNameChanged,
            this, &DlgMacroExecuteImp::onFileChooserFileNameChanged);
    connect(ui->createButton, &QPushButton::clicked,
            this, &DlgMacroExecuteImp::onCreateButtonClicked);
    connect(ui->deleteButton, &QPushButton::clicked,
            this, &DlgMacroExecuteImp::onDeleteButtonClicked);
    connect(ui->editButton, &QPushButton::clicked,
            this, &DlgMacroExecuteImp::onEditButtonClicked);
    connect(ui->renameButton, &QPushButton::clicked,
            this, &DlgMacroExecuteImp::onRenameButtonClicked);
    connect(ui->duplicateButton, &QPushButton::clicked,
            this, &DlgMacroExecuteImp::onDuplicateButtonClicked);
    connect(ui->toolbarButton, &QPushButton::clicked,
            this, &DlgMacroExecuteImp::onToolbarButtonClicked);
    connect(ui->addonsButton, &QPushButton::clicked,
            this, &DlgMacroExecuteImp::onAddonsButtonClicked);
    connect(ui->userMacroListBox, &QTreeWidget::currentItemChanged,
            this, &DlgMacroExecuteImp::onUserMacroListBoxCurrentItemChanged);
    connect(ui->systemMacroListBox, &QTreeWidget::currentItemChanged,
            this, &DlgMacroExecuteImp::onSystemMacroListBoxCurrentItemChanged);
    connect(ui->tabMacroWidget, &QTabWidget::currentChanged,
            this, &DlgMacroExecuteImp::onTabMacroWidgetCurrentChanged);
    connect(ui->LineEditFind, &QLineEdit::textChanged,
            this, &DlgMacroExecuteImp::onLineEditFindTextChanged);
    connect(ui->LineEditFindInFiles, &QLineEdit::textChanged,
            this, &DlgMacroExecuteImp::onLineEditFindInFilesTextChanged);
}

/** Take a folder and return a StringList of the filenames in it
 * filtered by both filename *and* by content, if the user has
 * put text in one or both of the search line edits.
 *
 * First filtering is done by file name, which reduces the
 * number of files to open and read (relatively expensive operation).
 *
 * Then we filter by file content after reducing the number of files
 * to open and read.  But both loops are skipped if there is no text
 * in either of the line edits.
 *
 * We do this as another function in order to reuse this code for
 * doing both the User and System macro list boxes in the fillUpList() function.
 */

QStringList DlgMacroExecuteImp::filterFiles(const QString& folder){
    QDir dir(folder, QLatin1String("*.FCMacro *.py"));
    QStringList unfiltered = dir.entryList(); //all .fcmacro and .py files
    QString fileFilter = ui->LineEditFind->text(); //used to search by filename
    QString searchText = ui->LineEditFindInFiles->text(); //used to search in file content

    if (fileFilter.isEmpty() && searchText.isEmpty()){ //skip filtering if no filters
        return unfiltered;
    }
    QStringList filteredByFileName;
    if (fileFilter.isEmpty()){
        filteredByFileName = unfiltered; //skip the loop if no file filter
    } else {
        QRegularExpression regexFileName(fileFilter, QRegularExpression::CaseInsensitiveOption);
        bool isValidFileFilter = regexFileName.isValid(); //check here instead of inside the loop
        for (auto uf : unfiltered){
            if (isValidFileFilter){
                if (regexFileName.match(uf).hasMatch()) {
                    filteredByFileName.append(uf);
                }
            } else { //not valid, so do a simple text search
                if (uf.contains(fileFilter, Qt::CaseInsensitive)) {
                    filteredByFileName.append(uf);
                }
            }
        }
    }

    if (searchText.isEmpty()){ //skip reading file contents if no find in file filter
        return filteredByFileName;
    }

    QRegularExpression regexContent(searchText, QRegularExpression::CaseInsensitiveOption);
    bool isValidContentFilter = regexContent.isValid();
    QStringList filteredByContent;
    for (auto fn : filteredByFileName) {
        const QString &fileName = fn;
        QString filePath = dir.filePath(fileName);
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString fileContent = in.readAll();
            if (isValidContentFilter){
                if (regexContent.match(fileContent).hasMatch()) {
                    filteredByContent.append(fileName);
                }
            } else {
                if (fileContent.contains(searchText, Qt::CaseInsensitive)){
                    filteredByContent.append(fileName);
                }
            }
            file.close();
        }
    }
    return filteredByContent;
}

/**
 * Fills up the list with macro files found in the specified location
 * that have been filtered by both filename and by content
 */
void DlgMacroExecuteImp::fillUpList()
{
    QStringList filteredByContent = this->filterFiles(this->macroPath);
    ui->userMacroListBox->clear();
    for (auto fn : filteredByContent){
        auto item = new MacroItem(ui->userMacroListBox,false);
        item->setText(0, fn);
    }

    QString dirstr = QString::fromStdString(App::Application::getHomePath()) + QString::fromLatin1("Macro");
    filteredByContent = this->filterFiles(dirstr);

    ui->systemMacroListBox->clear();
    for (auto fn : filteredByContent) {
        auto item = new MacroItem(ui->systemMacroListBox,true);
        item->setText(0, fn);
    }
}


/**
 * Selects a macro file in the list view.
 */
void DlgMacroExecuteImp::onUserMacroListBoxCurrentItemChanged(QTreeWidgetItem* item)
{
    if (item) {
        ui->LineEditMacroName->setText(item->text(0));

        ui->executeButton->setEnabled(true);
        ui->deleteButton->setEnabled(true);
        ui->toolbarButton->setEnabled(true);
        ui->createButton->setEnabled(true);
        ui->editButton->setEnabled(true);
        ui->renameButton->setEnabled(true);
        ui->duplicateButton->setEnabled(true);
    }
    else {
        ui->executeButton->setEnabled(false);
        ui->deleteButton->setEnabled(false);
        ui->toolbarButton->setEnabled(false);
        ui->createButton->setEnabled(true);
        ui->editButton->setEnabled(false);
        ui->renameButton->setEnabled(false);
        ui->duplicateButton->setEnabled(false);
    }
}

void DlgMacroExecuteImp::onLineEditFindTextChanged(const QString &text){
    Q_UNUSED(text);
    this->fillUpList();
}

void DlgMacroExecuteImp::onLineEditFindInFilesTextChanged(const QString &text){
    Q_UNUSED(text);
    this->fillUpList();
}

void DlgMacroExecuteImp::onSystemMacroListBoxCurrentItemChanged(QTreeWidgetItem* item)
{
    if (item) {
        ui->LineEditMacroName->setText(item->text(0));

        ui->executeButton->setEnabled(true);
        ui->deleteButton->setEnabled(false);
        ui->toolbarButton->setEnabled(false);
        ui->createButton->setEnabled(false);
        ui->editButton->setEnabled(true); //look but don't touch
        ui->renameButton->setEnabled(false);
        ui->duplicateButton->setEnabled(false);
    }
    else {
        ui->executeButton->setEnabled(false);
        ui->deleteButton->setEnabled(false);
        ui->toolbarButton->setEnabled(false);
        ui->createButton->setEnabled(false);
        ui->editButton->setEnabled(false);
        ui->renameButton->setEnabled(false);
        ui->duplicateButton->setEnabled(false);
    }
}

void DlgMacroExecuteImp::onTabMacroWidgetCurrentChanged(int index)
{
    QTreeWidgetItem* item;

    if (index == 0) { //user-specific
        item = ui->userMacroListBox->currentItem();
        if (item) {
            ui->executeButton->setEnabled(true);
            ui->deleteButton->setEnabled(true);
            ui->toolbarButton->setEnabled(true);
            ui->createButton->setEnabled(true);
            ui->editButton->setEnabled(true);
            ui->renameButton->setEnabled(true);
            ui->duplicateButton->setEnabled(true);
        }
        else {
            ui->executeButton->setEnabled(false);
            ui->deleteButton->setEnabled(false);
            ui->toolbarButton->setEnabled(false);
            ui->createButton->setEnabled(true);
            ui->editButton->setEnabled(false);
            ui->renameButton->setEnabled(false);
            ui->duplicateButton->setEnabled(false);
        }
    }
    else { //index==1 system-wide
        item = ui->systemMacroListBox->currentItem();

        if (item) {
            ui->executeButton->setEnabled(true);
            ui->deleteButton->setEnabled(false);
            ui->toolbarButton->setEnabled(false);
            ui->createButton->setEnabled(false);
            ui->editButton->setEnabled(true); //but you can't save it
            ui->renameButton->setEnabled(false);
            ui->duplicateButton->setEnabled(false);
        }
        else {
            ui->executeButton->setEnabled(false);
            ui->deleteButton->setEnabled(false);
            ui->toolbarButton->setEnabled(false);
            ui->createButton->setEnabled(false);
            ui->editButton->setEnabled(false);
            ui->renameButton->setEnabled(false);
            ui->duplicateButton->setEnabled(false);
        }
    }

    if (item) {
        ui->LineEditMacroName->setText(item->text(0));
    }
    else {
        ui->LineEditMacroName->clear();
    }
}

/**
 * Executes the selected macro file.
 */
void DlgMacroExecuteImp::accept()
{
    QTreeWidgetItem* item;

    int index = ui->tabMacroWidget->currentIndex();
    if (index == 0) { //user-specific
        item = ui->userMacroListBox->currentItem();
    }
    else {
        //index == 1 system-wide
        item = ui->systemMacroListBox->currentItem();
    }
    if (!item)
        return;

    QDialog::accept();

    auto mitem = static_cast<MacroItem *>(item);

    QDir dir;

    if (!mitem->systemWide){
        dir =QDir(this->macroPath);
    }
    else {
        QString dirstr = QString::fromStdString(App::Application::getHomePath()) + QString::fromLatin1("Macro");
        dir = QDir(dirstr);
    }

    QFileInfo fi(dir, item->text(0));
    try {
        WaitCursor wc;
        PythonTracingLocker tracelock(watcher->getTrace());

        getMainWindow()->appendRecentMacro(fi.filePath());
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
void DlgMacroExecuteImp::onFileChooserFileNameChanged(const QString& fn)
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
void DlgMacroExecuteImp::onEditButtonClicked()
{
    QDir dir;
    QTreeWidgetItem* item = nullptr;

    int index = ui->tabMacroWidget->currentIndex();
    if (index == 0) { //user-specific
        item = ui->userMacroListBox->currentItem();
        dir.setPath(this->macroPath);
    }
    else {
        //index == 1 system-wide
        item = ui->systemMacroListBox->currentItem();
        dir.setPath(QString::fromStdString(App::Application::getHomePath()) + QString::fromLatin1("Macro"));
    }

    if (!item)
        return;

    auto mitem = static_cast<MacroItem *>(item);

    QString file = QString::fromLatin1("%1/%2").arg(dir.absolutePath(), item->text(0));
    auto editor = new PythonEditor();
    editor->setWindowIcon(Gui::BitmapFactory().iconFromTheme("applications-python"));
    auto edit = new PythonEditorView(editor, getMainWindow());
    edit->setDisplayName(PythonEditorView::FileName);
    edit->open(file);
    edit->resize(400, 300);
    getMainWindow()->addWindow(edit);
    getMainWindow()->appendRecentMacro(file);

    if (mitem->systemWide) {
        editor->setReadOnly(true);
        QString shownName;
        shownName = QString::fromLatin1("%1[*] - [%2]").arg(item->text(0), tr("Read-only"));
        edit->setWindowTitle(shownName);
    }
    close();
}

/** Creates a new macro file. */
void DlgMacroExecuteImp::onCreateButtonClicked()
{
    // query file name
    bool replaceSpaces = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro")->GetBool("ReplaceSpaces", true);
    App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro")->SetBool("ReplaceSpaces", replaceSpaces); //create parameter

    QString fn = QInputDialog::getText(this, tr("Macro file"), tr("Enter a file name, please:"),
        QLineEdit::Normal, QString(), nullptr, Qt::MSWindowsFixedSizeDialogHint);

    if(replaceSpaces){
        fn = fn.replace(QString::fromStdString(" "),QString::fromStdString("_"));
    }

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
            auto editor = new PythonEditor();
            editor->setWindowIcon(Gui::BitmapFactory().iconFromTheme("applications-python"));
            auto edit = new PythonEditorView(editor, getMainWindow());
            edit->open(fi.absoluteFilePath());
            getMainWindow()->appendRecentMacro(fi.absoluteFilePath());
            edit->setWindowTitle(QString::fromLatin1("%1[*]").arg(fn));
            edit->resize(400, 300);
            getMainWindow()->addWindow(edit);
            close();
        }
    }
}

/** Deletes the selected macro file from your harddisc. */
void DlgMacroExecuteImp::onDeleteButtonClicked()
{
    QTreeWidgetItem* item = ui->userMacroListBox->currentItem();
    if (!item)
        return;

    auto mitem = static_cast<MacroItem *>(item);

    if (mitem->systemWide) {
        QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Delete macro"),
            QObject::tr("Not allowed to delete system-wide macros"));
        return;
    }

    QString fn = item->text(0);
    auto ret = QMessageBox::question(this, tr("Delete macro"),
        tr("Do you really want to delete the macro '%1'?").arg( fn ),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        QDir dir(this->macroPath);
        dir.remove(fn);
        int index = ui->userMacroListBox->indexOfTopLevelItem(item);
        ui->userMacroListBox->takeTopLevelItem(index);
        delete item;
    }
}

/**
 * Walk user through process of adding macro to global custom toolbar
 * We create a custom customize dialog with instructions embedded
 * within the dialog itself for the user, and the buttons to push in red text
 * There are 2 dialogs we need to create: the macros dialog and the
 * toolbar dialog.
 */

void DlgMacroExecuteImp::onToolbarButtonClicked()
{
    /**
     * advise user of what we are doing, offer chance to cancel
     * unless user already said not to show this messagebox again
    **/

    bool showAgain = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro")->GetBool("ShowWalkthroughMessage", true);
    if (showAgain){
        QMessageBox msgBox;
        QAbstractButton* doNotShowAgainButton = msgBox.addButton(tr("Do not show again"), QMessageBox::YesRole);
        msgBox.setText(tr("Guided Walkthrough"));
        msgBox.setInformativeText(tr("This will guide you in setting up this macro in a custom \
global toolbar.  Instructions will be in red text inside the dialog.\n\
\n\
Note: your changes will be applied when you next switch workbenches\n"));
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Ok);
        int result = msgBox.exec();
        if (result == QMessageBox::Cancel){
            return;
        }
        if (msgBox.clickedButton() == doNotShowAgainButton){
            App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro")->SetBool("ShowWalkthroughMessage",false);
        }
    }

    QTreeWidgetItem* item = ui->userMacroListBox->currentItem();
    if (!item)
        return;

    QString fn = item->text(0);
    QString bareFileName = QFileInfo(fn).baseName(); //for use as default menu text (filename without extension)

    /** check if user already has custom toolbar, so we can tailor instructions accordingly **/
    bool hasCustomToolbar = true;
    if (App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Workbench/Global/Toolbar")->GetGroups().empty()) {
        hasCustomToolbar=false;
    }

    /** check if user already has this macro command created, if so skip dialog 1 **/
    bool hasMacroCommand = false;
    QString macroMenuText;
    CommandManager & cCmdMgr = Application::Instance->commandManager();
    std::vector<Command*> aCmds = cCmdMgr.getGroupCommands("Macros");
    for (const auto & aCmd : aCmds) {
        auto mc = dynamic_cast<MacroCommand*>(aCmd);
        if (mc && fn.compare(QLatin1String(mc->getScriptName())) == 0) {
            hasMacroCommand = true;
            macroMenuText = QString::fromLatin1(mc->getMenuText());
        }
    }

    QTabWidget* tabWidget = nullptr;

    if (!hasMacroCommand){
        /** first the custom macros page dialog **/
        Gui::Dialog::DlgCustomizeImp dlg(this);

        /** title is normally "Customize" **/
        dlg.setWindowTitle(tr("Walkthrough, dialog 1 of 2"));

        tabWidget = dlg.findChild<QTabWidget*>(QString::fromLatin1("Gui__Dialog__TabWidget"));
        if (!tabWidget) {
            std::cerr << "Toolbar walkthrough error: Unable to find tabwidget" << std::endl;
            return;
        }

        auto setupCustomMacrosPage = tabWidget->findChild<QWidget*>(QString::fromLatin1("Gui__Dialog__DlgCustomActions"));
        if (!setupCustomMacrosPage) {
            std::cerr << "Toolbar walkthrough error: Unable to find setupCustomMacrosPage" << std::endl;
            return;
        }
        tabWidget->setCurrentWidget(setupCustomMacrosPage);

        auto groupBox7 = setupCustomMacrosPage->findChild<QGroupBox*>(QString::fromLatin1("GroupBox7"));
        if (!groupBox7) {
            Base::Console().Warning("Toolbar walkthrough: Unable to find groupBox7\n");
            //just warn when not a fatal error
        } else {
            /** normally the groupbox title is "Setup Custom Macros", but we change it here **/
            groupBox7->setTitle(tr("Walkthrough instructions: Fill in missing fields (optional) then click Add, then Close"));
            groupBox7->setStyleSheet(QString::fromLatin1("QGroupBox::title {color:red}"));
        }

        auto buttonAddAction = setupCustomMacrosPage->findChild<QPushButton*>(QString::fromLatin1("buttonAddAction"));
        if (!buttonAddAction) {
            Base::Console().Warning("Toolbar walkthrough: Unable to find buttonAddAction\n");
        } else {
            buttonAddAction->setStyleSheet(QString::fromLatin1("color:red"));
        }

        auto macroListBox = setupCustomMacrosPage->findChild<QComboBox*>(QString::fromLatin1("actionMacros"));
        if (!macroListBox) {
            Base::Console().Warning("Toolbar walkthrough: Unable to find actionMacros combo box\n");
        } else {
            int macroIndex = macroListBox->findText(fn); //fn is the macro filename
            macroListBox->setCurrentIndex(macroIndex); //select it for the user so they don't have to
        }

        auto menuText = setupCustomMacrosPage->findChild<QLineEdit*>(QString::fromLatin1("actionMenu"));
        if (!menuText) {
            Base::Console().Warning("Toolbar walkthrough: Unable to find actionMenu menuText\n");
        } else {
            menuText->setText(bareFileName); //user can fill in other fields, e.g. tooltip
        }

        dlg.exec();
    }

    /** now for the toolbar selection dialog **/

    Gui::Dialog::DlgCustomizeImp dlg(this);

    if (hasMacroCommand){
        dlg.setWindowTitle(tr("Walkthrough, dialog 1 of 1"));
    } else {
        dlg.setWindowTitle(tr("Walkthrough, dialog 2 of 2"));
    }

    tabWidget = nullptr;
    tabWidget = dlg.findChild<QTabWidget*>(QString::fromLatin1("Gui__Dialog__TabWidget"));
    if (!tabWidget) {
        std::cerr << "Toolbar walkthrough: Unable to find tabWidget Gui__Dialog__TabWidget" << std::endl;
        return;
    }

    auto setupToolbarPage = tabWidget->findChild<DlgCustomToolbars*>(QString::fromLatin1("Gui__Dialog__DlgCustomToolbars"));
    if (!setupToolbarPage){
        std::cerr << "Toolbar walkthrough: Unable to find setupToolbarPage Gui__Dialog__DlgCustomToolbars" << std::endl;
        return;
    }

    tabWidget->setCurrentWidget(setupToolbarPage);
    auto moveActionRightButton = setupToolbarPage->findChild<QPushButton*>(QString::fromLatin1("moveActionRightButton"));
    if (!moveActionRightButton){
        Base::Console().Warning("Toolbar walkthrough: Unable to find moveActionRightButton\n");
    } else {
        moveActionRightButton->setStyleSheet(QString::fromLatin1("background-color: red"));
    }
    /** tailor instructions depending on whether user already has custom toolbar created
     * if not, they need to click New button to create one first
    **/

    QString instructions2 = tr("Walkthrough instructions: Click right arrow button (->), then Close.");
    auto workbenchBox = setupToolbarPage->findChild<QComboBox*>(QString::fromLatin1("workbenchBox"));
    if (!workbenchBox) {
        Base::Console().Warning("Toolbar walkthrough: Unable to find workbenchBox\n");
    }
    else {
        /** find the Global workbench and select it for the user **/

        int globalIdx = workbenchBox->findData(QString::fromLatin1("Global"));
        if (globalIdx != -1){
            workbenchBox->setCurrentIndex(globalIdx);
            QMetaObject::invokeMethod(setupToolbarPage, "on_workbenchBox_activated",
                Qt::DirectConnection,
                Q_ARG(int, globalIdx));
        } else {
            Base::Console().Warning("Toolbar walkthrough: Unable to find Global workbench\n");
        }

        if (!hasCustomToolbar){
            auto newButton = setupToolbarPage->findChild<QPushButton*>(QString::fromLatin1("newButton"));
            if (!newButton){
                Base::Console().Warning("Toolbar walkthrough: Unable to find newButton\n");
            } else {
                newButton->setStyleSheet(QString::fromLatin1("color:red"));
                instructions2 = tr("Walkthrough instructions: Click New, then right arrow (->) button, then Close.");
            }
        }
    }
    /** "label" normally says "Note: the changes become active the next time you load the appropriate workbench" **/

    auto label = setupToolbarPage->findChild<QLabel*>(QString::fromLatin1("label"));
    if (!label){
        Base::Console().Warning("Toolbar walkthrough: Unable to find label\n");
    } else {
        label->setText(instructions2);
        label->setStyleSheet(QString::fromLatin1("color:red"));
    }

    /** find Macros category and select it for the user **/
    auto categoryBox = setupToolbarPage->findChild<QComboBox*>(QString::fromLatin1("categoryBox"));
    if (!categoryBox){
        Base::Console().Warning("Toolbar walkthrough: Unable to find categoryBox\n");
    } else {
        int macrosIdx = categoryBox->findText(tr("Macros"));
        if (macrosIdx != -1){
            categoryBox->setCurrentIndex(macrosIdx);
            QMetaObject::invokeMethod(setupToolbarPage, "on_categoryBox_activated",
                Qt::DirectConnection,
                Q_ARG(int, macrosIdx));
        } else {
            Base::Console().Warning("Toolbar walkthrough: Unable to find Macros in categoryBox\n");
        }
    }

    /** expand custom toolbar items **/
    auto toolbarTreeWidget = setupToolbarPage->findChild<QTreeWidget*>(QString::fromLatin1("toolbarTreeWidget"));
    if (!toolbarTreeWidget) {
        Base::Console().Warning("Toolbar walkthrough: Unable to find toolbarTreeWidget\n");
    } else {
        toolbarTreeWidget->expandAll();
    }

    /** preselect macro command for user **/

    auto commandTreeWidget = setupToolbarPage->findChild<QTreeWidget*>(QString::fromLatin1("commandTreeWidget"));
    if (!commandTreeWidget) {
        Base::Console().Warning("Toolbar walkthrough: Unable to find commandTreeWidget\n");
    } else {
        if (!hasMacroCommand){  //will be the last in the list, the one just created
            commandTreeWidget->setCurrentItem(commandTreeWidget->topLevelItem(commandTreeWidget->topLevelItemCount()-1));
            commandTreeWidget->scrollToItem(commandTreeWidget->currentItem());
        } else {  //pre-select it for the user (will be the macro menu text)
            QList <QTreeWidgetItem*> items = commandTreeWidget->findItems(macroMenuText, Qt::MatchFixedString | Qt::MatchWrap,1);
            if (!items.empty()) {
                commandTreeWidget->setCurrentItem(items[0]);
                commandTreeWidget->scrollToItem(commandTreeWidget->currentItem());
            }
        }
    }
    dlg.exec();
}



/**
 * renames the selected macro
 */
void DlgMacroExecuteImp::onRenameButtonClicked()
{
    QDir dir;
    QTreeWidgetItem* item = nullptr;

    int index = ui->tabMacroWidget->currentIndex();
    if (index == 0) { //user-specific
        item = ui->userMacroListBox->currentItem();
        dir.setPath(this->macroPath);
    }

    if (!item)
        return;

    bool replaceSpaces = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro")->GetBool("ReplaceSpaces", true);
    App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro")->SetBool("ReplaceSpaces", replaceSpaces); //create parameter

    QString oldName = item->text(0);
    QFileInfo oldfi(dir, oldName);
    QFile oldfile(oldfi.absoluteFilePath());

    // query new name
    QString fn = QInputDialog::getText(this, tr("Renaming Macro File"),
        tr("Enter new name:"), QLineEdit::Normal, oldName, nullptr, Qt::MSWindowsFixedSizeDialogHint);

    if(replaceSpaces){
        fn = fn.replace(QString::fromStdString(" "),QString::fromStdString("_"));
    }

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
            ui->LineEditMacroName->setText(fn);
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
void DlgMacroExecuteImp::onDuplicateButtonClicked()
{
    QDir dir;
    QTreeWidgetItem* item = nullptr;

    //When duplicating a macro we can either begin trying to find a unique name with @001 or begin with the current @NNN if applicable

    bool from001 = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro")->GetBool("DuplicateFrom001", false);
    App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro")->SetBool("DuplicateFrom001", from001); //create parameter

    //A user may wish to add a note to end of the filename when duplicating
    //example: mymacro@005.fix_bug_in_dialog.FCMacro
    //and then when duplicating to have the extra note removed so the suggested new name is:
    //mymacro@006.FCMacro instead of mymacro@006.fix_bug_in_dialog.FCMacro since the new duplicate will be given a new note

    bool ignoreExtra = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro")->GetBool("DuplicateIgnoreExtraNote", false);
    App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro")->SetBool("DuplicateIgnoreExtraNote", ignoreExtra); //create parameter

    //when creating a note it will be convenient to convert spaces to underscores if the user desires this behavior

    bool replaceSpaces = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro")->GetBool("ReplaceSpaces", true);
    App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro")->SetBool("ReplaceSpaces", replaceSpaces); //create parameter

    int index = ui->tabMacroWidget->currentIndex();
    if (index == 0) { //user-specific
        item = ui->userMacroListBox->currentItem();
        dir.setPath(this->macroPath);
    }

    if (!item){
        return;
    }

    QString oldName = item->text(0);
    QFileInfo oldfi(dir, oldName);
    QFile oldfile(oldfi.absoluteFilePath());
    QString completeSuffix = oldfi.completeSuffix(); //everything after the first "."
    QString extraNote = completeSuffix.left(completeSuffix.size()-oldfi.suffix().size());
    QString baseName = oldfi.baseName(); //everything before first "."
    QString neutralSymbol = QString::fromStdString("@");
    QString last3 = baseName.right(3);
    bool ok = true; //was conversion to int successful?
    int nLast3 = last3.toInt(&ok);
    last3 = QString::fromStdString("001"); //increment beginning with 001 unless from001 = false
    if (ok ){
        //last3 were all digits, so we strip them from the base name
        if (baseName.size()>3){ //if <= 3 leave be (e.g. 2.py becomes 2@001.py)
            if(!from001){
                last3 = baseName.right(3); //use these instead of 001
            }
            baseName = baseName.left(baseName.size()-3); //strip digits
            if (baseName.endsWith(neutralSymbol)){
                baseName = baseName.left(baseName.size()-1); //trim the "@", will be added back later
            }
        }
    }
    //at this point baseName = the base name without any digits, e.g. "MyMacro"
    //neutralSymbol = "@"
    //last3 is a string representing 3 digits, always "001"
    //unless from001 = false, in which case we begin with previous numbers
    //completeSuffix = FCMacro or py or FCMacro.py or else suffix will become FCMacro below
    //if ignoreExtra any extra notes added between @NN. and .FCMacro will be ignored
    //when suggesting a new filename

    if(ignoreExtra && !extraNote.isEmpty()){
        nLast3++;
        last3 = QString::number(nLast3);
        while (last3.size()<3){
            last3.prepend(QString::fromStdString("0")); //pad 0's if needed
        }
    }


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

    if(ignoreExtra && !extraNote.isEmpty()){
        oldNameDigitized = oldNameDigitized.remove(extraNote);
    }

    // give user a chance to pick a different name from digitized name suggested
    QString fn = QInputDialog::getText(this, tr("Duplicate Macro"),
        tr("Enter new name:"), QLineEdit::Normal, oldNameDigitized, 
        nullptr, Qt::MSWindowsFixedSizeDialogHint);
    if (replaceSpaces){
        fn = fn.replace(QString::fromStdString(" "),QString::fromStdString("_"));
    }
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
void DlgMacroExecuteImp::onAddonsButtonClicked()
{
    CommandManager& rMgr=Application::Instance->commandManager();
    rMgr.runCommandByName("Std_AddonMgr");
    this->fillUpList();
}

#include "moc_DlgMacroExecuteImp.cpp"
