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
#include "ui_DlgGeneral.h"
#include "Action.h"
#include "Application.h"
#include "DockWindowManager.h"
#include "MainWindow.h"
#include "PrefWidgets.h"
#include "PythonConsole.h"
#include "Language/Translator.h"

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgGeneralImp */

/**
 *  Constructs a DlgGeneralImp which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgGeneralImp::DlgGeneralImp( QWidget* parent )
  : PreferencePage(parent)
  , ui(new Ui_DlgGeneral)
{
    ui->setupUi(this);
    
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
            ui->AutoloadModuleCombo->addItem(it.key(), QVariant(it.value()));
        else
            ui->AutoloadModuleCombo->addItem(px, it.key(), QVariant(it.value()));
    }
}

/** 
 *  Destroys the object and frees any allocated resources
 */
DlgGeneralImp::~DlgGeneralImp()
{
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
    int index = ui->AutoloadModuleCombo->currentIndex();
    QVariant data = ui->AutoloadModuleCombo->itemData(index);
    QString startWbName = data.toString();
    App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
                          SetASCII("AutoloadModule", startWbName.toLatin1());

    ui->RecentFiles->onSave();
    ui->SplashScreen->onSave();
    ui->PythonWordWrap->onSave();
  
    QWidget* pc = DockWindowManager::instance()->getDockWindow("Python console");
    PythonConsole *pcPython = qobject_cast<PythonConsole*>(pc);
    if (pcPython) {
        bool pythonWordWrap = App::GetApplication().GetUserParameter().
            GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("General")->GetBool("PythonWordWrap", true);

        if (pythonWordWrap) {
            pcPython->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        } else {
            pcPython->setWordWrapMode(QTextOption::NoWrap);
        }
    }

    setRecentFileSize();
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("General");
    QString lang = QLocale::languageToString(QLocale::system().language());
    QByteArray language = hGrp->GetASCII("Language", (const char*)lang.toLatin1()).c_str();
    QByteArray current = ui->Languages->itemData(ui->Languages->currentIndex()).toByteArray();
    if (current != language) {
        hGrp->SetASCII("Language", current.constData());
        Translator::instance()->activateLanguage(current.constData());
    }

    QVariant size = ui->toolbarIconSize->itemData(ui->toolbarIconSize->currentIndex());
    int pixel = size.toInt();
    hGrp->SetInt("ToolbarIconSize", pixel);
    getMainWindow()->setIconSize(QSize(pixel,pixel));

    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow");
    hGrp->SetBool("TiledBackground", ui->tiledBackground->isChecked());
    QMdiArea* mdi = getMainWindow()->findChild<QMdiArea*>();
    mdi->setProperty("showImage", ui->tiledBackground->isChecked());

    QVariant sheet = ui->StyleSheets->itemData(ui->StyleSheets->currentIndex());
    if (this->selectedStyleSheet != sheet.toString()) {
        this->selectedStyleSheet = sheet.toString();
        hGrp->SetASCII("StyleSheet", (const char*)sheet.toByteArray());

        if (!sheet.toString().isEmpty()) {
            QFile f(sheet.toString());
            if (f.open(QFile::ReadOnly)) {
                mdi->setBackground(QBrush(Qt::NoBrush));
                QTextStream str(&f);
                qApp->setStyleSheet(str.readAll());

                ActionStyleEvent e(ActionStyleEvent::Clear);
                qApp->sendEvent(getMainWindow(), &e);
            }
        }
    }

    if (sheet.toString().isEmpty()) {
        if (ui->tiledBackground->isChecked()) {
            qApp->setStyleSheet(QString());
            ActionStyleEvent e(ActionStyleEvent::Restore);
            qApp->sendEvent(getMainWindow(), &e);
            mdi->setBackground(QPixmap(QLatin1String("images:background.png")));
        }
        else {
            qApp->setStyleSheet(QString());
            ActionStyleEvent e(ActionStyleEvent::Restore);
            qApp->sendEvent(getMainWindow(), &e);
            mdi->setBackground(QBrush(QColor(160,160,160)));
        }

#if QT_VERSION == 0x050600 && defined(Q_OS_WIN32)
        // Under Windows the tree indicator branch gets corrupted after a while.
        // For more details see also https://bugreports.qt.io/browse/QTBUG-52230
        // and https://codereview.qt-project.org/#/c/154357/2//ALL,unified
        // A workaround for Qt 5.6.0 is to set a minimal style sheet.
        QString qss = QString::fromLatin1(
               "QTreeView::branch:closed:has-children  {\n"
               "    image: url(:/icons/style/windows_branch_closed.png);\n"
               "}\n"
               "\n"
               "QTreeView::branch:open:has-children  {\n"
               "    image: url(:/icons/style/windows_branch_open.png);\n"
               "}\n");
        qApp->setStyleSheet(qss);
#endif
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
    ui->AutoloadModuleCombo->setCurrentIndex(ui->AutoloadModuleCombo->findData(startWbName));

    ui->RecentFiles->onRestore();
    ui->SplashScreen->onRestore();
    ui->PythonWordWrap->onRestore();

    // search for the language files
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("General");
    QString langToStr = QLocale::languageToString(QLocale::system().language());
    QByteArray language = hGrp->GetASCII("Language", langToStr.toLatin1()).c_str();

    int index = 1;
    TStringMap list = Translator::instance()->supportedLocales();
    ui->Languages->addItem(QString::fromLatin1("English"), QByteArray("English"));
    for (TStringMap::iterator it = list.begin(); it != list.end(); ++it, index++) {
        QByteArray lang = it->first.c_str();
        QString langname = QString::fromLatin1(lang.constData());

#if QT_VERSION >= 0x040800
        QLocale locale(QString::fromLatin1(it->second.c_str()));
        QString native = locale.nativeLanguageName();
        if (!native.isEmpty()) {
            if (native[0].isLetter())
                native[0] = native[0].toUpper();
            langname = native;
        }
#endif

        ui->Languages->addItem(langname, lang);
        if (language == lang) {
            ui->Languages->setCurrentIndex(index);
        }
    }

    QAbstractItemModel* model = ui->Languages->model();
    if (model)
        model->sort(0);

    int current = getMainWindow()->iconSize().width();
    ui->toolbarIconSize->addItem(tr("Small (%1px)").arg(16), QVariant((int)16));
    ui->toolbarIconSize->addItem(tr("Medium (%1px)").arg(24), QVariant((int)24));
    ui->toolbarIconSize->addItem(tr("Large (%1px)").arg(32), QVariant((int)32));
    ui->toolbarIconSize->addItem(tr("Extra large (%1px)").arg(48), QVariant((int)48));
    index = ui->toolbarIconSize->findData(QVariant(current));
    if (index < 0) {
        ui->toolbarIconSize->addItem(tr("Custom (%1px)").arg(current), QVariant((int)current));
        index = ui->toolbarIconSize->findData(QVariant(current));
    } 
    ui->toolbarIconSize->setCurrentIndex(index);

    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow");
    ui->tiledBackground->setChecked(hGrp->GetBool("TiledBackground", false));

    // List all .qss/.css files
    QMap<QString, QString> cssFiles;
    QDir dir;
    QStringList filter;
    filter << QString::fromLatin1("*.qss");
    filter << QString::fromLatin1("*.css");
    QFileInfoList fileNames;

    // read from user, resource and built-in directory
    QStringList qssPaths = QDir::searchPaths(QString::fromLatin1("qss"));
    for (QStringList::iterator it = qssPaths.begin(); it != qssPaths.end(); ++it) {
        dir.setPath(*it);
        fileNames = dir.entryInfoList(filter, QDir::Files, QDir::Name);
        for (QFileInfoList::iterator jt = fileNames.begin(); jt != fileNames.end(); ++jt) {
            if (cssFiles.find(jt->baseName()) == cssFiles.end()) {
                cssFiles[jt->baseName()] = jt->absoluteFilePath();
            }
        }
    }

    // now add all unique items
    ui->StyleSheets->addItem(tr("No style sheet"), QString::fromLatin1(""));
    for (QMap<QString, QString>::iterator it = cssFiles.begin(); it != cssFiles.end(); ++it) {
        ui->StyleSheets->addItem(it.key(), it.value());
    }

    this->selectedStyleSheet = QString::fromLatin1(hGrp->GetASCII("StyleSheet").c_str());
    index = ui->StyleSheets->findData(this->selectedStyleSheet);
    if (index > -1) ui->StyleSheets->setCurrentIndex(index);
}

void DlgGeneralImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgGeneralImp.cpp"
