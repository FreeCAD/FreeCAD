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
# include <cstring>
# include <algorithm>
# include <QApplication>
# include <QDebug>
# include <QGenericReturnArgument>
# include <QMessageBox>
# include <QScreen>
# include <QScrollArea>
# include <QScrollBar>
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>

#include "DlgPreferencesImp.h"
#include "ui_DlgPreferences.h"
#include "BitmapFactory.h"
#include "MainWindow.h"
#include "WidgetFactory.h"


using namespace Gui::Dialog;

const int DlgPreferencesImp::GroupNameRole = Qt::UserRole;

/* TRANSLATOR Gui::Dialog::DlgPreferencesImp */

std::list<DlgPreferencesImp::TGroupPages> DlgPreferencesImp::_pages;
DlgPreferencesImp* DlgPreferencesImp::_activeDialog = nullptr;

/**
 *  Constructs a DlgPreferencesImp which is a child of 'parent', with
 *  widget flags set to 'fl'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgPreferencesImp::DlgPreferencesImp(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl), ui(new Ui_DlgPreferences),
      invalidParameter(false), canEmbedScrollArea(true)
{
    ui->setupUi(this);
    ui->listBox->setFixedWidth(108);
    ui->listBox->setGridSize(QSize(108, 75));

    connect(ui->buttonBox,  SIGNAL (helpRequested()),
            getMainWindow(), SLOT (whatsThis()));
    connect(ui->listBox, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
            this, SLOT(changeGroup(QListWidgetItem *, QListWidgetItem*)));

    setupPages();

    // Maintain a static pointer to the current active dialog (if there is one) so that
    // if the static page manipulation functions are called while the dialog is showing
    // it can update its content.
    DlgPreferencesImp::_activeDialog = this;
}

/**
 *  Destroys the object and frees any allocated resources.
 */
DlgPreferencesImp::~DlgPreferencesImp()
{
    if (DlgPreferencesImp::_activeDialog == this) {
        DlgPreferencesImp::_activeDialog = nullptr;
    }
}

void DlgPreferencesImp::setupPages()
{
    // make sure that pages are ready to create
    GetWidgetFactorySupplier();
    for (const auto &group : _pages) {
        QTabWidget* groupTab = createTabForGroup(group.first);
        for (const auto &page : group.second) {
            createPageInGroup(groupTab, page);
        }
    }

    // show the first group
    ui->listBox->setCurrentRow(0);
}

/**
 * Create the necessary widgets for a new group named \a groupName. Returns a 
 * pointer to the group's QTabWidget: that widget's lifetime is managed by the 
 * tabWidgetStack, do not manually deallocate.
 */
QTabWidget* DlgPreferencesImp::createTabForGroup(const std::string &groupName)
{
    QString groupNameQString = QString::fromStdString(groupName);

    QTabWidget* tabWidget = new QTabWidget;
    ui->tabWidgetStack->addWidget(tabWidget);
    tabWidget->setProperty("GroupName", QVariant(groupNameQString));

    QListWidgetItem* item = new QListWidgetItem(ui->listBox);
    item->setData(GroupNameRole, QVariant(groupNameQString));
    item->setText(QObject::tr(groupNameQString.toLatin1()));
    item->setToolTip(QObject::tr(groupNameQString.toLatin1()));
    std::string fileName = groupName;
    for (auto & ch : fileName) {
        if (ch == ' ') ch = '_';
        else ch = tolower(ch);
    }
    fileName = std::string("preferences-") + fileName;
    QPixmap icon = Gui::BitmapFactory().pixmapFromSvg(fileName.c_str(), QSize(48, 48));
    if (icon.isNull()) {
        icon = Gui::BitmapFactory().pixmap(fileName.c_str());
        if (icon.isNull()) {
            qWarning() << "No group icon found for " << fileName.c_str();
        }
        else if (icon.size() != QSize(48, 48)) {
            icon = icon.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            qWarning() << "Group icon for " << fileName.c_str() << " is not of size 48x48, so it was scaled";
        }
    }
    item->setIcon(icon);
    item->setTextAlignment(Qt::AlignHCenter);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    return tabWidget;
}

