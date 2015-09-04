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
# include <QTextStream>
# include <boost/bind.hpp>
#endif

#include "AutoSaver.h"
#include <Base/Console.h>
#include <Base/Stream.h>
#include <Base/Tools.h>
#include <Base/Writer.h>
#include <App/Application.h>
#include <App/Document.h>

#include "WaitCursor.h"
#include "Widgets.h"

using namespace Gui;

AutoSaver* AutoSaver::self = 0;

AutoSaver::AutoSaver(QObject* parent)
  : QObject(parent), timeout(900000)
{
    App::GetApplication().signalNewDocument.connect(boost::bind(&AutoSaver::slotCreateDocument, this, _1));
    App::GetApplication().signalDeleteDocument.connect(boost::bind(&AutoSaver::slotDeleteDocument, this, _1));
}

AutoSaver::~AutoSaver()
{
}

AutoSaver* AutoSaver::instance()
{
    if (!self)
        self = new AutoSaver(QApplication::instance());
    return self;
}

void AutoSaver::setTimeout(int ms)
{
    timeout = Base::clamp<int>(ms, 0, 3600000); // between 0 and 60 min

    // go through the attached documents and apply the new timeout
    for (std::map<std::string, int>::iterator it = timerMap.begin(); it != timerMap.end(); ++it) {
        if (it->second > 0)
            killTimer(it->second);
        int id = timeout > 0 ? startTimer(timeout) : 0;
        it->second = id;
    }
}

void AutoSaver::slotCreateDocument(const App::Document& Doc)
{
    std::string name = Doc.getName();
    int id = timeout > 0 ? startTimer(timeout) : 0;
    timerMap[name] = id;
}

void AutoSaver::slotDeleteDocument(const App::Document& Doc)
{
    std::string name = Doc.getName();
    std::map<std::string, int>::iterator it = timerMap.find(name);
    if (it != timerMap.end()) {
        if (it->second > 0)
            killTimer(it->second);
        timerMap.erase(it);
    }
}

void AutoSaver::saveDocument(const std::string& name)
{
    Gui::WaitCursor wc;
    App::Document* doc = App::GetApplication().getDocument(name.c_str());
    if (doc) {
        // Write recovery meta file
        QFile file(QString::fromLatin1("%1/fc_recovery_file.xml")
            .arg(QString::fromUtf8(doc->TransientDir.getValue())));
        if (file.open(QFile::WriteOnly)) {
            QTextStream str(&file);
            str.setCodec("UTF-8");
            str << "<?xml version='1.0' encoding='utf-8'?>" << endl
                << "<AutoRecovery SchemaVersion=\"1\">" << endl;
            str << "  <Status>Created</Status>" << endl;
            str << "  <Label>" << doc->Label.getValue() << "</Label>" << endl; // store the document's current label
            str << "</AutoRecovery>" << endl;
            file.close();
        }

        std::string fn = doc->TransientDir.getValue();
        fn += "/fc_recovery_file.fcstd";
        Base::FileInfo tmp(fn);

        // make sure to tmp. disable saving thumbnails because this causes trouble if the
        // associated 3d view is not active
        Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetParameterGroupByPath
            ("User parameter:BaseApp/Preferences/Document");
        bool save = hGrp->GetBool("SaveThumbnail",false);
        hGrp->SetBool("SaveThumbnail",false);

        Gui::StatusWidget* sw = new Gui::StatusWidget(qApp->activeWindow());
        sw->setStatusText(tr("Please wait until the AutoRecovery file has been saved..."));
        sw->show();
        qApp->processEvents();

        // open extra scope to close ZipWriter properly
        Base::StopWatch watch;
        watch.start();
        {
            Base::ofstream file(tmp, std::ios::out | std::ios::binary);
            if (file.is_open()) {
                Base::ZipWriter writer(file);

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

        sw->hide();
        sw->deleteLater();

        std::string str = watch.toString(watch.elapsed());
        Base::Console().Log("Save AutoRecovery file: %s\n", str.c_str());
        hGrp->SetBool("SaveThumbnail",save);
    }
}

void AutoSaver::timerEvent(QTimerEvent * event)
{
    int id = event->timerId();
    for (std::map<std::string, int>::iterator it = timerMap.begin(); it != timerMap.end(); ++it) {
        if (it->second == id) {
            try {
                saveDocument(it->first);
                break;
            }
            catch (...) {
                Base::Console().Error("Failed to auto-save document '%s'\n", it->first.c_str());
            }
        }
    }
}

#include "moc_AutoSaver.cpp"
