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
# include <QDir>
# include <QFileDialog>
# include <QFileInfo>
# include <QHeaderView>
# include <QImageReader>
# include <QMessageBox>
# include <QTextStream>
#endif

#include "DlgActionsImp.h"
#include "Action.h"
#include "Application.h"
#include "Command.h"
#include "BitmapFactory.h"
#include "Widgets.h"
#include "ui_DlgChooseIcon.h"

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgCustomActionsImp */

/**
 *  Constructs a DlgCustomActionsImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
DlgCustomActionsImp::DlgCustomActionsImp( QWidget* parent )
  : CustomizeActionPage(parent), bShown( false )
{
    this->setupUi(this);
    // search for all macros
    std::string cMacroPath = App::GetApplication().
        GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro")
        ->GetASCII("MacroPath",App::Application::getUserAppDataDir().c_str());

    QDir d(QString::fromUtf8(cMacroPath.c_str()), QLatin1String("*.FCMacro"));
    actionMacros->insertItems(0, d.entryList());

    QStringList labels; labels << tr("Icons") << tr("Macros");
    actionListWidget->setHeaderLabels(labels);
    actionListWidget->header()->hide();
    actionListWidget->setIconSize(QSize(32, 32));
    actionListWidget->header()->setResizeMode(0, QHeaderView::ResizeToContents);

    showActions();
}

/** Destroys the object and frees any allocated resources */
DlgCustomActionsImp::~DlgCustomActionsImp()
{
}

/** 
 * Displays this page. If no macros were found a message box
 * appears.
 */
void DlgCustomActionsImp::showEvent(QShowEvent* e)
{
    QWidget::showEvent(e);
    if (actionMacros->count() == 0 && bShown == false)
    {
        bShown = true;
        QMessageBox::warning(this, tr("No macro"),tr("No macros found."));
    }
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
  // does nothing
}

void DlgCustomActionsImp::onRemoveMacroAction(const QByteArray&)
{
  // does nothing
}

void DlgCustomActionsImp::onModifyMacroAction(const QByteArray&)
{
  // does nothing
}

void DlgCustomActionsImp::showActions()
{
    CommandManager& rclMan = Application::Instance->commandManager();
    std::vector<Command*> aclCurMacros = rclMan.getGroupCommands("Macros");
    for (std::vector<Command*>::iterator it = aclCurMacros.begin(); it != aclCurMacros.end(); ++it)
    {
        QTreeWidgetItem* item = new QTreeWidgetItem(actionListWidget);
        QByteArray actionName = (*it)->getName();
        item->setData(1, Qt::UserRole, actionName);
        item->setText(1, QString::fromUtf8((*it)->getMenuText()));
        item->setSizeHint(0, QSize(32, 32));
        if ( (*it)->getPixmap() )
            item->setIcon(0, BitmapFactory().pixmap((*it)->getPixmap()));
    }
}

void DlgCustomActionsImp::on_actionListWidget_itemActivated(QTreeWidgetItem *item)
{
    if (!item) 
        return; // no valid item

    // search for the command in the manager and if necessary in the temporary created ones
    QByteArray actionName = item->data(1, Qt::UserRole).toByteArray();
    CommandManager& rclMan = Application::Instance->commandManager();
    Command* pCmd = rclMan.getCommandByName(actionName.constData());
    MacroCommand* pScript = dynamic_cast<MacroCommand*>(pCmd);

    // if valid command
    if ( pScript )
    {
        bool bFound = false;
        QString scriptName = QString::fromUtf8(pScript->getScriptName());
        for (int i = 0; i<actionMacros->count(); i++)
        {
            if (actionMacros->itemText(i).startsWith(scriptName, Qt::CaseSensitive))
            {
                bFound = true;
                actionMacros->setCurrentIndex(i);
                break;
            }
        }

        if (!bFound)
        {
            QMessageBox::critical(this, tr("Macro not found"), 
                    tr("Sorry, couldn't find macro file '%1'.").arg(scriptName));
        }

        // fill up labels with the command's data
        actionWhatsThis -> setText(QString::fromUtf8(pScript->getWhatsThis()));
        actionMenu      -> setText(QString::fromUtf8(pScript->getMenuText()));
        actionToolTip   -> setText(QString::fromUtf8(pScript->getToolTipText()));
        actionStatus    -> setText(QString::fromUtf8(pScript->getStatusTip()));
        actionAccel     -> setText(QString::fromAscii(pScript->getAccel()));
        pixmapLabel->clear();
        m_sPixmap = QString::null;
        const char* name = pScript->getPixmap();
        if (name && std::strlen(name) > 2)
        {
            QPixmap p = Gui::BitmapFactory().pixmap(pScript->getPixmap());
            pixmapLabel->setPixmap(p);
            m_sPixmap = QString::fromUtf8(name); // can also be a path
        }
    }
}

