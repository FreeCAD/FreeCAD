/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QCoreApplication>
# include <QDate>
# include <QDesktopServices>
# include <QDir>
# include <QLocale>
# include <QMessageBox>
# include <QSettings>
# include <QUrl>
# include <cmath>
# include <vector>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Gui/DocumentRecovery.h>
#include <Gui/MainWindow.h>

#include "DlgSettingsCacheDirectory.h"
#include "ui_DlgSettingsCacheDirectory.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsCacheDirectory */

QString DlgSettingsCacheDirectory::currentSize;

DlgSettingsCacheDirectory::DlgSettingsCacheDirectory(QWidget* parent)
  : PreferencePage(parent)
  , ui(new Ui_DlgSettingsCacheDirectory)
{
    ui->setupUi(this);
    ui->labelCache->setToolTip(tr("Notify the user if the cache size exceeds the specified limit"));
    if (currentSize.isEmpty())
        currentSize = tr("Unknown");
    setCurrentCacheSize(currentSize);

    QString path = QString::fromStdString(App::Application::getUserCachePath());
    ui->cacheLocation->setText(path);

    ui->comboBoxLimit->addItem(QString::fromLatin1("100 MB"), 100);
    ui->comboBoxLimit->addItem(QString::fromLatin1("300 MB"), 300);
    ui->comboBoxLimit->addItem(QString::fromLatin1("500 MB"), 500);
    ui->comboBoxLimit->addItem(QString::fromLatin1("1 GB"), 1024);
    ui->comboBoxLimit->addItem(QString::fromLatin1("2 GB"), 2048);
    ui->comboBoxLimit->addItem(QString::fromLatin1("3 GB"), 3072);

    connect(ui->pushButtonCheck, &QPushButton::clicked, this, &DlgSettingsCacheDirectory::runCheck);
    connect(ui->openButton, &QPushButton::clicked, this, &DlgSettingsCacheDirectory::openDirectory);
}

DlgSettingsCacheDirectory::~DlgSettingsCacheDirectory() = default;

void DlgSettingsCacheDirectory::saveSettings()
{
    ApplicationCacheSettings::setCheckPeriod(ui->comboBoxPeriod->currentIndex());
    ApplicationCacheSettings::setCacheSizeLimit(ui->comboBoxLimit->currentData().toUInt());
}

void DlgSettingsCacheDirectory::loadSettings()
{
    int period = ApplicationCacheSettings::getCheckPeriod();
    if (period >= 0 && period < ui->comboBoxPeriod->count())
        ui->comboBoxPeriod->setCurrentIndex(period);
    unsigned int limit = ApplicationCacheSettings::getCacheSizeLimit();
    int index = ui->comboBoxLimit->findData(limit);

    // if not found then add a new item with this value
    if (index < 0) {
        ui->comboBoxLimit->addItem(QString::fromLatin1("%1 MB").arg(limit), limit);
        index = ui->comboBoxLimit->count() - 1;
    }
    ui->comboBoxLimit->setCurrentIndex(index);
}

void DlgSettingsCacheDirectory::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        int period = ui->comboBoxPeriod->currentIndex();
        ui->retranslateUi(this);
        ui->comboBoxPeriod->setCurrentIndex(period);
        setCurrentCacheSize(currentSize);
    }
    QWidget::changeEvent(e);
}

void DlgSettingsCacheDirectory::setCurrentCacheSize(const QString& str)
{
    currentSize = str;
    ui->labelCurrentCache->setText(tr("Current cache size: %1").arg(str));
}

void DlgSettingsCacheDirectory::runCheck()
{
    Gui::Dialog::ApplicationCache cache;
    unsigned int sizeInMB = ui->comboBoxLimit->currentData().toUInt();
    cache.setLimit(ApplicationCache::toBytes(sizeInMB));
    qint64 total = cache.size();
    setCurrentCacheSize(ApplicationCache::toString(total));

    // When performing the clean-up then recompute the new cache size
    if (cache.performAction(total)) {
        total = cache.size();
        setCurrentCacheSize(ApplicationCache::toString(total));
    }
}

void DlgSettingsCacheDirectory::openDirectory()
{
    QString path = QString::fromStdString(App::Application::getUserCachePath());
    QUrl url = QUrl::fromLocalFile(path);
    QDesktopServices::openUrl(url);
}

// ----------------------------------------------------------------------------

ApplicationCache::ApplicationCache()
{
    limit = std::pow(1024, 3);
    setPeriod(Period::Weekly);
}

/*!
 * \brief ApplicationCache::setPeriod
 * Set the period to check for the cache size
 * \param period
 */
void ApplicationCache::setPeriod(ApplicationCache::Period period)
{
    switch (period) {
    case Period::Always:
        numDays = -1;
        break;
    case Period::Daily:
        numDays = 1;
        break;
    case Period::Weekly:
        numDays = 7;
        break;
    case Period::Monthly:
        numDays = 31;
        break;
    case Period::Yearly:
        numDays = 365;
        break;
    case Period::Never:
        numDays = INT_MAX;
        break;
    }
}

/*!
 * \brief ApplicationCache::setLimit
 * Set the limit in bytes to perform a check
 * \param value
 */
void ApplicationCache::setLimit(qint64 value)
{
    limit = value;
}

/*!
 * \brief ApplicationCache::periodicCheckOfSize
 * Checks if the periodic check should be performed now
 * \return
 */
