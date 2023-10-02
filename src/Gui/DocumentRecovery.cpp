/***************************************************************************
 *   Copyright (c) 2015 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

// Implement FileWriter which puts files into a directory
// write a property to file only when it has been modified
// implement xml meta file

#include "PreCompiled.h"

#ifndef _PreComp_
# include <boost/interprocess/sync/file_lock.hpp>
# include <QApplication>
# include <QCloseEvent>
# include <QDateTime>
# include <QDebug>
# include <QDir>
# include <QDomDocument>
# include <QFileInfo>
# include <QHeaderView>
# include <QList>
# include <QMap>
# include <QMenu>
# include <QMessageBox>
# include <QSet>
# include <QTextStream>
# include <QTreeWidgetItem>
# include <QVector>
# include <sstream>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Exception.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/DlgCheckableMessageBox.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>

#include "DocumentRecovery.h"
#include "ui_DocumentRecovery.h"
#include "WaitCursor.h"


FC_LOG_LEVEL_INIT("Gui", true, true)

using namespace Gui;
using namespace Gui::Dialog;
namespace sp = std::placeholders;

// taken from the script doctools.py
std::string DocumentRecovery::doctools =
"import os,sys,string\n"
"import xml.sax\n"
"import xml.sax.handler\n"
"import xml.sax.xmlreader\n"
"import zipfile\n"
"\n"
"# SAX handler to parse the Document.xml\n"
"class DocumentHandler(xml.sax.handler.ContentHandler):\n"
"	def __init__(self, dirname):\n"
"		self.files = []\n"
"		self.dirname = dirname\n"
"\n"
"	def startElement(self, name, attributes):\n"
"		if name == 'XLink':\n"
"			return\n"
"		item=attributes.get(\"file\")\n"
"		if item:\n"
"			self.files.append(os.path.join(self.dirname,str(item)))\n"
"\n"
"	def characters(self, data):\n"
"		return\n"
"\n"
"	def endElement(self, name):\n"
"		return\n"
"\n"
"def extractDocument(filename, outpath):\n"
"	zfile=zipfile.ZipFile(filename)\n"
"	files=zfile.namelist()\n"
"\n"
"	for i in files:\n"
"		data=zfile.read(i)\n"
"		dirs=i.split(\"/\")\n"
"		if len(dirs) > 1:\n"
"			dirs.pop()\n"
"			curpath=outpath\n"
"			for j in dirs:\n"
"				curpath=curpath+\"/\"+j\n"
"				os.mkdir(curpath)\n"
"		output=open(outpath+\"/\"+i,\'wb\')\n"
"		output.write(data)\n"
"		output.close()\n"
"\n"
"def createDocument(filename, outpath):\n"
"	files=getFilesList(filename)\n"
"	dirname=os.path.dirname(filename)\n"
"	guixml=os.path.join(dirname,\"GuiDocument.xml\")\n"
"	if os.path.exists(guixml):\n"
"		files.extend(getFilesList(guixml))\n"
"	compress=zipfile.ZipFile(outpath,\'w\',zipfile.ZIP_DEFLATED)\n"
"	for i in files:\n"
"		dirs=os.path.split(i)\n"
"		#print i, dirs[-1]\n"
"		compress.write(i,dirs[-1],zipfile.ZIP_DEFLATED)\n"
"	compress.close()\n"
"\n"
"def getFilesList(filename):\n"
"	dirname=os.path.dirname(filename)\n"
"	handler=DocumentHandler(dirname)\n"
"	parser=xml.sax.make_parser()\n"
"	parser.setContentHandler(handler)\n"
"	parser.parse(filename)\n"
"\n"
"	files=[]\n"
"	files.append(filename)\n"
"	files.extend(iter(handler.files))\n"
"	return files\n"
;


namespace Gui { namespace Dialog {
class DocumentRecoveryPrivate
{
public:
    using XmlConfig = QMap<QString, QString>;

    enum Status {
        Unknown = 0, /*!< The file is not available */
        Created = 1, /*!< The file was created but not processed so far*/
        Overage = 2, /*!< The recovery file is older than the actual project file */
        Success = 3, /*!< The file could be recovered */
        Failure = 4, /*!< The file could not be recovered */
    };
    struct Info {
        QString projectFile;
        QString xmlFile;
        QString label;
        QString fileName;
        QString tooltip;
        Status status = Unknown;
    };
    Ui_DocumentRecovery ui;
    bool recovered;
    QList<Info> recoveryInfo;

    Info getRecoveryInfo(const QFileInfo&) const;
    void writeRecoveryInfo(const Info&) const;
    XmlConfig readXmlFile(const QString& fn) const;
};

}
}

