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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <QApplication>
# include <QFile>
# include <QDir>
# include <QRunnable>
# include <QTextStream>
# include <QThreadPool>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>
#include <Base/Tools.h>
#include <Base/Writer.h>

#include "AutoSaver.h"
#include "Document.h"
#include "MainWindow.h"
#include "ViewProvider.h"
#include "WaitCursor.h"

FC_LOG_LEVEL_INIT("App",true,true)

using namespace Gui;
namespace sp = std::placeholders;

AutoSaver* AutoSaver::self = nullptr;
const int AutoSaveTimeout = 900000;

AutoSaver::AutoSaver(QObject* parent)
  : QObject(parent)
  , timeout(AutoSaveTimeout)
  , compressed(true)
{
    //NOLINTBEGIN
    App::GetApplication().signalNewDocument.connect(std::bind(&AutoSaver::slotCreateDocument, this, sp::_1));
    App::GetApplication().signalDeleteDocument.connect(std::bind(&AutoSaver::slotDeleteDocument, this, sp::_1));
    //NOLINTEND
}

AutoSaver::~AutoSaver() = default;

AutoSaver* AutoSaver::instance()
{
    if (!self) {
        self = new AutoSaver(QApplication::instance());
    }
    return self;
}

void AutoSaver::renameFile(QString dirName, QString file, QString tmpFile)
{
    FC_LOG("auto saver rename " << tmpFile.toUtf8().constData()
            << " -> " << file.toUtf8().constData());
    QDir dir(dirName);
    dir.remove(file);
    dir.rename(tmpFile,file);
}

void AutoSaver::setTimeout(int ms)
{
    timeout = Base::clamp<int>(ms, 0, 3600000); // between 0 and 60 min

    // go through the attached documents and apply the new timeout
    for (auto & it : saverMap) {
        if (it.second->timerId > 0)
            killTimer(it.second->timerId);
        int id = timeout > 0 ? startTimer(timeout) : 0;
        it.second->timerId = id;
    }
}

void AutoSaver::setCompressed(bool on)
{
    this->compressed = on;
}

void AutoSaver::slotCreateDocument(const App::Document& Doc)
{
    std::string name = Doc.getName();
    int id = timeout > 0 ? startTimer(timeout) : 0;
    AutoSaveProperty* as = new AutoSaveProperty(&Doc);
    as->timerId = id;

    if (!this->compressed) {
        std::string dirName = Doc.TransientDir.getValue();
        dirName += "/fc_recovery_files";
        Base::FileInfo fi(dirName);
        fi.createDirectory();
        as->dirName = dirName;
    }
    saverMap.insert(std::make_pair(name, as));
}

void AutoSaver::slotDeleteDocument(const App::Document& Doc)
{
    std::string name = Doc.getName();
    std::map<std::string, AutoSaveProperty*>::iterator it = saverMap.find(name);
    if (it != saverMap.end()) {
        if (it->second->timerId > 0)
            killTimer(it->second->timerId);
        delete it->second;
        saverMap.erase(it);
    }
}

