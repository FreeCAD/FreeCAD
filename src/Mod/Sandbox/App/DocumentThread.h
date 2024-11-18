/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer@users.sourceforge.net>        *
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


#ifndef SANDBOX_DOCUMENTTHREAD_H
#define SANDBOX_DOCUMENTTHREAD_H

#include <QThread>
#include <Base/Mutex.h>
#include <Base/Handle.h>
#include <App/DocumentObject.h>

namespace Mesh {
    class MeshObject;
}

namespace Sandbox {

class SandboxAppExport DocumentThread : public QThread
{
public:
    DocumentThread(QObject* parent=0);
    ~DocumentThread();

protected:
    void run();
};

class SandboxAppExport WorkerThread : public QThread
{
public:
    WorkerThread(QObject* parent=0);
    ~WorkerThread();

protected:
    void run();
};

class SandboxAppExport PythonThread : public QThread
{
public:
    PythonThread(QObject* parent=0);
    ~PythonThread();

protected:
    void run();
    static QRecursiveMutex mutex;
};

class SandboxAppExport MeshLoaderThread : public QThread
{
public:
    MeshLoaderThread(const QString&, QObject* parent=0);
    ~MeshLoaderThread();

    Base::Reference<Mesh::MeshObject> getMesh() const;

protected:
    void run();

private:
    QString filename;
    Base::Reference<Mesh::MeshObject> mesh;
};

class SandboxObject : public App::DocumentObject
{
    PROPERTY_HEADER(SandboxObject);

public:
    SandboxObject();
    ~SandboxObject();

    App::PropertyInteger Integer;

    short mustExecute(void) const;
    App::DocumentObjectExecReturn *execute(void);
    void onChanged(const App::Property* prop);
    const char* getViewProviderName(void) const {
        return "Gui::ViewProviderDocumentObject";
    }

    void setIntValue(int);
    void resetValue();
};

class SandboxAppExport DocumentTestThread : public QThread
{
public:
    DocumentTestThread(QObject* parent=0);
    ~DocumentTestThread();

protected:
    void run();
};

class SandboxAppExport DocumentSaverThread : public QThread
{
public:
    DocumentSaverThread(App::Document* doc, QObject* parent=0);
    ~DocumentSaverThread();

protected:
    void run();

private:
    App::Document* doc;
};

}

#endif // SANDBOX_DOCUMENTTHREAD_H

