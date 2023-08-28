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
# include <QDialogButtonBox>
# include <QDir>
# include <QFileDialog>
# include <QFileInfo>
# include <QHeaderView>
# include <QImageReader>
# include <QKeySequence>
# include <QLineEdit>
# include <QMessageBox>
#endif

#include "DlgActionsImp.h"
#include "ui_DlgActions.h"
#include "Action.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "ShortcutManager.h"
#include "ui_DlgChooseIcon.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgCustomActionsImp */

/**
 *  Constructs a DlgCustomActionsImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgCustomActionsImp::DlgCustomActionsImp( QWidget* parent )
  : CustomizeActionPage(parent)
  , ui(new Ui_DlgCustomActions)
{
    ui->setupUi(this);
    setupConnections();

    // search for all macros
    std::string cMacroPath = App::GetApplication().
        GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro")
        ->GetASCII("MacroPath",App::Application::getUserMacroDir().c_str());

    QDir d(QString::fromUtf8(cMacroPath.c_str()), QLatin1String("*.FCMacro *.py"));
    for (unsigned int i=0; i<d.count(); i++ )
        ui->actionMacros->insertItem(0,d[i],QVariant(false));

    QString systemMacroDirStr = QString::fromStdString(App::Application::getHomePath()) + QString::fromLatin1("Macro");
    d = QDir(systemMacroDirStr, QLatin1String("*.FCMacro *.py"));
    if (d.exists()) {
        for (unsigned int i=0; i<d.count(); i++ ) {
            ui->actionMacros->insertItem(0,d[i],QVariant(true));
        }
    }

    QStringList labels; labels << tr("Icons") << tr("Macros");
    ui->actionListWidget->setHeaderLabels(labels);
    ui->actionListWidget->header()->hide();
    ui->actionListWidget->setIconSize(QSize(32, 32));
    ui->actionListWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    showActions();
}

/** Destroys the object and frees any allocated resources */
DlgCustomActionsImp::~DlgCustomActionsImp()
{
    if (bChanged)
        MacroCommand::save();
}

void DlgCustomActionsImp::setupConnections()
{
    connect(ui->actionListWidget, &QTreeWidget::itemActivated,
            this, &DlgCustomActionsImp::onActionListWidgetItemActivated);
    connect(ui->buttonChoosePixmap, &QToolButton::clicked,
            this, &DlgCustomActionsImp::onButtonChoosePixmapClicked);
    connect(ui->buttonAddAction, &QPushButton::clicked,
            this, &DlgCustomActionsImp::onButtonAddActionClicked);
    connect(ui->buttonRemoveAction, &QPushButton::clicked,
            this, &DlgCustomActionsImp::onButtonRemoveActionClicked);
    connect(ui->buttonReplaceAction, &QPushButton::clicked,
            this, &DlgCustomActionsImp::onButtonReplaceActionClicked);
}

bool DlgCustomActionsImp::event(QEvent* e)
{
    bool ok = QWidget::event(e);

    if (e->type() == QEvent::ParentChange || e->type() == QEvent::ParentAboutToChange)
    {
        QWidget* topLevel = this->parentWidget();
        while (topLevel && !topLevel->inherits("QDialog"))
            topLevel = topLevel->parentWidget();
        if ( topLevel )
        {
            int index = topLevel->metaObject()->indexOfSignal( QMetaObject::normalizedSignature("addMacroAction(const QByteArray&)") );
            if ( index >= 0 ) {
                if ( e->type() == QEvent::ParentChange ) {
                    connect(this, SIGNAL(addMacroAction( const QByteArray& )),
                            topLevel, SIGNAL(addMacroAction( const QByteArray& )));
                    connect(this, SIGNAL(removeMacroAction( const QByteArray& )),
                            topLevel, SIGNAL(removeMacroAction( const QByteArray& )));
                    connect(this, SIGNAL(modifyMacroAction( const QByteArray& )),
                            topLevel, SIGNAL(modifyMacroAction( const QByteArray& )));
                } else {
                    disconnect(this, SIGNAL(addMacroAction( const QByteArray& )),
                               topLevel, SIGNAL(addMacroAction( const QByteArray& )));
                    disconnect(this, SIGNAL(removeMacroAction( const QByteArray& )),
                               topLevel, SIGNAL(removeMacroAction( const QByteArray& )));
                    disconnect(this, SIGNAL(modifyMacroAction( const QByteArray& )),
                               topLevel, SIGNAL(modifyMacroAction( const QByteArray& )));
                }
            }
        }
    }

    return ok;
}

