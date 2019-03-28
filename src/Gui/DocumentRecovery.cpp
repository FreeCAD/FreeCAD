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
# include <QApplication>
# include <QCloseEvent>
# include <QDateTime>
# include <QDebug>
# include <QDir>
# include <QFile>
# include <QFileInfo>
# include <QHeaderView>
# include <QMenu>
# include <QMessageBox>
# include <QPushButton>
# include <QTextStream>
# include <QTreeWidgetItem>
# include <QMap>
# include <QList>
# include <QVector>
# include <sstream>
#endif

#include "DocumentRecovery.h"
#include "ui_DocumentRecovery.h"
#include "WaitCursor.h"

#include <Base/Exception.h>

#include <App/Application.h>
#include <App/Document.h>

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>

#include <QDomDocument>
#include <boost/interprocess/sync/file_lock.hpp>

using namespace Gui;
using namespace Gui::Dialog;

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
"		item=attributes.get(\"file\")\n"
"		if item != None:\n"
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
    typedef QMap<QString, QString> XmlConfig;

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
        Status status;
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
    d_ptr->ui.buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Start Recovery"));
#if QT_VERSION >= 0x050000
    d_ptr->ui.treeWidget->header()->setSectionResizeMode(QHeaderView::Stretch);
#else
    d_ptr->ui.treeWidget->header()->setResizeMode(QHeaderView::Stretch);
#endif

    d_ptr->recovered = false;

    for (QList<QFileInfo>::const_iterator it = dirs.begin(); it != dirs.end(); ++it) {
        DocumentRecoveryPrivate::Info info = d_ptr->getRecoveryInfo(*it);

        if (info.status == DocumentRecoveryPrivate::Created) {
            d_ptr->recoveryInfo << info;

            QTreeWidgetItem* item = new QTreeWidgetItem(d_ptr->ui.treeWidget);
            item->setText(0, info.label);
            item->setToolTip(0, info.tooltip);
            item->setText(1, tr("Not yet recovered"));
            item->setToolTip(1, info.projectFile);
            d_ptr->ui.treeWidget->addTopLevelItem(item);
        }
    }
}

DocumentRecovery::~DocumentRecovery()
{
}

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
    Q_D(DocumentRecovery);

    if (!d->recoveryInfo.isEmpty())
        e->ignore();
}