void DlgCustomActionsImp::on_buttonAddAction_clicked()
{
    if (actionMacros-> currentText().isEmpty())
    {
        QMessageBox::warning(this, tr("Empty macro"),tr("Please specify the macro first."));
        return;
    }

    if (actionMenu->text().isEmpty())
    {
        QMessageBox::warning(this, tr("Empty text"),tr("Please specify the menu text first."));
        return;
    }

    // search for the command in the manager
    QByteArray actionName = newActionName().toAscii();
    CommandManager& rclMan = Application::Instance->commandManager();
    MacroCommand* macro = new MacroCommand(actionName);
    rclMan.addCommand( macro );

    // add new action
    QTreeWidgetItem* item = new QTreeWidgetItem(actionListWidget);
    item->setData(1, Qt::UserRole, actionName);
    item->setText(1, actionMenu->text());
    item->setSizeHint(0, QSize(32, 32));
    if (pixmapLabel->pixmap())
        item->setIcon(0, *pixmapLabel->pixmap());

    // Convert input text into utf8
    if (!actionWhatsThis->text().isEmpty())
        macro->setWhatsThis(actionWhatsThis->text().toUtf8());
    actionWhatsThis->clear();
  
    if (!actionMacros-> currentText().isEmpty())
        macro->setScriptName(actionMacros->currentText().toUtf8());
  
    if (!actionMenu->text().isEmpty())
        macro->setMenuText(actionMenu->text().toUtf8());
    actionMenu->clear();

    if (!actionToolTip->text().isEmpty())
        macro->setToolTipText(actionToolTip->text().toUtf8());
    actionToolTip->clear();

    if (!actionStatus->text().isEmpty())
        macro->setStatusTip(actionStatus->text().toUtf8());
    actionStatus->clear();

    if (!m_sPixmap.isEmpty())
        macro->setPixmap(m_sPixmap.toAscii());
    pixmapLabel->clear();
    m_sPixmap = QString::null;

    if (!actionAccel->text().isEmpty()) {
      macro->setAccel(actionAccel->text().toAscii());
    }
    actionAccel->clear();

    // emit signal to notify the container widget
    addMacroAction(actionName);
}

void DlgCustomActionsImp::on_buttonReplaceAction_clicked()
{
    QTreeWidgetItem* item = actionListWidget->currentItem();
    if (!item)
    {
        QMessageBox::warning(this, tr("No item selected"),tr("Please select a macro item first."));
        return;
    }

    if (actionMenu->text().isEmpty())
    {
        QMessageBox::warning(this, tr("Empty text"),tr("Please specify the menu text first."));
        return;
    }

    // search for the command in the manager
    QByteArray actionName = item->data(1, Qt::UserRole).toByteArray();
    item->setText(1, actionMenu->text());
    CommandManager& rclMan = Application::Instance->commandManager();
    Command* pCmd = rclMan.getCommandByName(actionName.constData());
    MacroCommand* macro = dynamic_cast<MacroCommand*>(pCmd);

    if (!actionWhatsThis->text().isEmpty())
        macro->setWhatsThis(actionWhatsThis->text().toUtf8());
    actionWhatsThis->clear();
  
    if (!actionMacros-> currentText().isEmpty())
        macro->setScriptName(actionMacros->currentText().toUtf8());
  
    if (!actionMenu->text().isEmpty())
        macro->setMenuText(actionMenu->text().toUtf8());
    actionMenu->clear();

    if (!actionToolTip->text().isEmpty())
        macro->setToolTipText(actionToolTip->text().toUtf8());
    actionToolTip->clear();

    if (!actionStatus->text().isEmpty())
        macro->setStatusTip(actionStatus->text().toUtf8());
    actionStatus->clear();

    if (!m_sPixmap.isEmpty())
        macro->setPixmap(m_sPixmap.toAscii());
    pixmapLabel->clear();
    m_sPixmap = QString::null;

    if (!actionAccel->text().isEmpty()) {
        macro->setAccel(actionAccel->text().toAscii());
    }
    actionAccel->clear();

    // check whether the macro is already in use
    Action* action = macro->getAction();
    if (action)
    {
        // does all the text related stuff
        action->setText(QString::fromUtf8(macro->getMenuText()));
        action->setToolTip(QString::fromUtf8(macro->getToolTipText()));
        action->setWhatsThis(QString::fromUtf8(macro->getWhatsThis()));
        action->setStatusTip(QString::fromUtf8(macro->getStatusTip()));
        if( macro->getPixmap() )
            action->setIcon(Gui::BitmapFactory().pixmap(macro->getPixmap()));
        action->setShortcut(QString::fromAscii(macro->getAccel()));
    }

    // emit signal to notify the container widget
    modifyMacroAction(actionName);

    // call this at the end because it internally invokes the highlight method
    if (macro->getPixmap())
        item->setIcon(0, Gui::BitmapFactory().pixmap(macro->getPixmap()));
}