DocumentRecovery::DocumentRecovery(const QList<QFileInfo>& dirs, QWidget* parent)
  : QDialog(parent), d_ptr(new DocumentRecoveryPrivate())
{
    d_ptr->ui.setupUi(this);
    connect(d_ptr->ui.buttonCleanup, &QPushButton::clicked,
            this, &DocumentRecovery::onButtonCleanupClicked);
    d_ptr->ui.buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Start Recovery"));
    d_ptr->ui.treeWidget->header()->setSectionResizeMode(QHeaderView::Stretch);

    d_ptr->recovered = false;

    for (QList<QFileInfo>::const_iterator it = dirs.begin(); it != dirs.end(); ++it) {
        DocumentRecoveryPrivate::Info info = d_ptr->getRecoveryInfo(*it);

        if (info.status == DocumentRecoveryPrivate::Created) {
            d_ptr->recoveryInfo << info;

            auto item = new QTreeWidgetItem(d_ptr->ui.treeWidget);
            item->setText(0, info.label);
            item->setToolTip(0, info.tooltip);
            item->setText(1, tr("Not yet recovered"));
            item->setToolTip(1, info.projectFile);
            d_ptr->ui.treeWidget->addTopLevelItem(item);
        }
    }

    this->adjustSize();
}

DocumentRecovery::~DocumentRecovery() = default;

bool DocumentRecovery::foundDocuments() const
{
    Q_D(const DocumentRecovery);
    return (!d->recoveryInfo.isEmpty());
}

QString DocumentRecovery::createProjectFile(const QString& documentXml)
{
    QString source = documentXml;
    QFileInfo fi(source);
    QString dest = fi.dir().absoluteFilePath(QString::fromLatin1("fc_recovery_file.fcstd"));

    std::stringstream str;
    str << doctools << "\n";
    str << "createDocument(\"" << (const char*)source.toUtf8()
        << "\", \"" << (const char*)dest.toUtf8() << "\")";
    Gui::Command::runCommand(Gui::Command::App, str.str().c_str());

    return dest;
}

void DocumentRecovery::closeEvent(QCloseEvent* e)
{
    // Do not disable the X button in the title bar
    // #0004281: Close Document Recovery
    e->accept();
}

