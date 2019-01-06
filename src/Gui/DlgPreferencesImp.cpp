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
# include <QDesktopWidget>
# include <QGenericReturnArgument>
# include <QMessageBox>
# include <QScrollArea>
# include <QScrollBar>
#endif

#include <Base/Exception.h>
#include <Base/Console.h> 
#include <App/Application.h>
#include "DlgPreferencesImp.h"
#include "ui_DlgPreferences.h"
#include "PropertyPage.h"
#include "WidgetFactory.h"
#include "BitmapFactory.h"
#include "MainWindow.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgPreferencesImp */

std::list<DlgPreferencesImp::TGroupPages> DlgPreferencesImp::_pages;

/**
 *  Constructs a DlgPreferencesImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgPreferencesImp::DlgPreferencesImp(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl), ui(new Ui_DlgPreferences),
      invalidParameter(false), canEmbedScrollArea(true)
{
    ui->setupUi(this);
    ui->listBox->setFixedWidth(130);
    ui->listBox->setGridSize(QSize(108, 120));

    connect(ui->buttonBox,  SIGNAL (helpRequested()),
            getMainWindow(), SLOT (whatsThis()));
    connect(ui->listBox, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
            this, SLOT(changeGroup(QListWidgetItem *, QListWidgetItem*)));

    setupPages();
}

/**
 *  Destroys the object and frees any allocated resources.
 */
DlgPreferencesImp::~DlgPreferencesImp()
{
  // no need to delete child widgets, Qt does it all for us
}

void DlgPreferencesImp::setupPages()
{
    // make sure that pages are ready to create
    GetWidgetFactorySupplier();
    for (std::list<TGroupPages>::iterator it = _pages.begin(); it != _pages.end(); ++it) {
        QTabWidget* tabWidget = new QTabWidget;
        ui->tabWidgetStack->addWidget(tabWidget);
        
        QByteArray group = it->first.c_str();
        QListWidgetItem *item = new QListWidgetItem(ui->listBox);
        item->setData(Qt::UserRole, QVariant(group));
        item->setText(QObject::tr(group.constData()));
        std::string fileName = it->first;
        for (std::string::iterator ch = fileName.begin(); ch != fileName.end(); ++ch) {
            if (*ch == ' ') *ch = '_';
            else *ch = tolower(*ch);
        }
        fileName = std::string("preferences-") + fileName;
        QPixmap icon = Gui::BitmapFactory().pixmapFromSvg(fileName.c_str(), QSize(96,96));
        if (icon.isNull()) {
            icon = Gui::BitmapFactory().pixmap(fileName.c_str());
            if (icon.isNull()) {
                qWarning() << "No group icon found for " << fileName.c_str();
            }
            else if (icon.size() != QSize(96,96)) {
                qWarning() << "Group icon for " << fileName.c_str() << " is not of size 96x96";
            }
        }
        item->setIcon(icon);
        item->setTextAlignment(Qt::AlignHCenter);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        for (std::list<std::string>::iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
            PreferencePage* page = WidgetFactory().createPreferencePage(jt->c_str());
            if (page) {
                tabWidget->addTab(page, page->windowTitle());
                page->loadSettings();
            }
            else {
                Base::Console().Warning("%s is not a preference page\n", jt->c_str());
            }
        }
    }

    // show the first group
    ui->listBox->setCurrentRow(0);
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
    for (std::list<TGroupPages>::iterator it = _pages.begin(); it != _pages.end(); ++it) {
        if (it->first == group) {
            it->second.push_back(className);
            return;
        }
    }

    std::list<std::string> pages;
    pages.push_back(className);
    _pages.push_back(std::make_pair(group, pages));
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
        if (item->data(Qt::UserRole).toString() == group) {
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
                        page->qt_metacall(QMetaObject::InvokeMetaMethod, index, 0);
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
    canEmbedScrollArea = false;
    QDialog::showEvent(ev);
}

void DlgPreferencesImp::resizeEvent(QResizeEvent* ev)
{
    if (canEmbedScrollArea) {
        // embed the widget stack into a scroll area if the size is
        // bigger than the available desktop
        QRect rect = QApplication::desktop()->availableGeometry();
        int maxHeight = rect.height();
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
                int newHeight = std::min<int>(height(), maxHeight-30);
                QMetaObject::invokeMethod(this, "resizeWindow",
                    Qt::QueuedConnection,
                    QGenericReturnArgument(),
                    Q_ARG(int, newWidth),
                    Q_ARG(int, newHeight));
            }
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
            QByteArray group = item->data(Qt::UserRole).toByteArray();
            item->setText(QObject::tr(group.constData()));
        }
    } else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgPreferencesImp.cpp"