bool ApplicationCache::periodicCheckOfSize() const
{
    QString vendor = QString::fromLatin1(App::Application::Config()["ExeVendor"].c_str());
    QString application = QString::fromStdString(App::Application::getExecutableName());

    QSettings settings(vendor, application);
    QString key = QString::fromLatin1("LastCacheCheck");
    QDate date = settings.value(key).toDate();
    QDate now = QDate::currentDate();

    // get the days since the last check
    int days = date.daysTo(now);
    if (date.isNull()) {
        days = 1000;
    }

    if (days >= numDays) {
        settings.setValue(key, now);
        return true;
    }

    return false;
}

/*!
 * \brief ApplicationCache::performAction
 * If the cache size \a total is higher than the limit then show a dialog to the user
 * \param total
 */
bool ApplicationCache::performAction(qint64 total)
{
    bool performed = false;
    if (total > limit) {
        QString path = QString::fromStdString(App::Application::getUserCachePath());
        QMessageBox msgBox(Gui::getMainWindow());
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle(tr("Cache directory"));

        QString hint = tr("The cache directory %1 exceeds the size of %2.").arg(path, ApplicationCache::toString(limit));
        QString ask = tr("Do you want to clear it now?");
        QString warn = tr("Warning: Please make sure that this is the only running %1 instance "
                          "and that no documents are opened as this may result into data loss!").arg(QCoreApplication::applicationName());

        msgBox.setText(QString::fromLatin1("%1 %2\n\n\n%3").arg(hint, ask, warn));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Open);
        msgBox.setDefaultButton(QMessageBox::No);

        while (true) {
            int ret = msgBox.exec();
            if (ret == QMessageBox::Open) {
                QUrl url = QUrl::fromLocalFile(path);
                QDesktopServices::openUrl(url);
            }
            else {
                if (ret == QMessageBox::Yes) {
                    clearDirectory(path);
                    performed = true;
                }
                break;
            }
        }
    }

    return performed;
}

/*!
 * \brief ApplicationCache::size
 * Determines the size of the cache.
 * \return
 */
qint64 ApplicationCache::size() const
{
    qint64 total = dirSize(QString::fromStdString(App::Application::getUserCachePath()));
    return total;
}

/*!
 * \internal
 */
void ApplicationCache::clearDirectory(const QString& path)
{
    // Add the transient directories and the lock files to the ignore list
    QDir tmp = QString::fromUtf8(App::Application::getUserCachePath().c_str());
    tmp.setNameFilters(QStringList() << QString::fromLatin1("*.lock"));
    tmp.setFilter(QDir::Files);

    QList<QFileInfo> dirs;
    std::vector<App::Document*> docs = App::GetApplication().getDocuments();
    for (auto it : docs) {
        QDir dir(QString::fromStdString(it->TransientDir.getStrValue()));
        QFileInfo fi(dir.absolutePath());
        dirs.append(fi);
    }

    DocumentRecoveryCleaner cleaner;
    cleaner.setIgnoreFiles(tmp.entryList());
    cleaner.setIgnoreDirectories(dirs);
    cleaner.clearDirectory(QFileInfo(path));
}

/*!
 * \internal
 */
qint64 ApplicationCache::dirSize(QString dirPath) const
{
    qint64 total = 0;
    QDir dir(dirPath);

    QDir::Filters fileFilters = QDir::Files;
    const auto& files = dir.entryList(fileFilters);
    for (const QString& filePath : files) {
        QFileInfo fi(dir, filePath);
        total += fi.size();
    }

    // traverse sub-directories recursively
    QDir::Filters dirFilters = QDir::Dirs | QDir::NoDotAndDotDot;
    const auto& dirs = dir.entryList(dirFilters);
    for (const QString& subDirPath : dirs)
        total += dirSize(dirPath + QDir::separator() + subDirPath);
    return total;
}

/*!
 * \brief ApplicationCache::applyUserSettings
 * Set period and limit according to user settings
 */
void ApplicationCache::applyUserSettings()
{
    int period = ApplicationCacheSettings::getCheckPeriod();
    setPeriod(static_cast<Period>(period));

    unsigned int sizeInMB = ApplicationCacheSettings::getCacheSizeLimit();
    setLimit(ApplicationCache::toBytes(sizeInMB));
}

qint64 ApplicationCache::toBytes(unsigned int sizeInMB)
{
    qint64 value = static_cast<qint64>(sizeInMB) * 1024 * 1024;
    return value;
}

QString ApplicationCache::toString(qint64 size)
{
    QStringList units = {QString::fromLatin1("Bytes"),
                         QString::fromLatin1("KB"),
                         QString::fromLatin1("MB"),
                         QString::fromLatin1("GB")};
    int i;
    double outputSize = size;
    for (i=0; i<units.size()-1; i++) {
        if (outputSize < 1024)
            break;
        outputSize /= 1024;
    }

    return QString::fromLatin1("%1 %2").arg(QLocale().toString(outputSize, 'f', 2), units[i]);
}

// ----------------------------------------------------------------------------

unsigned int ApplicationCacheSettings::getCacheSizeLimit()
{
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("CacheDirectory");
    return hGrp->GetUnsigned("Limit", 500);
}

void ApplicationCacheSettings::setCacheSizeLimit(unsigned int limit)
{
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("CacheDirectory");
    hGrp->SetUnsigned("Limit", limit);
}

int ApplicationCacheSettings::getCheckPeriod()
{
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("CacheDirectory");
    return hGrp->GetInt("Period", static_cast<int>(ApplicationCache::Period::Weekly));
}

void ApplicationCacheSettings::setCheckPeriod(int period)
{
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("CacheDirectory");
    hGrp->SetInt("Period", period);
}


#include "moc_DlgSettingsCacheDirectory.cpp"