void DocumentRecovery::accept()
{
    Q_D(DocumentRecovery);

    if (!d->recovered) {

        WaitCursor wc;
        int index = 0;
        std::vector<int> indices;
        std::vector<std::string> filenames, paths, labels, errs;
        for (auto &info : d->recoveryInfo) {
            QString errorInfo;
            QTreeWidgetItem* item = d_ptr->ui.treeWidget->topLevelItem(index);

            try {
                QString file = info.projectFile;
                QFileInfo fi(file);
                if (fi.fileName() == QLatin1String("Document.xml"))
                    file = createProjectFile(info.projectFile);

                paths.emplace_back(file.toUtf8().constData());
                filenames.emplace_back(info.fileName.toUtf8().constData());
                labels.emplace_back(info.label.toUtf8().constData());
                indices.push_back(index);
                ++index;
            }
            catch (const std::exception& e) {
                errorInfo = QString::fromLatin1(e.what());
            }
            catch (const Base::Exception& e) {
                errorInfo = QString::fromLatin1(e.what());
            }
            catch (...) {
                errorInfo = tr("Unknown problem occurred");
            }

            if (!errorInfo.isEmpty()) {
                info.status = DocumentRecoveryPrivate::Failure;
                if (item) {
                    item->setText(1, tr("Failed to recover"));
                    item->setToolTip(1, errorInfo);
                    item->setForeground(1, QColor(170,0,0));
                }
                d->writeRecoveryInfo(info);
            }
        }

        auto docs = App::GetApplication().openDocuments(filenames,&paths,&labels,&errs);

        for (size_t i = 0; i < docs.size(); ++i) {
            auto &info = d->recoveryInfo[indices[i]];
            QTreeWidgetItem* item = d_ptr->ui.treeWidget->topLevelItem(indices[i]);
            if (!docs[i] || !errs[i].empty()) {
                if (docs[i])
                    App::GetApplication().closeDocument(docs[i]->getName());
                info.status = DocumentRecoveryPrivate::Failure;

                if (item) {
                    item->setText(1, tr("Failed to recover"));
                    item->setToolTip(1, QString::fromUtf8(errs[i].c_str()));
                    item->setForeground(1, QColor(170,0,0));
                }
                // write back current status
                d->writeRecoveryInfo(info);
            }
            else {
                auto gdoc = Application::Instance->getDocument(docs[i]);
                if (gdoc)
                    gdoc->setModified(true);

                info.status = DocumentRecoveryPrivate::Success;
                if (item) {
                    item->setText(1, tr("Successfully recovered"));
                    item->setForeground(1, QColor(0,170,0));
                }

                QDir transDir(QString::fromUtf8(docs[i]->TransientDir.getValue()));

                QFileInfo xfi(info.xmlFile);
                QFileInfo fi(info.projectFile);
                bool res = false;

                if (fi.fileName() == QLatin1String("fc_recovery_file.fcstd")) {
                    transDir.remove(fi.fileName());
                    res = transDir.rename(fi.absoluteFilePath(),fi.fileName());
                }
                else {
                    transDir.rmdir(fi.dir().dirName());
                    res = transDir.rename(fi.absolutePath(),fi.dir().dirName());
                }

                if (res) {
                    transDir.remove(xfi.fileName());
                    res = transDir.rename(xfi.absoluteFilePath(),xfi.fileName());
                }

                if (!res) {
                    FC_WARN("Failed to move recovery file of document '"
                            << docs[i]->Label.getValue() << "'");
                }
                else {
                    DocumentRecoveryCleaner().clearDirectory(QFileInfo(xfi.absolutePath()));
                    QDir().rmdir(xfi.absolutePath());
                }

                // DO NOT write success into recovery info, in case the program
                // crash again before the user save the just recovered file.
            }
        }

        d->ui.buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Finish"));
        d->ui.buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(false);
        d->recovered = true;
    }
    else {
        QDialog::accept();
    }
}

void DocumentRecoveryPrivate::writeRecoveryInfo(const DocumentRecoveryPrivate::Info& info) const
{
    // Write recovery meta file
    QFile file(info.xmlFile);
    if (file.open(QFile::WriteOnly)) {
        QTextStream str(&file);
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        str.setCodec("UTF-8");
#endif
        str << "<?xml version='1.0' encoding='utf-8'?>\n"
            << "<AutoRecovery SchemaVersion=\"1\">\n";
        switch (info.status) {
        case Created:
            str << "  <Status>Created</Status>\n";
            break;
        case Overage:
            str << "  <Status>Deprecated</Status>\n";
            break;
        case Success:
            str << "  <Status>Success</Status>\n";
            break;
        case Failure:
            str << "  <Status>Failure</Status>\n";
            break;
        default:
            str << "  <Status>Unknown</Status>\n";
            break;
        }
        str << "  <Label>" << info.label << "</Label>\n";
        str << "  <FileName>" << info.fileName << "</FileName>\n";
        str << "</AutoRecovery>\n";
        file.close();
    }
}