void DlgCustomActionsImp::onAddMacroAction(const QByteArray&)
{
    bChanged = true;
}

void DlgCustomActionsImp::onRemoveMacroAction(const QByteArray &name)
{
    bChanged = true;
    ShortcutManager::instance()->reset(name.constData());
}

void DlgCustomActionsImp::onModifyMacroAction(const QByteArray&)
{
    bChanged = true;
}

void DlgCustomActionsImp::showActions()
{
    CommandManager& rclMan = Application::Instance->commandManager();
    std::vector<Command*> aclCurMacros = rclMan.getGroupCommands("Macros");
    for (const auto & aclCurMacro : aclCurMacros)
    {
        auto item = new QTreeWidgetItem(ui->actionListWidget);
        QByteArray actionName = aclCurMacro->getName();
        item->setData(1, Qt::UserRole, actionName);
        item->setText(1, QString::fromUtf8(aclCurMacro->getMenuText()));
        item->setSizeHint(0, QSize(32, 32));
        if ( aclCurMacro->getPixmap() )
            item->setIcon(0, BitmapFactory().pixmap(aclCurMacro->getPixmap()));
    }
}

void DlgCustomActionsImp::onActionListWidgetItemActivated(QTreeWidgetItem *item)
{
    if (!item)
        return; // no valid item

    // search for the command in the manager and if necessary in the temporary created ones
    QByteArray actionName = item->data(1, Qt::UserRole).toByteArray();
    CommandManager& rclMan = Application::Instance->commandManager();
    Command* pCmd = rclMan.getCommandByName(actionName.constData());
    auto pScript = dynamic_cast<MacroCommand*>(pCmd);

    // if valid command
    if ( pScript )
    {
        bool bFound = false;
        QString scriptName = QString::fromUtf8(pScript->getScriptName());
        for (int i = 0; i<ui->actionMacros->count(); i++)
        {
            if (ui->actionMacros->itemText(i).startsWith(scriptName, Qt::CaseSensitive))
            {
                bFound = true;
                ui->actionMacros->setCurrentIndex(i);
                break;
            }
        }

        if (!bFound)
        {
            QMessageBox::critical(this, tr("Macro not found"),
                    tr("Sorry, couldn't find macro file '%1'.").arg(scriptName));
        }

        // fill up labels with the command's data
        ui->actionWhatsThis -> setText(QString::fromUtf8(pScript->getWhatsThis()));
        ui->actionMenu      -> setText(QString::fromUtf8(pScript->getMenuText()));
        ui->actionToolTip   -> setText(QString::fromUtf8(pScript->getToolTipText()));
        ui->actionStatus    -> setText(QString::fromUtf8(pScript->getStatusTip()));
        ui->actionAccel     -> setText(ShortcutManager::instance()->getShortcut(
                    actionName.constData(), pScript->getAccel()));
        ui->pixmapLabel->clear();
        m_sPixmap.clear();
        const char* name = pScript->getPixmap();
        if (name && std::strlen(name) > 2)
        {
            QPixmap p = Gui::BitmapFactory().pixmap(pScript->getPixmap());
            ui->pixmapLabel->setPixmap(p);
            m_sPixmap = QString::fromUtf8(name); // can also be a path
        }
    }
}