void AutoSaver::saveDocument(const std::string& name, AutoSaveProperty& saver)
{
    Gui::WaitCursor wc;
    App::Document* doc = App::GetApplication().getDocument(name.c_str());
    if (doc && !doc->testStatus(App::Document::PartialDoc)
            && !doc->testStatus(App::Document::TempDoc))
    {
        // Set the document's current transient directory
        std::string dirName = doc->TransientDir.getValue();
        dirName += "/fc_recovery_files";
        saver.dirName = dirName;

        // Write recovery meta file
        QFile file(QString::fromLatin1("%1/fc_recovery_file.xml")
            .arg(QString::fromUtf8(doc->TransientDir.getValue())));
        if (file.open(QFile::WriteOnly)) {
            QTextStream str(&file);
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
            str.setCodec("UTF-8");
#endif
            str << "<?xml version='1.0' encoding='utf-8'?>\n"
                << "<AutoRecovery SchemaVersion=\"1\">\n";
            str << "  <Status>Created</Status>\n";
            str << "  <Label>" << QString::fromUtf8(doc->Label.getValue()) << "</Label>\n"; // store the document's current label
            str << "  <FileName>" << QString::fromUtf8(doc->FileName.getValue()) << "</FileName>\n"; // store the document's current filename
            str << "</AutoRecovery>\n";
            file.close();
        }

        // make sure to tmp. disable saving thumbnails because this causes trouble if the
        // associated 3d view is not active
        Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetParameterGroupByPath
            ("User parameter:BaseApp/Preferences/Document");
        bool save = hGrp->GetBool("SaveThumbnail",true);
        hGrp->SetBool("SaveThumbnail",false);

        getMainWindow()->showMessage(tr("Please wait until the AutoRecovery file has been saved..."), 5000);
        //qApp->processEvents();

        // open extra scope to close ZipWriter properly
        Base::StopWatch watch;
        watch.start();
        {
            if (!this->compressed) {
                RecoveryWriter writer(saver);

                // We will be using thread pool if not compressed.
                // So, always force binary format because ASCII
                // is not reentrant. See PropertyPartShape::SaveDocFile
                writer.setMode("BinaryBrep");

                writer.putNextEntry("Document.xml");

                doc->Save(writer);

                // Special handling for Gui document.
                doc->signalSaveDocument(writer);

                // write additional files
                writer.writeFiles();
            }
            // only create the file if something has changed
            else if (!saver.touched.empty()) {
                std::string fn = doc->TransientDir.getValue();
                fn += "/fc_recovery_file.fcstd";
                Base::FileInfo tmp(fn);
                Base::ofstream file(tmp, std::ios::out | std::ios::binary);
                if (file.is_open())
                {
                    Base::ZipWriter writer(file);
                    if (hGrp->GetBool("SaveBinaryBrep", true))
                        writer.setMode("BinaryBrep");

                    writer.setComment("AutoRecovery file");
                    writer.setLevel(1); // apparently the fastest compression
                    writer.putNextEntry("Document.xml");

                    doc->Save(writer);

                    // Special handling for Gui document.
                    doc->signalSaveDocument(writer);

                    // write additional files
                    writer.writeFiles();
                }
            }
        }

        std::string str = watch.toString(watch.elapsed());
        Base::Console().Log("Save AutoRecovery file: %s\n", str.c_str());
        hGrp->SetBool("SaveThumbnail",save);
    }
}

void AutoSaver::timerEvent(QTimerEvent * event)
{
    int id = event->timerId();
    for (auto & it : saverMap) {
        if (it.second->timerId == id) {
            try {
                saveDocument(it.first, *it.second);
                it.second->touched.clear();
                break;
            }
            catch (...) {
                Base::Console().Error("Failed to auto-save document '%s'\n", it.first.c_str());
            }
        }
    }
}

// ----------------------------------------------------------------------------

AutoSaveProperty::AutoSaveProperty(const App::Document* doc) : timerId(-1)
{
    //NOLINTBEGIN
    documentNew = const_cast<App::Document*>(doc)->signalNewObject.connect
        (std::bind(&AutoSaveProperty::slotNewObject, this, sp::_1));
    documentMod = const_cast<App::Document*>(doc)->signalChangedObject.connect
        (std::bind(&AutoSaveProperty::slotChangePropertyData, this, sp::_2));
    //NOLINTEND
}

AutoSaveProperty::~AutoSaveProperty()
{
    documentNew.disconnect();
    documentMod.disconnect();
}

void AutoSaveProperty::slotNewObject(const App::DocumentObject& obj)
{
    std::vector<App::Property*> props;
    obj.getPropertyList(props);

    // if an object was deleted and then restored by an undo then add all properties
    // because this might be the data files which we may want to re-write
    for (const auto & prop : props) {
        slotChangePropertyData(*prop);
    }
}

void AutoSaveProperty::slotChangePropertyData(const App::Property& prop)
{
    std::stringstream str;
    str << static_cast<const void *>(&prop) << std::ends;
    std::string address = str.str();
    this->touched.insert(address);
}

// ----------------------------------------------------------------------------