DocumentRecoveryPrivate::Info DocumentRecoveryPrivate::getRecoveryInfo(const QFileInfo& fi) const
{
    DocumentRecoveryPrivate::Info info;
    info.status = DocumentRecoveryPrivate::Unknown;
    info.label = qApp->translate("StdCmdNew","Unnamed");

    QString file;
    QDir doc_dir(fi.absoluteFilePath());
    QDir rec_dir(doc_dir.absoluteFilePath(QLatin1String("fc_recovery_files")));

    // compressed recovery file
    if (doc_dir.exists(QLatin1String("fc_recovery_file.fcstd"))) {
        file = doc_dir.absoluteFilePath(QLatin1String("fc_recovery_file.fcstd"));
    }
    // separate files for recovery
    else if (rec_dir.exists(QLatin1String("Document.xml"))) {
        file = rec_dir.absoluteFilePath(QLatin1String("Document.xml"));
    }

    info.status = DocumentRecoveryPrivate::Created;
    info.projectFile = file;
    info.tooltip = fi.fileName();

    // when the Xml meta exists get some relevant information
    info.xmlFile = doc_dir.absoluteFilePath(QLatin1String("fc_recovery_file.xml"));
    if (doc_dir.exists(QLatin1String("fc_recovery_file.xml"))) {
        XmlConfig cfg = readXmlFile(info.xmlFile);

        if (cfg.contains(QString::fromLatin1("Label"))) {
            info.label = cfg[QString::fromLatin1("Label")];
        }

        if (cfg.contains(QString::fromLatin1("FileName"))) {
            info.fileName = cfg[QString::fromLatin1("FileName")];
        }

        if (cfg.contains(QString::fromLatin1("Status"))) {
            QString status = cfg[QString::fromLatin1("Status")];
            if (status == QLatin1String("Deprecated"))
                info.status = DocumentRecoveryPrivate::Overage;
            else if (status == QLatin1String("Success"))
                info.status = DocumentRecoveryPrivate::Success;
            else if (status == QLatin1String("Failure"))
                info.status = DocumentRecoveryPrivate::Failure;
        }

        if (info.status == DocumentRecoveryPrivate::Created) {
            // compare the modification dates
            QFileInfo fileInfo(info.fileName);
            if (!info.fileName.isEmpty() && fileInfo.exists()) {
                QDateTime dateRecv = QFileInfo(file).lastModified();
                QDateTime dateProj = fileInfo.lastModified();
                if (dateRecv < dateProj) {
                    info.status = DocumentRecoveryPrivate::Overage;
                    writeRecoveryInfo(info);
                    qWarning() << "Ignore recovery file " << file.toUtf8()
                        << " because it is older than the project file" << info.fileName.toUtf8() << "\n";
                }
            }
        }
    }

    return info;
}

DocumentRecoveryPrivate::XmlConfig DocumentRecoveryPrivate::readXmlFile(const QString& fn) const
{
    DocumentRecoveryPrivate::XmlConfig cfg;
    QDomDocument domDocument;
    QFile file(fn);
    if (!file.open(QFile::ReadOnly))
        return cfg;

    QString errorStr;
    int errorLine;
    int errorColumn;

    if (!domDocument.setContent(&file, true, &errorStr, &errorLine,
                                &errorColumn)) {
        return cfg;
    }

    QDomElement root = domDocument.documentElement();
    if (root.tagName() != QLatin1String("AutoRecovery")) {
        return cfg;
    }

    file.close();

    QVector<QString> filter;
    filter << QString::fromLatin1("Label");
    filter << QString::fromLatin1("FileName");
    filter << QString::fromLatin1("Status");

    QDomElement child;
    if (!root.isNull()) {
        child = root.firstChildElement();
        while (!child.isNull()) {
            QString name = child.localName();
            QString value = child.text();
            if (std::find(filter.begin(), filter.end(), name) != filter.end())
                cfg[name] = value;
            child = child.nextSiblingElement();
        }
    }

    return cfg;
}

void DocumentRecovery::contextMenuEvent(QContextMenuEvent* ev)
{
    QList<QTreeWidgetItem*> items = d_ptr->ui.treeWidget->selectedItems();
    if (!items.isEmpty()) {
        QMenu menu;
        menu.addAction(tr("Delete"), this, &DocumentRecovery::onDeleteSection);
        menu.exec(ev->globalPos());
    }
}

void DocumentRecovery::onDeleteSection()
{
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowTitle(tr("Cleanup"));
    msgBox.setText(tr("Are you sure you want to delete the selected transient directories?"));
    msgBox.setInformativeText(tr("When deleting the selected transient directory you won't be able to recover any files afterwards."));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    int ret = msgBox.exec();
    if (ret == QMessageBox::No)
        return;

    QList<QTreeWidgetItem*> items = d_ptr->ui.treeWidget->selectedItems();
    QDir tmp = QString::fromUtf8(App::Application::getUserCachePath().c_str());
    for (QList<QTreeWidgetItem*>::iterator it = items.begin(); it != items.end(); ++it) {
        int index = d_ptr->ui.treeWidget->indexOfTopLevelItem(*it);
        QTreeWidgetItem* item = d_ptr->ui.treeWidget->takeTopLevelItem(index);

        QString projectFile = item->toolTip(0);
        DocumentRecoveryCleaner().clearDirectory(QFileInfo(tmp.filePath(projectFile)));
        tmp.rmdir(projectFile);
        delete item;
    }

    int numItems = d_ptr->ui.treeWidget->topLevelItemCount();
    if (numItems == 0) {
        d_ptr->ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        d_ptr->ui.buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(true);
    }
}