void DlgCustomActionsImp::onButtonAddActionClicked()
{
    if (ui->actionMacros-> currentText().isEmpty())
    {
        QMessageBox::warning(this, tr("Empty macro"),tr("Please specify the macro first."));
        return;
    }

    if (ui->actionMenu->text().isEmpty())
    {
        QMessageBox::warning(this, tr("Empty text"),tr("Please specify the menu text first."));
        return;
    }

    // search for the command in the manager
    CommandManager& rclMan = Application::Instance->commandManager();
    QByteArray actionName = QString::fromStdString(rclMan.newMacroName()).toLatin1();
    auto macro = new MacroCommand(actionName, ui->actionMacros->itemData(ui->actionMacros->currentIndex()).toBool());
    rclMan.addCommand( macro );

    // add new action
    auto item = new QTreeWidgetItem(ui->actionListWidget);
    item->setData(1, Qt::UserRole, actionName);
    item->setText(1, ui->actionMenu->text());
    item->setSizeHint(0, QSize(32, 32));
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
    item->setIcon(0, ui->pixmapLabel->pixmap(Qt::ReturnByValue));
#else
    if (ui->pixmapLabel->pixmap())
        item->setIcon(0, *ui->pixmapLabel->pixmap());
#endif

    // Convert input text into utf8
    if (!ui->actionWhatsThis->text().isEmpty())
        macro->setWhatsThis(ui->actionWhatsThis->text().toUtf8());
    ui->actionWhatsThis->clear();

    if (!ui->actionMacros-> currentText().isEmpty())
        macro->setScriptName(ui->actionMacros->currentText().toUtf8());

    if (!ui->actionMenu->text().isEmpty())
        macro->setMenuText(ui->actionMenu->text().toUtf8());
    ui->actionMenu->clear();

    if (!ui->actionToolTip->text().isEmpty())
        macro->setToolTipText(ui->actionToolTip->text().toUtf8());
    ui->actionToolTip->clear();

    if (!ui->actionStatus->text().isEmpty())
        macro->setStatusTip(ui->actionStatus->text().toUtf8());
    ui->actionStatus->clear();

    if (!m_sPixmap.isEmpty())
        macro->setPixmap(m_sPixmap.toLatin1());
    ui->pixmapLabel->clear();
    m_sPixmap.clear();

    if (!ui->actionAccel->text().isEmpty()) {
        ShortcutManager::instance()->setShortcut(
                actionName.constData(), ui->actionAccel->text().toLatin1().constData());
    }
    ui->actionAccel->clear();

    // emit signal to notify the container widget
    Q_EMIT addMacroAction(actionName);
}

void DlgCustomActionsImp::onButtonReplaceActionClicked()
{
    QTreeWidgetItem* item = ui->actionListWidget->currentItem();
    if (!item)
    {
        QMessageBox::warning(this, tr("No item selected"),tr("Please select a macro item first."));
        return;
    }

    if (ui->actionMenu->text().isEmpty())
    {
        QMessageBox::warning(this, tr("Empty text"),tr("Please specify the menu text first."));
        return;
    }

    // search for the command in the manager
    QByteArray actionName = item->data(1, Qt::UserRole).toByteArray();
    item->setText(1, ui->actionMenu->text());
    CommandManager& rclMan = Application::Instance->commandManager();
    Command* pCmd = rclMan.getCommandByName(actionName.constData());
    auto macro = dynamic_cast<MacroCommand*>(pCmd);
    if (!macro)
        return;

    if (!ui->actionWhatsThis->text().isEmpty())
        macro->setWhatsThis(ui->actionWhatsThis->text().toUtf8());
    ui->actionWhatsThis->clear();

    if (!ui->actionMacros-> currentText().isEmpty())
        macro->setScriptName(ui->actionMacros->currentText().toUtf8());

    if (!ui->actionMenu->text().isEmpty())
        macro->setMenuText(ui->actionMenu->text().toUtf8());
    ui->actionMenu->clear();

    if (!ui->actionToolTip->text().isEmpty())
        macro->setToolTipText(ui->actionToolTip->text().toUtf8());
    ui->actionToolTip->clear();

    if (!ui->actionStatus->text().isEmpty())
        macro->setStatusTip(ui->actionStatus->text().toUtf8());
    ui->actionStatus->clear();

    if (!m_sPixmap.isEmpty())
        macro->setPixmap(m_sPixmap.toLatin1());
    ui->pixmapLabel->clear();
    m_sPixmap.clear();

    if (!ui->actionAccel->text().isEmpty()) {
        macro->setAccel(ui->actionAccel->text().toLatin1());
    }
    ui->actionAccel->clear();

    // check whether the macro is already in use
    Action* action = macro->getAction();
    if (action)
    {
        // does all the text related stuff
        action->setText(QString::fromUtf8(macro->getMenuText()));
        action->setToolTip(QString::fromUtf8(macro->getToolTipText()));
        action->setWhatsThis(QString::fromUtf8(macro->getWhatsThis()));
        action->setStatusTip(QString::fromUtf8(macro->getStatusTip()));
        if (macro->getPixmap())
            action->setIcon(Gui::BitmapFactory().pixmap(macro->getPixmap()));
        action->setShortcut(ShortcutManager::instance()->getShortcut(
                    actionName.constData(), macro->getAccel()));
    }

    // emit signal to notify the container widget
    Q_EMIT modifyMacroAction(actionName);

    // call this at the end because it internally invokes the highlight method
    if (macro->getPixmap())
        item->setIcon(0, Gui::BitmapFactory().pixmap(macro->getPixmap()));
}