RecoveryWriter::RecoveryWriter(AutoSaveProperty& saver)
  : Base::FileWriter(saver.dirName.c_str()), saver(saver)
{
}

RecoveryWriter::~RecoveryWriter() = default;

bool RecoveryWriter::shouldWrite(const std::string& name, const Base::Persistence *object) const
{
    // Property files of a view provider can always be written because
    // these are rather small files.
    if (object->isDerivedFrom(App::Property::getClassTypeId())) {
        const auto* prop = static_cast<const App::Property*>(object);
        const App::PropertyContainer* parent = prop->getContainer();
        if (parent && parent->isDerivedFrom(Gui::ViewProvider::getClassTypeId()))
            return true;
    }
    else if (object->isDerivedFrom(Gui::Document::getClassTypeId())) {
        return true;
    }

    // These are the addresses of touched properties of a document object.
    std::stringstream str;
    str << static_cast<const void *>(object) << std::ends;
    std::string address = str.str();

    // Check if the property will be exported to the same file. If the file has changed or if the property hasn't been
    // yet exported then (re-)write the file.
    std::map<std::string, std::string>::iterator it = saver.fileMap.find(address);
    if (it == saver.fileMap.end() || it->second != name) {
        saver.fileMap[address] = name;
        return true;
    }

    std::set<std::string>::const_iterator jt = saver.touched.find(address);
    return (jt != saver.touched.end());
}

namespace Gui {

class RecoveryRunnable : public QRunnable
{
public:
    RecoveryRunnable(const std::set<std::string>& modes, const char* dir, const char* file, const App::Property* p)
        : prop(p->Copy())
        , writer(dir)
    {
        writer.setModes(modes);

        dirName = QString::fromUtf8(dir);
        fileName = QString::fromUtf8(file);
        tmpName = QString::fromLatin1("%1.tmp%2").arg(fileName).arg(rand());
        writer.putNextEntry(tmpName.toUtf8().constData());
    }
    ~RecoveryRunnable() override
    {
        delete prop;
    }
    void run() override
    {
        prop->SaveDocFile(writer);
        writer.close();

        // We could have renamed the file in this thread. However, there is
        // still chance of crash when we deleted the original and before rename
        // the new file. So we ask the main thread to do it. There is still
        // possibility of crash caused by thread other than the main, but
        // that's the best we can do for now.
        QMetaObject::invokeMethod(AutoSaver::instance(), "renameFile",
                Qt::QueuedConnection, Q_ARG(QString,dirName)
                ,Q_ARG(QString,fileName),Q_ARG(QString,tmpName));
    }

private:
    App::Property* prop;
    Base::FileWriter writer;
    QString dirName;
    QString fileName;
    QString tmpName;
};

}

void RecoveryWriter::writeFiles()
{
    // use a while loop because it is possible that while
    // processing the files new ones can be added
    size_t index = 0;
    this->FileStream.close();
    while (index < FileList.size()) {
        FileEntry entry = FileList.begin()[index];

        if (shouldWrite(entry.FileName, entry.Object)) {
            std::string filePath = entry.FileName;
            std::string::size_type pos = 0;
            while ((pos = filePath.find('/', pos)) != std::string::npos) {
                std::string dirName = DirName + "/" + filePath.substr(0, pos);
                pos++;
                Base::FileInfo fi(dirName);
                fi.createDirectory();
            }

            // For properties a copy can be created and then this can be written to disk in a thread
            if (entry.Object->isDerivedFrom(App::Property::getClassTypeId())) {
                const auto* prop = static_cast<const App::Property*>(entry.Object);
                QThreadPool::globalInstance()->start(new RecoveryRunnable(getModes(), DirName.c_str(), entry.FileName.c_str(), prop));
            }
            else {
                std::string fileName = DirName + "/" + entry.FileName;
                this->FileStream.open(fileName.c_str(), std::ios::out | std::ios::binary);
                entry.Object->SaveDocFile(*this);
                this->FileStream.close();
            }
        }

        index++;
    }
}


#include "moc_AutoSaver.cpp"