void DocumentRecovery::onButtonCleanupClicked()
{
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowTitle(tr("Cleanup"));
    msgBox.setText(tr("Are you sure you want to delete all transient directories?"));
    msgBox.setInformativeText(tr("When deleting all transient directories you won't be able to recover any files afterwards."));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    int ret = msgBox.exec();
    if (ret == QMessageBox::No)
        return;

    d_ptr->ui.treeWidget->clear();
    d_ptr->ui.buttonCleanup->setEnabled(false);
    d_ptr->ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    d_ptr->ui.buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(true);

    DocumentRecoveryHandler handler;
    handler.checkForPreviousCrashes(std::bind(&DocumentRecovery::cleanup, this, sp::_1, sp::_2, sp::_3));
    DlgCheckableMessageBox::showMessage(tr("Delete"), tr("Transient directories deleted."));
    reject();
}

void DocumentRecovery::cleanup(QDir& tmp, const QList<QFileInfo>& dirs, const QString& lockFile)
{
    if (!dirs.isEmpty()) {
        for (QList<QFileInfo>::const_iterator jt = dirs.cbegin(); jt != dirs.cend(); ++jt) {
            DocumentRecoveryCleaner().clearDirectory(*jt);
            tmp.rmdir(jt->fileName());
        }
    }
    tmp.remove(lockFile);
}

// ----------------------------------------------------------------------------

bool DocumentRecoveryFinder::checkForPreviousCrashes()
{
    //NOLINTBEGIN
    DocumentRecoveryHandler handler;
    handler.checkForPreviousCrashes(std::bind(&DocumentRecoveryFinder::checkDocumentDirs, this, sp::_1, sp::_2, sp::_3));
    //NOLINTEND

    return showRecoveryDialogIfNeeded();
}

void DocumentRecoveryFinder::checkDocumentDirs(QDir& tmp, const QList<QFileInfo>& dirs, const QString& fn)
{
    if (dirs.isEmpty()) {
        // delete the lock file immediately if no transient directories are related
        tmp.remove(fn);
    }
    else {
        int countDeletedDocs = 0;
        QString recovery_files = QString::fromLatin1("fc_recovery_files");
        for (QList<QFileInfo>::const_iterator it = dirs.cbegin(); it != dirs.cend(); ++it) {
            QDir doc_dir(it->absoluteFilePath());
            doc_dir.setFilter(QDir::NoDotAndDotDot|QDir::AllEntries);
            uint entries = doc_dir.entryList().count();
            if (entries == 0) {
                // in this case we can delete the transient directory because
                // we cannot do anything
                if (tmp.rmdir(it->filePath()))
                    countDeletedDocs++;
            }
            // search for the existence of a recovery file
            else if (doc_dir.exists(QLatin1String("fc_recovery_file.xml"))) {
                // store the transient directory in case it's not empty
                restoreDocFiles << *it;
            }
            // search for the 'fc_recovery_files' sub-directory and check that it's the only entry
            else if (entries == 1 && doc_dir.exists(recovery_files)) {
                // if the sub-directory is empty delete the transient directory
                QDir rec_dir(doc_dir.absoluteFilePath(recovery_files));
                rec_dir.setFilter(QDir::NoDotAndDotDot|QDir::AllEntries);
                if (rec_dir.entryList().isEmpty()) {
                    doc_dir.rmdir(recovery_files);
                    if (tmp.rmdir(it->filePath()))
                        countDeletedDocs++;
                }
            }
        }

        // all directories corresponding to the lock file have been deleted
        // so delete the lock file, too
        if (countDeletedDocs == dirs.size()) {
            tmp.remove(fn);
        }
    }
}

