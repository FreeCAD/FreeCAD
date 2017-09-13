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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <QCoreApplication>
# include <QObject>
# include <QEvent>
# include <QMutex>
# include <QMutexLocker>
# include <QSemaphore>
# include <QThread>
# include <QWaitCondition>
#endif

#include "DocumentProtector.h"

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Exception.h>

using namespace Sandbox;


namespace Sandbox {

static const int QT_CUSTOM_EVENT_PROTECTOR = 10000;

class AbstractCustomProtectorEvent : public QEvent
{
public:
    AbstractCustomProtectorEvent()
        : QEvent(QEvent::Type(QT_CUSTOM_EVENT_PROTECTOR)), semaphore(0)
    {
    }
    ~AbstractCustomProtectorEvent()
    {
        if (semaphore)
            semaphore->release();
    }
    virtual void execute() = 0;

    QSemaphore* semaphore;
};

class CustomAddObjectEvent : public AbstractCustomProtectorEvent
{
public:
    CustomAddObjectEvent(App::DocumentObject** o, App::Document* d, const std::string& type, const std::string& name)
        : obj(o), doc(d), type(type), name(name)
    {
    }
    ~CustomAddObjectEvent()
    {
    }
    void execute()
    {
        *obj = doc->addObject(this->type.c_str(), this->name.c_str());
    }

protected:
    App::DocumentObject** obj;
    App::Document* doc;
    std::string type, name;
};

class CustomRemoveObjectEvent : public AbstractCustomProtectorEvent
{
public:
    CustomRemoveObjectEvent(App::Document* d, const std::string& n) : doc(d), name(n)
    {
    }
    ~CustomRemoveObjectEvent()
    {
    }
    void execute()
    {
        doc->removeObject(this->name.c_str());
    }

protected:
    App::Document* doc;
    std::string name;
};

class CustomRecomputeEvent : public AbstractCustomProtectorEvent
{
public:
    CustomRecomputeEvent(App::Document* d) : doc(d)
    {
    }
    ~CustomRecomputeEvent()
    {
    }
    void execute()
    {
        doc->recompute();
    }

protected:
    App::Document* doc;
};

class CustomPropertyEvent : public AbstractCustomProtectorEvent
{
public:
    CustomPropertyEvent(App::Property& p, const App::Property& v)
      : property(p), value(v)
    {
    }
    ~CustomPropertyEvent()
    {
    }
    void execute()
    {
        property.Paste(value);
    }

protected:
    App::Property& property;
    const App::Property& value;
};

class CustomCallableEvent : public AbstractCustomProtectorEvent
{
public:
    CustomCallableEvent(const AbstractCallable& call)
      : callable(call)
    {
    }
    ~CustomCallableEvent()
    {
    }
    void execute()
    {
        callable();
    }

protected:
    const AbstractCallable& callable;
};

class CustomPurgeEvent : public AbstractCustomProtectorEvent
{
public:
    CustomPurgeEvent(App::DocumentObject* o)
      : obj(o)
    {
    }
    ~CustomPurgeEvent()
    {
    }
    void execute()
    {
        obj->purgeTouched();
    }

protected:
    App::DocumentObject* obj;
};

class DocumentReceiver : public QObject
{
public:
    DocumentReceiver(QObject *parent = 0) : QObject(parent)
    {
    }
    ~DocumentReceiver()
    {
    }

    static DocumentReceiver *globalInstance();

protected:
    void customEvent(QEvent*);
    void postEventAndWait(QEvent*);

    // friends
    friend class DocumentProtector;
    friend class DocumentObjectProtector;
};

Q_GLOBAL_STATIC(DocumentReceiver, theInstance)

DocumentReceiver *DocumentReceiver::globalInstance()
{
    return theInstance();
}

// Posts an event and waits for it to finish processing
void DocumentReceiver::postEventAndWait(QEvent* e)
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = this->thread(); // this is the main thread

    if (currentThread == thr) {
        // we're in the main thread
        QCoreApplication::sendEvent(this, e);
        delete e;
    }
    else {
        // NOTE: We send an event to this instance that's why it is important
        // that this object is part of the main thread
        QSemaphore semaphore;
        static_cast<AbstractCustomProtectorEvent*>(e)->semaphore = &semaphore;
        QCoreApplication::postEvent(this, e);
        // wait until the event has been processed
        semaphore.acquire();
    }
}

void DocumentReceiver::customEvent(QEvent* e)
{
    if ((int)e->type() == QT_CUSTOM_EVENT_PROTECTOR) {
        static_cast<AbstractCustomProtectorEvent*>(e)->execute();
    }
}

}

DocumentProtector::DocumentProtector(App::Document* doc)
  : App::DocumentObserver(doc)
{
}

DocumentProtector::~DocumentProtector()
{
}

void DocumentProtector::init()
{
    // this method must be called somewhere from the main thread
    DocumentReceiver::globalInstance()->
        moveToThread(QCoreApplication::instance()->thread());
}

void DocumentProtector::slotDeletedDocument(const App::Document& Doc)
{
    if (&Doc == getDocument()) {
        this->detachDocument();
    }
}

void DocumentProtector::validate()
{
    if (!this->getDocument())
        throw Base::ValueError("Handled document is null");
}

App::DocumentObject *DocumentProtector::addObject(const std::string& type, const std::string& name)
{
    validate();
    App::DocumentObject* obj;
    DocumentReceiver::globalInstance()->postEventAndWait
        (new CustomAddObjectEvent(&obj, this->getDocument(), type, name));
    return obj;
}

void DocumentProtector::removeObject(const std::string& name)
{
    validate();
    DocumentReceiver::globalInstance()->postEventAndWait
        (new CustomRemoveObjectEvent(this->getDocument(), name));
}

void DocumentProtector::recompute()
{
    validate();
    DocumentReceiver::globalInstance()->postEventAndWait
        (new CustomRecomputeEvent(this->getDocument()));
}

// ------------------------------------------

DocumentObjectProtector::DocumentObjectProtector(App::DocumentObject* o) : obj(o)
{
}

DocumentObjectProtector::~DocumentObjectProtector()
{
}

void DocumentObjectProtector::validate()
{
    if (!obj)
        throw Base::ValueError("Handled document object is null");
}

App::DocumentObject* DocumentObjectProtector::getObject() const
{
    return this->obj;
}

bool DocumentObjectProtector::setProperty(const std::string& name, const App::Property& value)
{
    validate();
    App::Property* prop = obj->getPropertyByName(name.c_str());
    if (!prop)
        return false;
    DocumentReceiver::globalInstance()->postEventAndWait(new CustomPropertyEvent(*prop, value));
    return true;
}

void DocumentObjectProtector::execute(const AbstractCallable& call)
{
    validate();
    DocumentReceiver::globalInstance()->postEventAndWait(new CustomCallableEvent(call));
}

void DocumentObjectProtector::purgeTouched()
{
    validate();
    DocumentReceiver::globalInstance()->postEventAndWait(new CustomPurgeEvent(obj));
}
