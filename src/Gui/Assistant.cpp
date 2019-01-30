/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QDir>
# include <QFileInfo>
# include <QLibraryInfo>
# include <QMessageBox>
# include <QProcess>
# include <QTextStream>
# include <QCoreApplication>
#endif

#include "Assistant.h"
#include <Base/Console.h>
#include <App/Application.h>

using namespace Gui;

/* TRANSLATOR Gui::Assistant */

Assistant::Assistant()
    : proc(0)
{
}

Assistant::~Assistant()
{
    if (proc && proc->state() == QProcess::Running) {
        proc->terminate();
        proc->waitForFinished(3000);
    }
}

void Assistant::showDocumentation(const QString &page)
{
    if (!startAssistant())
        return;
    if (!page.isEmpty()) {
        QTextStream str(proc);
        str << QLatin1String("setSource qthelp://org.freecad.usermanual/doc/")
            << page << QLatin1Char('\n') << endl;
    }
}

bool Assistant::startAssistant()
{
#if QT_VERSION < 0x040400
    QMessageBox::critical(0, QObject::tr("Help"),
    QObject::tr("Unable to load documentation.\n"
    "In order to load it Qt 4.4 or higher is required."));
    return false;
#endif

    if (!proc) {
        proc = new QProcess();
        connect(proc, SIGNAL(readyReadStandardOutput()),
                this, SLOT(readyReadStandardOutput()));
        connect(proc, SIGNAL(readyReadStandardError()),
                this, SLOT(readyReadStandardError()));
    }

    if (proc->state() != QProcess::Running) {
#ifdef Q_OS_WIN
        QString app;
        app = QDir::toNativeSeparators(QString::fromUtf8
            (App::GetApplication().getHomePath()) + QLatin1String("bin/"));
#elif defined(Q_OS_MAC)
        QString app = QCoreApplication::applicationDirPath() + QDir::separator();
#else
        QString app = QLibraryInfo::location(QLibraryInfo::BinariesPath) + QDir::separator();
#endif 
        app += QLatin1String("assistant");

        // get the name of the executable and the doc path
        QString exe = QString::fromUtf8(App::GetApplication().getExecutableName());
        QString doc = QString::fromUtf8(App::Application::getHelpDir().c_str());
        QString qhc = doc + exe.toLower() + QLatin1String(".qhc");

        QFileInfo fi(qhc);
        if (!fi.isReadable()) {
            QMessageBox::critical(0, tr("%1 Help").arg(exe),
                tr("%1 help files not found (%2). You might need to install the %1 documentation package.").arg(exe, qhc));
            return false;
        }

        static bool first = true;
        if (first) {
            Base::Console().Log("Help file at %s\n", (const char*)qhc.toUtf8());
            first = false;
        }

        QStringList args;

        args << QLatin1String("-collectionFile") << qhc
             << QLatin1String("-enableRemoteControl");

        proc->start(app, args);

        if (!proc->waitForStarted()) {
            QMessageBox::critical(0, tr("%1 Help").arg(exe),
                tr("Unable to launch Qt Assistant (%1)").arg(app));
            return false;
        }
    }

    return true;
}

void Assistant::readyReadStandardOutput()
{
    QByteArray data = proc->readAllStandardOutput();
    Base::Console().Log("Help view: %s\n", data.constData());
}

void Assistant::readyReadStandardError()
{
    QByteArray data = proc->readAllStandardError();
    Base::Console().Log("Help view: %s\n", data.constData());
}

#include "moc_Assistant.cpp"