/**
 * Create a new preference page called \a pageName on the group tab \a tabWidget.
 */
void DlgPreferencesImp::createPageInGroup(QTabWidget *tabWidget, const std::string &pageName)
{
    PreferencePage* page = WidgetFactory().createPreferencePage(pageName.c_str());
    if (page) {
        tabWidget->addTab(page, page->windowTitle());
        page->loadSettings();
        page->setProperty("GroupName", tabWidget->property("GroupName"));
        page->setProperty("PageName", QVariant(QString::fromStdString(pageName)));
    }
    else {
        Base::Console().Warning("%s is not a preference page\n", pageName.c_str());
    }
}

void DlgPreferencesImp::changeGroup(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;
    ui->tabWidgetStack->setCurrentIndex(ui->listBox->row(current));
}

/**
 * Adds a preference page with its class name \a className and
 * the group \a group it belongs to. To create this page it must
 * be registered in the WidgetFactory.
 * @see WidgetFactory
 * @see PrefPageProducer
 */
void DlgPreferencesImp::addPage(const std::string& className, const std::string& group)
{
    std::list<TGroupPages>::iterator groupToAddTo = _pages.end();
    for (std::list<TGroupPages>::iterator it = _pages.begin(); it != _pages.end(); ++it) {
        if (it->first == group) {
            groupToAddTo = it;
            break;
        }
    }

    if (groupToAddTo != _pages.end()) {
        // The group exists: add this page to the end of the list
        groupToAddTo->second.push_back(className);
    }
    else {
        // This is a new group: create it, with its one page
        std::list<std::string> pages;
        pages.push_back(className);
        _pages.push_back(std::make_pair(group, pages));
    }

    if (DlgPreferencesImp::_activeDialog != nullptr) {
        // If the dialog is currently showing, tell it to insert the new page
        _activeDialog->reloadPages();
    }
}

void DlgPreferencesImp::removePage(const std::string& className, const std::string& group)
{
    for (std::list<TGroupPages>::iterator it = _pages.begin(); it != _pages.end(); ++it) {
        if (it->first == group) {
            if (className.empty()) {
                _pages.erase(it);
                return;
            }
            else {
                std::list<std::string>& p = it->second;
                for (std::list<std::string>::iterator jt = p.begin(); jt != p.end(); ++jt) {
                    if (*jt == className) {
                        p.erase(jt);
                        if (p.empty())
                            _pages.erase(it);
                        return;
                    }
                }
            }
        }
    }
}

/**
 * Activates the page at position \a index of the group with name \a group.
 */
void DlgPreferencesImp::activateGroupPage(const QString& group, int index)
{
    int ct = ui->listBox->count();
    for (int i=0; i<ct; i++) {
        QListWidgetItem* item = ui->listBox->item(i);
        if (item->data(GroupNameRole).toString() == group) {
            ui->listBox->setCurrentItem(item);
            QTabWidget* tabWidget = (QTabWidget*)ui->tabWidgetStack->widget(i);
            tabWidget->setCurrentIndex(index);
            break;
        }
    }
}

void DlgPreferencesImp::accept()
{
    this->invalidParameter = false;
    applyChanges();
    if (!this->invalidParameter)
        QDialog::accept();
}

void DlgPreferencesImp::on_buttonBox_clicked(QAbstractButton* btn)
{
    if (ui->buttonBox->standardButton(btn) == QDialogButtonBox::Apply)
        applyChanges();
    else if (ui->buttonBox->standardButton(btn) == QDialogButtonBox::Reset)
        restoreDefaults();
}

