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
# include <QApplication>
# include <QLocale>
# include <QStyleFactory>
# include <QTextStream>
#endif

#include "DlgGeneralImp.h"
#include "Action.h"
#include "Application.h"
#include "DockWindowManager.h"
#include "MainWindow.h"
#include "PrefWidgets.h"
#include "Language/Translator.h"

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgGeneralImp */

/**
 *  Constructs a DlgGeneralImp which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
DlgGeneralImp::DlgGeneralImp( QWidget* parent )
  : PreferencePage( parent ), watched(0)
{
    this->setupUi(this);
    // hide to fix 0000375: Mac Interface window style setting not saved
    windowStyleLabel->hide();
    WindowStyle->hide();
    
    // fills the combo box with all available workbenches
    // sorted by their menu text
    QStringList work = Application::Instance->workbenches();
    QMap<QString, QString> menuText;
    for (QStringList::Iterator it = work.begin(); it != work.end(); ++it) {
        QString text = Application::Instance->workbenchMenuText(*it);
        menuText[text] = *it;
    }

    for (QMap<QString, QString>::Iterator it = menuText.begin(); it != menuText.end(); ++it) {
        QPixmap px = Application::Instance->workbenchIcon(it.value());
        if (px.isNull())
            AutoloadModuleCombo->addItem(it.key(), QVariant(it.value()));
        else
            AutoloadModuleCombo->addItem(px, it.key(), QVariant(it.value()));
    }

    // do not save the content but the current item only
    QWidget* dw = DockWindowManager::instance()->getDockWindow("Report view");
    if (dw)
    {
        watched = dw->findChild<QTabWidget*>();
        if (watched)
        {
            for (int i=0; i<watched->count(); i++)
                AutoloadTabCombo->addItem( watched->tabText(i) );
            watched->installEventFilter(this);
        }
    }
    if (!watched) {
        // use separate dock widgets instead of the old tab widget
        tabReportLabel->hide();
        AutoloadTabCombo->hide();
    }
}

/** 
 *  Destroys the object and frees any allocated resources
 */
DlgGeneralImp::~DlgGeneralImp()
{
    // no need to delete child widgets, Qt does it all for us
    if (watched)
        watched->removeEventFilter(this);
}

/** Sets the size of the recent file list from the user parameters.
 * @see RecentFilesAction
 * @see StdCmdRecentFiles
 */
void DlgGeneralImp::setRecentFileSize()
{
    RecentFilesAction *recent = getMainWindow()->findChild<RecentFilesAction *>(QLatin1String("recentFiles"));
    if (recent) {
        ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("RecentFiles");
        recent->resizeList(hGrp->GetInt("RecentFiles", 4));
    }
}

void DlgGeneralImp::saveSettings()
{
    int index = AutoloadModuleCombo->currentIndex();
    QVariant data = AutoloadModuleCombo->itemData(index);
    QString startWbName = data.toString();
    App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
                          SetASCII("AutoloadModule", startWbName.toAscii());
    
    AutoloadTabCombo->onSave();
    RecentFiles->onSave();
    SplashScreen->onSave();

    // set new user defined style
    //(void)QApplication::setStyle(WindowStyle->currentText());

    setRecentFileSize();
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("General");
    QString lang = QLocale::languageToString(QLocale::system().language());
    QByteArray language = hGrp->GetASCII("Language", (const char*)lang.toAscii()).c_str();
    QByteArray current = Languages->itemData(Languages->currentIndex()).toByteArray();
    if (current != language)
    {
        hGrp->SetASCII("Language", current.constData());
        Translator::instance()->activateLanguage(current.constData());
    }

    QVariant size = this->toolbarIconSize->itemData(this->toolbarIconSize->currentIndex());
    int pixel = size.toInt();
    hGrp->SetInt("ToolbarIconSize", pixel);
    getMainWindow()->setIconSize(QSize(pixel,pixel));

    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow");
    hGrp->SetBool("TiledBackground", this->tiledBackground->isChecked());
    QMdiArea* mdi = getMainWindow()->findChild<QMdiArea*>();
    mdi->setProperty("showImage", this->tiledBackground->isChecked());

    QVariant sheet = this->StyleSheets->itemData(this->StyleSheets->currentIndex());
    if (this->selectedStyleSheet != sheet.toString()) {
        this->selectedStyleSheet = sheet.toString();
        hGrp->SetASCII("StyleSheet", (const char*)sheet.toByteArray());

        if (!sheet.toString().isEmpty()) {
            QFile f(sheet.toString());
            if (f.open(QFile::ReadOnly)) {
                mdi->setBackground(QBrush(Qt::NoBrush));
                QTextStream str(&f);
                qApp->setStyleSheet(str.readAll());
            }
        }
    }

    if (sheet.toString().isEmpty()) {
        if (this->tiledBackground->isChecked()) {
            qApp->setStyleSheet(QString());
            mdi->setBackground(QPixmap(QLatin1String(":/icons/background.png")));
        }
        else {
            qApp->setStyleSheet(QString());
            mdi->setBackground(QBrush(QColor(160,160,160)));
        }
    }

    if (mdi->style())
        mdi->style()->unpolish(qApp);
}

