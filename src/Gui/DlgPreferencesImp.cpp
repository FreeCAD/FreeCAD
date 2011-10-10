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
# include <QMessageBox>
#endif

#include <Base/Exception.h>
#include <Base/Console.h> 

#include "DlgPreferencesImp.h"
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
 *  TRUE to construct a modal dialog.
 */
DlgPreferencesImp::DlgPreferencesImp( QWidget* parent, Qt::WFlags fl )
    : QDialog(parent, fl)
{
    this->setupUi(this);
    connect( buttonHelp,  SIGNAL ( clicked() ), getMainWindow(), SLOT ( whatsThis() ));
    connect(listBox, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
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
        this->tabWidgetStack->addWidget(tabWidget);
        
        QByteArray group = it->first.c_str();
        QListWidgetItem *item = new QListWidgetItem(listBox);
        item->setData(Qt::UserRole, QVariant(group));
        item->setText(QObject::tr(group.constData()));
        std::string fileName = it->first;
        for (std::string::iterator ch = fileName.begin(); ch != fileName.end(); ++ch) {
            if (*ch == ' ') *ch = '_';
            else *ch = tolower(*ch);
        }
        fileName = std::string("preferences-") + fileName;
        QPixmap icon = Gui::BitmapFactory().pixmapFromSvg(fileName.c_str(), QSize(96,96));
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
    listBox->setCurrentRow(0);
}

void DlgPreferencesImp::changeGroup(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;
    tabWidgetStack->setCurrentIndex(listBox->row(current));
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
    int ct = listBox->count();
    for (int i=0; i<ct; i++) {
        QListWidgetItem* item = listBox->item(i);
        if (item->data(Qt::UserRole).toString() == group) {
            listBox->setCurrentItem(item);
            QTabWidget* tabWidget = (QTabWidget*)tabWidgetStack->widget(i);
            tabWidget->setCurrentIndex(index);
            break;
        }
    }
}

void DlgPreferencesImp::accept()
{
    this->invalidParameter = false;
    on_buttonApply_clicked();
    if (!this->invalidParameter)
        QDialog::accept();
}

void DlgPreferencesImp::on_buttonApply_clicked()
{
    try {
        for (int i=0; i<tabWidgetStack->count(); i++) {
            QTabWidget* tabWidget = (QTabWidget*)tabWidgetStack->widget(i);
            for (int j=0; j<tabWidget->count(); j++) {
                QWidget* page = tabWidget->widget(j);
                int index = page->metaObject()->indexOfMethod("checkSettings()");
                try {
                    if (index >= 0) {
                        page->qt_metacall(QMetaObject::InvokeMetaMethod, index, 0);
                    }
                } catch (const Base::Exception& e) {
                    listBox->setCurrentRow(i);
                    tabWidget->setCurrentIndex(j);
                    QMessageBox::warning(this, tr("Wrong parameter"), QString::fromAscii(e.what()));
                    throw;
                }
            }
        }
    } catch (const Base::Exception&) {
        this->invalidParameter = true;
        return;
    }

    for (int i=0; i<tabWidgetStack->count(); i++) {
        QTabWidget* tabWidget = (QTabWidget*)tabWidgetStack->widget(i);
        for (int j=0; j<tabWidget->count(); j++) {
            PreferencePage* page = qobject_cast<PreferencePage*>(tabWidget->widget(j));
            if (page)
                page->saveSettings();
        }
    }
}

void DlgPreferencesImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        retranslateUi(this);
        // update the widgets' tabs
        for (int i=0; i<tabWidgetStack->count(); i++) {
            QTabWidget* tabWidget = (QTabWidget*)tabWidgetStack->widget(i);
            for (int j=0; j<tabWidget->count(); j++) {
                QWidget* page = tabWidget->widget(j);
                tabWidget->setTabText(j, page->windowTitle());
            }
        }
        // update the items' text
        for (int i=0; i<listBox->count(); i++) {
            QListWidgetItem *item = listBox->item(i);
            QByteArray group = item->data(Qt::UserRole).toByteArray();
            item->setText(QObject::tr(group.constData()));
        }
    } else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgPreferencesImp.cpp"