void DlgCustomActionsImp::onButtonRemoveActionClicked()
{
    // remove item from list view
    QTreeWidgetItem* item = ui->actionListWidget->currentItem();
    if (!item)
        return;
    int current = ui->actionListWidget->indexOfTopLevelItem(item);
    ui->actionListWidget->takeTopLevelItem(current);
    QByteArray actionName = item->data(1, Qt::UserRole).toByteArray();
    delete item;

    // if the command is registered in the manager just remove it
    CommandManager& rclMan = Application::Instance->commandManager();
    std::vector<Command*> aclCurMacros = rclMan.getGroupCommands("Macros");
    for (auto & aclCurMacro : aclCurMacros)
    {
        if (actionName == aclCurMacro->getName())
        {
            // emit signal to notify the container widget
            Q_EMIT removeMacroAction(actionName);
            // remove from manager and delete it immediately
            rclMan.removeCommand(aclCurMacro);
            break;
        }
    }
}

IconDialog::IconDialog(QWidget* parent)
  : QDialog(parent), ui(new Ui_DlgChooseIcon)
{
    ui->setupUi(this);
    ui->listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // signals and slots connections
    connect(ui->listWidget, &QListWidget::itemClicked, this, &IconDialog::accept);
    connect(ui->addButton, &QPushButton::clicked, this, &IconDialog::onAddIconPath);

    QListWidgetItem* item;
    QStringList names = BitmapFactory().findIconFiles();
    for (const auto & name : names) {
        item = new QListWidgetItem(ui->listWidget);
        //item->setIcon(QIcon(*it));
        item->setIcon(QIcon(BitmapFactory().pixmap((const char*)name.toUtf8())));
        item->setText(QFileInfo(name).baseName());
        item->setToolTip(name);
    }
}

IconDialog::~IconDialog()
{
    delete ui;
}

QListWidgetItem* IconDialog::currentItem() const
{
    return ui->listWidget->currentItem();
}

void IconDialog::resizeEvent(QResizeEvent*)
{
    ui->listWidget->setFlow(QListView::LeftToRight);
}

void IconDialog::onAddIconPath()
{
    // Add the user defined paths
    Base::Reference<ParameterGrp> group = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Bitmaps");
    std::vector<std::string> paths = group->GetASCIIs("CustomPath");
    QStringList pathList;
    for (const auto & path : paths)
        pathList << QString::fromUtf8(path.c_str());

    IconFolders dlg(pathList, this);
    dlg.setWindowTitle(tr("Icon folders"));
    if (dlg.exec()) {
        QStringList paths = dlg.getPaths();

        // Write to user config
        group->Clear();
        int index=0;
        for (QStringList::iterator it = paths.begin(); it != paths.end(); ++it, ++index) {
            std::stringstream str;
            str << "CustomPath" << index;
            group->SetASCII(str.str().c_str(), (const char*)it->toUtf8());
        }

        QStringList search = BitmapFactory().getPaths();
        for (auto & it : search) {
            it = QDir::toNativeSeparators(it);
        }
        for (const auto & path : paths) {
            if (search.indexOf(path) < 0) {
                QStringList filters;
                QList<QByteArray> formats = QImageReader::supportedImageFormats();
                for (const auto & format : formats)
                    filters << QString::fromLatin1("*.%1").arg(QString::fromLatin1(format).toLower());
                QDir d(path);
                d.setNameFilters(filters);
                QFileInfoList fi = d.entryInfoList();
                for (const auto & jt : fi) {
                    QString file = jt.absoluteFilePath();
                    auto item = new QListWidgetItem(ui->listWidget);
                    item->setIcon(QIcon(file));
                    item->setText(jt.baseName());
                    item->setToolTip(file);
                }

                BitmapFactory().addPath(path);
            }
        }
    }
}

void DlgCustomActionsImp::onButtonChoosePixmapClicked()
{
    // create a dialog showing all pixmaps
    Gui::Dialog::IconDialog dlg(this);
    dlg.setModal(true);
    dlg.exec();

    ui->pixmapLabel->clear();
    m_sPixmap.clear();
    if (dlg.result() == QDialog::Accepted) {
        QListWidgetItem* item = dlg.currentItem();
        if (item) {
            m_sPixmap = item->text();
            ui->pixmapLabel->setPixmap(item->icon().pixmap(QSize(32,32)));
        }
    }
}

void DlgCustomActionsImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        ui->actionListWidget->clear();
        showActions();
        ui->actionAccel->setText(qApp->translate("Gui::AccelLineEdit", "none"));
    }
    QWidget::changeEvent(e);
}

IconFolders::IconFolders(const QStringList& paths, QWidget* parent)
  : QDialog(parent), restart(false), maxLines(10)
{
    resize(600,400);
    auto buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &IconFolders::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &IconFolders::reject);

    gridLayout = new QGridLayout();
    auto mainLayout = new QGridLayout(this);
    mainLayout->addLayout(gridLayout, 0, 0, 1, 1);

    auto verticalSpacer = new QSpacerItem(20, 108, QSizePolicy::Minimum, QSizePolicy::Expanding);
    mainLayout->addItem(verticalSpacer, 1, 0, 1, 1);
    mainLayout->addWidget(buttonBox, 2, 0, 1, 1);

    // Add the user defined paths
    int numPaths = static_cast<int>(paths.size());
    int maxRow = this->maxLines;
    for (int row=0; row<maxRow; row++) {
        auto edit = new QLineEdit(this);
        edit->setReadOnly(true);
        gridLayout->addWidget(edit, row, 0, 1, 1);
        auto removeButton = new QPushButton(this);
        removeButton->setIcon(BitmapFactory().iconFromTheme("list-remove"));
        gridLayout->addWidget(removeButton, row, 1, 1, 1);

        if (row < numPaths) {
            edit->setText(paths[row]);
        }
        else {
            edit->hide();
            removeButton->hide();
        }

        buttonMap.append(qMakePair(edit, removeButton));
        connect(removeButton, &QPushButton::clicked, this, &IconFolders::removeFolder);
    }

    textLabel = new QLabel(this);
    textLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    textLabel->setText(tr("Add or remove custom icon folders"));
    addButton = new QPushButton(this);
    addButton->setIcon(BitmapFactory().iconFromTheme("list-add"));
    gridLayout->addWidget(textLabel, maxRow, 0, 1, 1);
    gridLayout->addWidget(addButton, maxRow, 1, 1, 1);

    connect(addButton, &QPushButton::clicked, this, &IconFolders::addFolder);
    if (numPaths >= this->maxLines)
        addButton->setDisabled(true);
}

IconFolders::~IconFolders() = default;

void IconFolders::addFolder()
{
    int countHidden = -1;
    QStringList paths;
    for (const auto & it : buttonMap) {
        if (it.first->isHidden()) {
            countHidden++;
            if (countHidden == 0) {
                QString dir = QFileDialog::getExistingDirectory(this, IconDialog::tr("Add icon folder"), QString());
                if (!dir.isEmpty() && paths.indexOf(dir) < 0) {
                    QLineEdit* edit = it.first;
                    edit->setVisible(true);
                    edit->setText(dir);
                    QPushButton* removeButton = it.second;
                    removeButton->setVisible(true);
                }
            }
        }
        else {
            paths << QDir::toNativeSeparators(it.first->text());
        }
    }

    if (countHidden <= 0) {
        addButton->setDisabled(true);
    }
}

void IconFolders::removeFolder()
{
    if (!restart) {
        restart = true;
        QMessageBox::information(this, tr("Remove folder"),
            tr("Removing a folder only takes effect after an application restart."));
    }

    addButton->setEnabled(true);
    auto remove = static_cast<QPushButton*>(sender());
    QLineEdit* edit = nullptr;
    for (const auto & it : buttonMap) {
        if (it.second == remove) {
            edit = it.first;
        }
        else if (edit) {
            // move up the text of the line edits
            edit->setText(it.first->text());

            if (it.first->isVisible()) {
                edit = it.first;
                remove = it.second;
            }
            else {
                edit->hide();
                remove->hide();
                break;
            }
        }
    }
}

QStringList IconFolders::getPaths() const
{
    QStringList paths;
    for (const auto & it : buttonMap) {
        if (!it.first->isHidden()) {
            paths << QDir::toNativeSeparators(it.first->text());
        }
        else {
            break;
        }
    }
    return paths;
}

#include "moc_DlgActionsImp.cpp"