void DlgPreferencesImp::restoreDefaults()
{
    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(tr("Clear user settings"));
    box.setText(tr("Do you want to clear all your user settings?"));
    box.setInformativeText(tr("If you agree all your settings will be cleared."));
    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    box.setDefaultButton(QMessageBox::No);

    if (box.exec() == QMessageBox::Yes) {
        // keep this parameter
        bool saveParameter = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
                              GetBool("SaveUserParameter", true);

        ParameterManager* mgr = App::GetApplication().GetParameterSet("User parameter");
        mgr->Clear();

        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
                              SetBool("SaveUserParameter", saveParameter);

#if 0
        QList<PreferencePage*> pages = this->findChildren<PreferencePage*>();
        for (QList<PreferencePage*>::iterator it = pages.begin(); it != pages.end(); ++it) {
            (*it)->loadSettings();
        }
#else
        reject();
#endif
    }
}

/**
 * If the dialog is currently showing and the static variable _pages changed, this function 
 * will rescan that list of pages and add any that are new to the current dialog. It will not
 * remove any pages that are no longer in the list, and will not change the user's current
 * active page.
 */
void DlgPreferencesImp::reloadPages()
{
    // Make sure that pages are ready to create
    GetWidgetFactorySupplier();

    for (const auto &group : _pages) {
        QString groupName = QString::fromStdString(group.first);

        // First, does this group already exist?
        QTabWidget* tabWidget = nullptr;
        for (int tabNumber = 0; tabNumber < ui->tabWidgetStack->count(); ++tabNumber) {
            auto thisTabWidget = qobject_cast<QTabWidget*>(ui->tabWidgetStack->widget(tabNumber));
            if (thisTabWidget->property("GroupName").toString() == groupName) {
                tabWidget = thisTabWidget;
                break;
            }
        }

        // This is a new tab that wasn't there when we started this instance of the dialog: 
        if (!tabWidget) {
            tabWidget = createTabForGroup(group.first);
        }

        // Move on to the pages in the group to see if we need to add any
        for (const auto& page : group.second) {

            // Does this page already exist?
            QString pageName = QString::fromStdString(page);
            bool pageExists = false;
            for (int pageNumber = 0; pageNumber < tabWidget->count(); ++pageNumber) {
                PreferencePage* prefPage = qobject_cast<PreferencePage*>(tabWidget->widget(pageNumber));
                if (prefPage && prefPage->property("PageName").toString() == pageName) {
                    pageExists = true;
                    break;
                }
            }

            // This is a new page that wasn't there when we started this instance of the dialog:
            if (!pageExists) {
                createPageInGroup(tabWidget, page);
            }
        }
    }
}

void DlgPreferencesImp::applyChanges()
{
    // Checks if any of the classes that represent several pages of settings
    // (DlgSettings*.*) implement checkSettings() method.  If any of them do,
    // call it to validate if user input is correct.  If something fails (i.e.,
    // not correct), shows a messageBox and set this->invalidParameter = true to
    // cancel further operation in other methods (like in accept()).
    try {
        for (int i=0; i<ui->tabWidgetStack->count(); i++) {
            QTabWidget* tabWidget = (QTabWidget*)ui->tabWidgetStack->widget(i);
            for (int j=0; j<tabWidget->count(); j++) {
                QWidget* page = tabWidget->widget(j);
                int index = page->metaObject()->indexOfMethod("checkSettings()");
                try {
                    if (index >= 0) {
                        page->qt_metacall(QMetaObject::InvokeMetaMethod, index, nullptr);
                    }
                }
                catch (const Base::Exception& e) {
                    ui->listBox->setCurrentRow(i);
                    tabWidget->setCurrentIndex(j);
                    QMessageBox::warning(this, tr("Wrong parameter"), QString::fromLatin1(e.what()));
                    throw;
                }
            }
        }
    } catch (const Base::Exception&) {
        this->invalidParameter = true;
        return;
    }

    // If everything is ok (i.e., no validation problem), call method
    // saveSettings() in every subpage (DlgSetting*) object.
    for (int i=0; i<ui->tabWidgetStack->count(); i++) {
        QTabWidget* tabWidget = (QTabWidget*)ui->tabWidgetStack->widget(i);
        for (int j=0; j<tabWidget->count(); j++) {
            PreferencePage* page = qobject_cast<PreferencePage*>(tabWidget->widget(j));
            if (page)
                page->saveSettings();
        }
    }

    bool saveParameter = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
                          GetBool("SaveUserParameter", true);
    if (saveParameter) {
        ParameterManager* parmgr = App::GetApplication().GetParameterSet("User parameter");
        parmgr->SaveDocument(App::Application::Config()["UserParameter"].c_str());
    }
}