void DlgGeneralImp::loadSettings()
{
    std::string start = App::Application::Config()["StartWorkbench"];
    start = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
                                  GetASCII("AutoloadModule", start.c_str());
    QString startWbName = QLatin1String(start.c_str());
    AutoloadModuleCombo->setCurrentIndex(AutoloadModuleCombo->findData(startWbName));

    AutoloadTabCombo->onRestore();
    RecentFiles->onRestore();
    SplashScreen->onRestore();

    // fill up styles
    //
    QStringList styles = QStyleFactory::keys();
    WindowStyle->addItems(styles);
    QString style = QApplication::style()->objectName().toLower();
    int i=0;
    for (QStringList::ConstIterator it = styles.begin(); it != styles.end(); ++it, i++) {
        if (style == (*it).toLower()) {
            WindowStyle->setCurrentIndex( i );
            break;
        }
    }

    // search for the language files
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("General");
    QString lang = QLocale::languageToString(QLocale::system().language());
    QByteArray language = hGrp->GetASCII("Language", (const char*)lang.toAscii()).c_str();
    int index = 1;
    Languages->addItem(QString::fromAscii("English"), QByteArray("English"));
    TStringMap list = Translator::instance()->supportedLocales();
    for (TStringMap::iterator it = list.begin(); it != list.end(); ++it, index++) {
        QLocale locale(QString::fromAscii(it->second.c_str()));
        QByteArray lang = it->first.c_str();
        QString langname = QString::fromAscii(lang.constData());
#if QT_VERSION >= 0x040800
        QString native = locale.nativeLanguageName();
        if (!native.isEmpty()) {
            if (native[0].isLetter())
                native[0] = native[0].toUpper();
            langname = native;
        }
#endif
        Languages->addItem(langname, lang);
        if (language == lang) {
            Languages->setCurrentIndex(index);
        }
    }

    int size = QApplication::style()->pixelMetric(QStyle::PM_ToolBarIconSize);
    int current = getMainWindow()->iconSize().width();
    this->toolbarIconSize->addItem(tr("Default (%1 x %1)").arg(size), QVariant((int)size));
    this->toolbarIconSize->addItem(tr("Small (%1 x %1)").arg(16), QVariant((int)16));
    this->toolbarIconSize->addItem(tr("Large (%1 x %1)").arg(32), QVariant((int)32));
    this->toolbarIconSize->addItem(tr("Extra large (%1 x %1)").arg(48), QVariant((int)48));
    index = this->toolbarIconSize->findData(QVariant(current));
    if (index > -1) this->toolbarIconSize->setCurrentIndex(index);

    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow");
    this->tiledBackground->setChecked(hGrp->GetBool("TiledBackground", false));

    // List all .css files
    QMap<QString, QString> cssFiles;
    QDir dir;
    QString filter = QString::fromAscii("*.css");
    QFileInfoList fileNames;

    // read from user directory
    dir.setPath(QString::fromUtf8((App::Application::getUserAppDataDir() + "Gui/Stylesheets/").c_str()));
    fileNames = dir.entryInfoList(QStringList(filter), QDir::Files, QDir::Name);
    for (QFileInfoList::iterator it = fileNames.begin(); it != fileNames.end(); ++it) {
        if (cssFiles.find(it->baseName()) == cssFiles.end()) {
            cssFiles[it->baseName()] = it->absoluteFilePath();
        }
    }

    // read from resource directory
    dir.setPath(QString::fromUtf8((App::Application::getResourceDir() + "Gui/Stylesheets/").c_str()));
    fileNames = dir.entryInfoList(QStringList(filter), QDir::Files, QDir::Name);
    for (QFileInfoList::iterator it = fileNames.begin(); it != fileNames.end(); ++it) {
        if (cssFiles.find(it->baseName()) == cssFiles.end()) {
            cssFiles[it->baseName()] = it->absoluteFilePath();
        }
    }

    // read from the built-in directory
    dir.setPath(QLatin1String(":/stylesheets"));
    fileNames = dir.entryInfoList(QStringList(filter), QDir::Files, QDir::Name);
    this->StyleSheets->addItem(tr("No style sheet"), QString::fromAscii(""));
    for (QFileInfoList::iterator it = fileNames.begin(); it != fileNames.end(); ++it) {
        if (cssFiles.find(it->baseName()) == cssFiles.end()) {
            cssFiles[it->baseName()] = it->absoluteFilePath();
        }
    }

    // now add all unique items
    for (QMap<QString, QString>::iterator it = cssFiles.begin(); it != cssFiles.end(); ++it) {
        this->StyleSheets->addItem(it.key(), it.value());
    }

    this->selectedStyleSheet = QString::fromAscii(hGrp->GetASCII("StyleSheet").c_str());
    index = this->StyleSheets->findData(this->selectedStyleSheet);
    if (index > -1) this->StyleSheets->setCurrentIndex(index);
}

void DlgGeneralImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        retranslateUi(this);
        for (int i = 0; i < Languages->count(); i++) {
            QByteArray lang = Languages->itemData(i).toByteArray();
            Languages->setItemText(i, Gui::Translator::tr(lang.constData()));
        }
    } else {
        QWidget::changeEvent(e);
    }
}

bool DlgGeneralImp::eventFilter(QObject* o, QEvent* e)
{
    // make sure that report tabs have been translated
    if (o == watched && e->type() == QEvent::LanguageChange) {
        for (int i=0; i<watched->count(); i++)
            AutoloadTabCombo->setItemText( i, watched->tabText(i) );
    }

    return QWidget::eventFilter(o, e);
}

#include "moc_DlgGeneralImp.cpp"