void DocumentRecovery::accept()
{
    Q_D(DocumentRecovery);

    if (!d->recovered) {

        WaitCursor wc;
        int index = -1;
        std::vector<int> indices;
        std::vector<std::string> filenames, pathes, labels, errs;
        for(auto &info : d->recoveryInfo) {
            ++index;
            std::string documentName;
            QString errorInfo;
            QTreeWidgetItem* item = d_ptr->ui.treeWidget->topLevelItem(index);

            try {
                QString file = info.projectFile;
                QFileInfo fi(file);
                if (fi.fileName() == QLatin1String("Document.xml"))
                    file = createProjectFile(info.projectFile);

                pathes.emplace_back(file.toUtf8().constData());
                filenames.emplace_back(info.fileName.toUtf8().constData());
                labels.emplace_back(info.label.toUtf8().constData());
                indices.push_back(index);
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

        auto docs = App::GetApplication().openDocuments(filenames,&pathes,&labels,&errs);

        for(int i=0;i<(int)docs.size();++i) {
            auto &info = d->recoveryInfo[indices[i]];
            QTreeWidgetItem* item = d_ptr->ui.treeWidget->topLevelItem(indices[i]);
            if(!docs[i] || errs[i].size()) {
                if(docs[i])
                    App::GetApplication().closeDocument(docs[i]->getName());
                info.status = DocumentRecoveryPrivate::Failure;
                if (item) {
                    item->setText(1, tr("Failed to recover"));
                    item->setToolTip(1, QString::fromUtf8(errs[index].c_str()));
                    item->setForeground(1, QColor(170,0,0));
                }
            }else{
                info.status = DocumentRecoveryPrivate::Success;
                if (item) {
                    item->setText(1, tr("Successfully recovered"));
                    item->setForeground(1, QColor(0,170,0));
                }
            }

            // write back current status
            d->writeRecoveryInfo(info);
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
        str.setCodec("UTF-8");
        str << "<?xml version='1.0' encoding='utf-8'?>" << endl
            << "<AutoRecovery SchemaVersion=\"1\">" << endl;
        switch (info.status) {
        case Created:
            str << "  <Status>Created</Status>" << endl;
            break;
        case Overage:
            str << "  <Status>Deprecated</Status>" << endl;
            break;
        case Success:
            str << "  <Status>Success</Status>" << endl;
            break;
        case Failure:
            str << "  <Status>Failure</Status>" << endl;
            break;
        default:
            str << "  <Status>Unknown</Status>" << endl;
            break;
        }
        str << "  <Label>" << info.label << "</Label>" << endl;
        str << "  <FileName>" << info.fileName << "</FileName>" << endl;
        str << "</AutoRecovery>" << endl;
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
        menu.addAction(tr("Delete"), this, SLOT(onDeleteSection()));
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
    QDir tmp = QString::fromUtf8(App::Application::getTempPath().c_str());
    for (QList<QTreeWidgetItem*>::iterator it = items.begin(); it != items.end(); ++it) {
        int index = d_ptr->ui.treeWidget->indexOfTopLevelItem(*it);
        QTreeWidgetItem* item = d_ptr->ui.treeWidget->takeTopLevelItem(index);

        QString projectFile = item->toolTip(0);
        clearDirectory(QFileInfo(tmp.filePath(projectFile)));
        tmp.rmdir(projectFile);
        delete item;
    }

    int numItems = d_ptr->ui.treeWidget->topLevelItemCount();
    if (numItems == 0) {
        d_ptr->ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        d_ptr->ui.buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(true);
    }
}

void DocumentRecovery::on_buttonCleanup_clicked()
{
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowTitle(tr("Cleanup"));
    msgBox.setText(tr("Are you sure you want to delete all transient directories?"));
    msgBox.setInformativeText(tr("When deleting all transient directory you won't be able to recover any files afterwards."));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    int ret = msgBox.exec();
    if (ret == QMessageBox::No)
        return;

    d_ptr->ui.treeWidget->clear();
    d_ptr->ui.buttonCleanup->setEnabled(false);
    d_ptr->ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    d_ptr->ui.buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(true);

    QDir tmp = QString::fromUtf8(App::Application::getTempPath().c_str());
    tmp.setNameFilters(QStringList() << QString::fromLatin1("*.lock"));
    tmp.setFilter(QDir::Files);

    QList<QFileInfo> restoreDocFiles;
    QString exeName = QString::fromLatin1(App::GetApplication().getExecutableName());
    QList<QFileInfo> locks = tmp.entryInfoList();
    for (QList<QFileInfo>::iterator it = locks.begin(); it != locks.end(); ++it) {
        QString bn = it->baseName();
        // ignore the lock file for this instance
        QString pid = QString::number(QCoreApplication::applicationPid());
        if (bn.startsWith(exeName) && bn.indexOf(pid) < 0) {
            QString fn = it->absoluteFilePath();
            boost::interprocess::file_lock flock((const char*)fn.toLocal8Bit());
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
                if (!dirs.isEmpty()) {
                    for (QList<QFileInfo>::iterator jt = dirs.begin(); jt != dirs.end(); ++jt) {
                        clearDirectory(*jt);
                        tmp.rmdir(jt->fileName());
                    }
                }
                tmp.remove(it->fileName());
            }
        }
    }

    QMessageBox::information(this, tr("Finished"), tr("Transient directories deleted."));
}

void DocumentRecovery::clearDirectory(const QFileInfo& dir)
{
    QDir qThisDir(dir.absoluteFilePath());
    if (!qThisDir.exists())
        return;

    // Remove all files in this directory
    qThisDir.setFilter(QDir::Files);
    QStringList files = qThisDir.entryList();
    for (QStringList::iterator it = files.begin(); it != files.end(); ++it) {
        QString file = *it;
        qThisDir.remove(file);
    }

    // Clear this directory of any sub-directories
    qThisDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    QFileInfoList subdirs = qThisDir.entryInfoList();
    for (QFileInfoList::iterator it = subdirs.begin(); it != subdirs.end(); ++it) {
        clearDirectory(*it);
        qThisDir.rmdir(it->fileName());
    }
}

#include "moc_DocumentRecovery.cpp"