void DlgPreferencesImp::showEvent(QShowEvent* ev)
{
    //canEmbedScrollArea = false;
    this->adjustSize();
    QDialog::showEvent(ev);
}

void DlgPreferencesImp::resizeEvent(QResizeEvent* ev)
{
    if (canEmbedScrollArea) {
        // embed the widget stack into a scroll area if the size is
        // bigger than the available desktop
        QRect rect = QApplication::primaryScreen()->availableGeometry();
        int maxHeight = rect.height() - 60;
        int maxWidth = rect.width();
        if (height() > maxHeight || width() > maxWidth) {
            canEmbedScrollArea = false;
            ui->hboxLayout->removeWidget(ui->tabWidgetStack);
            QScrollArea* scrollArea = new QScrollArea(this);
            scrollArea->setFrameShape(QFrame::NoFrame);
            scrollArea->setWidgetResizable(true);
            scrollArea->setWidget(ui->tabWidgetStack);
            ui->hboxLayout->addWidget(scrollArea);

            // if possible the minimum width should so that it doesn't show
            // a horizontal scroll bar.
            QScrollBar* bar = scrollArea->verticalScrollBar();
            if (bar) {
                int newWidth = width() + bar->width();
                newWidth = std::min<int>(newWidth, maxWidth);
                int newHeight = std::min<int>(height(), maxHeight);
                QMetaObject::invokeMethod(this, "resizeWindow",
                    Qt::QueuedConnection,
                    QGenericReturnArgument(),
                    Q_ARG(int, newWidth),
                    Q_ARG(int, newHeight));
            }
            QPoint center = rect.center();
            move(center.x() - width() * 0.5, 10);
        }
    }
    QDialog::resizeEvent(ev);
}

void DlgPreferencesImp::resizeWindow(int w, int h)
{
    resize(w, h);
}

void DlgPreferencesImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        // update the widgets' tabs
        for (int i=0; i<ui->tabWidgetStack->count(); i++) {
            QTabWidget* tabWidget = (QTabWidget*)ui->tabWidgetStack->widget(i);
            for (int j=0; j<tabWidget->count(); j++) {
                QWidget* page = tabWidget->widget(j);
                tabWidget->setTabText(j, page->windowTitle());
            }
        }
        // update the items' text
        for (int i=0; i<ui->listBox->count(); i++) {
            QListWidgetItem *item = ui->listBox->item(i);
            QByteArray group = item->data(GroupNameRole).toByteArray();
            item->setText(QObject::tr(group.constData()));
        }
    } else {
        QWidget::changeEvent(e);
    }
}

void DlgPreferencesImp::reload()
{
    for (int i = 0; i < ui->tabWidgetStack->count(); i++) {
        QTabWidget* tabWidget = (QTabWidget*)ui->tabWidgetStack->widget(i);
        for (int j = 0; j < tabWidget->count(); j++) {
            PreferencePage* page = qobject_cast<PreferencePage*>(tabWidget->widget(j));
            if (page)
                page->loadSettings();
        }
    }
    applyChanges();
}

#include "moc_DlgPreferencesImp.cpp"