bool DocumentRecoveryFinder::showRecoveryDialogIfNeeded()
{
    bool foundRecoveryFiles = false;
    if (!restoreDocFiles.isEmpty()) {
        Gui::Dialog::DocumentRecovery dlg(restoreDocFiles, Gui::getMainWindow());
        if (dlg.foundDocuments()) {
            foundRecoveryFiles = true;
            dlg.exec();
        }
    }

    return foundRecoveryFiles;
}

// ----------------------------------------------------------------------------

void DocumentRecoveryHandler::checkForPreviousCrashes(const std::function<void(QDir&, const QList<QFileInfo>&, const QString&)> & callableFunc) const
{
    QDir tmp = QString::fromUtf8(App::Application::getUserCachePath().c_str());
    tmp.setNameFilters(QStringList() << QString::fromLatin1("*.lock"));
    tmp.setFilter(QDir::Files);

    QString exeName = QString::fromStdString(App::Application::getExecutableName());
    QList<QFileInfo> locks = tmp.entryInfoList();
    for (QList<QFileInfo>::iterator it = locks.begin(); it != locks.end(); ++it) {
        QString bn = it->baseName();
        // ignore the lock file for this instance
        QString pid = QString::number(QCoreApplication::applicationPid());
        if (bn.startsWith(exeName) && bn.indexOf(pid) < 0) {
            QString fn = it->absoluteFilePath();

#if !defined(FC_OS_WIN32) || (BOOST_VERSION < 107600)
            boost::interprocess::file_lock flock(fn.toUtf8());
#else
            boost::interprocess::file_lock flock(fn.toStdWString().c_str());
#endif
            if (flock.try_lock()) {
                // OK, this file is a leftover from a previous crash
                QString crashed_pid = bn.mid(exeName.length()+1);
                // search for transient directories with this PID
                QString filter;
                QTextStream str(&filter);
                str << exeName << "_Doc_*_" << crashed_pid;
                tmp.setNameFilters(QStringList() << filter);
                tmp.setFilter(QDir::Dirs);
                QList<QFileInfo> dirs = tmp.entryInfoList();

                callableFunc(tmp, dirs, it->fileName());
            }
        }
    }
}

// ----------------------------------------------------------------------------

void DocumentRecoveryCleaner::clearDirectory(const QFileInfo& dir)
{
    QDir qThisDir(dir.absoluteFilePath());
    if (!qThisDir.exists())
        return;

    // Remove all files in this directory
    qThisDir.setFilter(QDir::Files);
    QStringList files = qThisDir.entryList();
    subtractFiles(files);
    for (QStringList::iterator it = files.begin(); it != files.end(); ++it) {
        QString file = *it;
        qThisDir.remove(file);
    }

    // Clear this directory of any sub-directories
    qThisDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    QFileInfoList subdirs = qThisDir.entryInfoList();
    subtractDirs(subdirs);
    for (QFileInfoList::iterator it = subdirs.begin(); it != subdirs.end(); ++it) {
        clearDirectory(*it);
        qThisDir.rmdir(it->fileName());
    }
}

void DocumentRecoveryCleaner::subtractFiles(QStringList& files)
{
    if (!ignoreFiles.isEmpty() && !files.isEmpty()) {
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
        auto set1 = QSet<QString>(files.begin(), files.end());
        auto set2 = QSet<QString>(ignoreFiles.begin(), ignoreFiles.end());
        set1.subtract(set2);
        files = QList<QString>(set1.begin(), set1.end());
#else
        QSet<QString> set1 = files.toSet();
        QSet<QString> set2 = ignoreFiles.toSet();
        set1.subtract(set2);
        files = set1.toList();
#endif
    }
}

void DocumentRecoveryCleaner::subtractDirs(QFileInfoList& dirs)
{
    if (!ignoreDirs.isEmpty() && !dirs.isEmpty()) {
        for (const auto& it : qAsConst(ignoreDirs)) {
            dirs.removeOne(it);
        }
    }
}

void DocumentRecoveryCleaner::setIgnoreFiles(const QStringList& list)
{
    ignoreFiles = list;
}

void DocumentRecoveryCleaner::setIgnoreDirectories(const QFileInfoList& list)
{
    ignoreDirs = list;
}

#include "moc_DocumentRecovery.cpp"
