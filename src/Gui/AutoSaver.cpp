/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

using namespace Gui;

AutoSaver* AutoSaver::self = 0;

AutoSaver::AutoSaver(QObject* parent)
  : QObject(parent), timeout(5)
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

void AutoSaver::setTimeout(int s)
{
    timeout = Base::clamp<int>(s, 0, 30);
}

void AutoSaver::slotCreateDocument(const App::Document& Doc)
{
    std::string name = Doc.getName();
    if (timeout > 0) {
        int id = startTimer(timeout * 60000);
        timerMap[name] = id;
    }
}

void AutoSaver::slotDeleteDocument(const App::Document& Doc)
{
    std::string name = Doc.getName();
    std::map<std::string, int>::iterator it = timerMap.find(name);
    if (it != timerMap.end()) {
        killTimer(it->second);
        timerMap.erase(it);
    }
}

void AutoSaver::saveDocument(const std::string& name)
{
    Gui::WaitCursor wc;
    App::Document* doc = App::GetApplication().getDocument(name.c_str());
    if (doc) {
        std::string fn = doc->TransientDir.getValue();
        fn += "/fc_autosave_file.fcstd";
        Base::FileInfo tmp(fn);

        // make sure to tmp. disable saving thumbnails because this causes trouble if the
        // associated 3d view is not active
        Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetParameterGroupByPath
            ("User parameter:BaseApp/Preferences/Document");
        bool save = hGrp->GetBool("SaveThumbnail",false);
        hGrp->SetBool("SaveThumbnail",false);

        // open extra scope to close ZipWriter properly
        {
            Base::ofstream file(tmp, std::ios::out | std::ios::binary);
            Base::ZipWriter writer(file);

            writer.setComment("FreeCAD Document");
            writer.setLevel(0);
            writer.putNextEntry("Document.xml");

            doc->Save(writer);

            // Special handling for Gui document.
            doc->signalSaveDocument(writer);

            // write additional files
            writer.writeFiles();
        }
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
