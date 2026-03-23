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


#pragma once

#include <QObject>

#include <map>
#include <set>
#include <string>
#include <fastsignals/signal.h>
#include <Base/Writer.h>

namespace App
{
class Document;
class DocumentObject;
class Property;
}  // namespace App

namespace Gui
{
class ViewProvider;

class AutoSaveProperty
{
public:
    AutoSaveProperty(const App::Document* doc);
    ~AutoSaveProperty();
    int timerId;
    std::set<std::string> touched;
    std::string dirName;
    std::map<std::string, std::string> fileMap;

private:
    void slotNewObject(const App::DocumentObject&);
    void slotChangePropertyData(const App::Property&);
    using Connection = fastsignals::connection;
    Connection documentNew;
    Connection documentMod;
};

/*!
 The class AutoSaver is used to automatically save a document to a temporary file.
 @author Werner Mayer
 */
class AutoSaver: public QObject
{
    Q_OBJECT

private:
    static AutoSaver* self;
    AutoSaver(QObject* parent);
    ~AutoSaver() override;

public:
    static AutoSaver* instance();
    /*!
     Sets the timeout in milliseconds. A value of 0 means that no timer is used.
     */
    void setTimeout(int ms);
    /*!
     Enables or disables to create compreesed recovery files.
     */
    void setCompressed(bool on);

protected:
    void slotCreateDocument(const App::Document& Doc);
    void slotDeleteDocument(const App::Document& Doc);
    void timerEvent(QTimerEvent* event) override;
    void saveDocument(const std::string&, AutoSaveProperty&);

public Q_SLOTS:
    void renameFile(QString dirName, QString file, QString tmpFile);

private:
    int timeout; /*!< Timeout in milliseconds */
    bool compressed;
    std::map<std::string, AutoSaveProperty*> saverMap;
};

class RecoveryWriter: public Base::FileWriter
{
public:
    RecoveryWriter(AutoSaveProperty&);
    ~RecoveryWriter() override;

    /*!
     This method can be re-implemented in sub-classes to avoid
     to write out certain objects. The default implementation
     always returns true.
     */
    bool shouldWrite(const std::string&, const Base::Persistence*) const override;
    void writeFiles() override;

private:
    AutoSaveProperty& saver;
};

}  // namespace Gui