void DlgCustomActionsImp::on_buttonRemoveAction_clicked()
{
    // remove item from list view
    QTreeWidgetItem* item = actionListWidget->currentItem();
    if (!item) 
        return;
    int current = actionListWidget->indexOfTopLevelItem(item);
    actionListWidget->takeTopLevelItem(current);
    QByteArray actionName = item->data(1, Qt::UserRole).toByteArray();
    delete item;

    // if the command is registered in the manager just remove it
    CommandManager& rclMan = Application::Instance->commandManager();
    std::vector<Command*> aclCurMacros = rclMan.getGroupCommands("Macros");
    for (std::vector<Command*>::iterator it2 = aclCurMacros.begin(); it2!= aclCurMacros.end(); ++it2)
    {
        if (actionName == (*it2)->getName())
        {
            // emit signal to notify the container widget
            removeMacroAction(actionName);
            // remove from manager and delete it immediately
            rclMan.removeCommand(*it2);
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
    connect(ui->listWidget, SIGNAL(itemClicked (QListWidgetItem *)),
            this, SLOT(accept()));
    connect(ui->addButton, SIGNAL(clicked()),
            this, SLOT(onAddIconPath()));

    QListWidgetItem* item;
    QStringList names = BitmapFactory().findIconFiles();
    for (QStringList::Iterator it = names.begin(); it != names.end(); ++it) {
        item = new QListWidgetItem(ui->listWidget);
        //item->setIcon(QIcon(*it));
        item->setIcon(QIcon(BitmapFactory().pixmap((const char*)it->toUtf8())));
        item->setText(QFileInfo(*it).baseName());
        item->setToolTip(*it);
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
    QString dir = QFileDialog::getExistingDirectory(this, IconDialog::tr("Add icon"), QString());
    if (!dir.isEmpty()) {
        QStringList filters;
        QList<QByteArray> formats = QImageReader::supportedImageFormats();
        for (QList<QByteArray>::iterator it = formats.begin(); it != formats.end(); ++it)
            filters << QString::fromAscii("*.%1").arg(QString::fromAscii(*it).toLower());
        QDir d(dir);
        d.setNameFilters(filters);
        QFileInfoList fi = d.entryInfoList();
        for (QFileInfoList::iterator it = fi.begin(); it != fi.end(); ++it) {
            QListWidgetItem* item;
            QString file = it->absoluteFilePath();
            item = new QListWidgetItem(ui->listWidget);
            item->setIcon(QIcon(file));
            item->setText(it->baseName());
            item->setToolTip(file);
        }

        BitmapFactory().addPath(dir);
        BitmapFactory().addCustomPath(dir);
    }
}

void DlgCustomActionsImp::on_buttonChoosePixmap_clicked()
{
    // create a dialog showing all pixmaps
    Gui::Dialog::IconDialog dlg(this);
    dlg.setModal(true);
    dlg.exec();

    pixmapLabel->clear();
    m_sPixmap = QString::null;
    if (dlg.result() == QDialog::Accepted) {
        QListWidgetItem* item = dlg.currentItem();
        if (item) {
            m_sPixmap = item->text();
            pixmapLabel->setPixmap(item->icon().pixmap(QSize(32,32)));
        }
    }
}

QString DlgCustomActionsImp::newActionName()
{
    int id = 0;
    QString sName;
    bool bUsed;

    CommandManager& rclMan = Application::Instance->commandManager();
    std::vector<Command*> aclCurMacros = rclMan.getGroupCommands("Macros");

    do
    {
        bUsed = false;
        sName = QString::fromAscii("Std_Macro_%1").arg( id++ );

        std::vector<Command*>::iterator it;
        for ( it = aclCurMacros.begin(); it!= aclCurMacros.end(); ++it )
        {
            if (sName == QLatin1String((*it)->getName()))
            {
                bUsed = true;
                break;
            }
        }
    } while ( bUsed );

    return sName;
}

void DlgCustomActionsImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        this->retranslateUi(this);
        actionListWidget->clear();
        showActions();
        actionAccel->setText(qApp->translate("Gui::AccelLineEdit", "none"));
    }
    QWidget::changeEvent(e);
}


#include "moc_DlgActionsImp.cpp" 
