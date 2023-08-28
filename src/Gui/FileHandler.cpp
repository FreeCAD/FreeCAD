// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QFileInfo>
# include <QImageReader>
# include <QStringList>
# include <Inventor/SoInput.h>
#endif

#include "FileHandler.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "CommandT.h"
#include "EditorView.h"
#include "PythonEditor.h"
#include "MainWindow.h"
#include <App/Application.h>
#include <Base/Tools.h>

using namespace Gui;

FileHandler::FileHandler(const QString& filename)
    : filename(filename)
{

}

bool FileHandler::openFile()
{
    docname.clear();
    return openInternal();
}

bool FileHandler::importFile(const std::string& document)
{
    docname = document;
    return openInternal();
}

QString FileHandler::extension() const
{
    QFileInfo fi;
    fi.setFile(filename);
    return fi.suffix().toLower();
}

App::Document* FileHandler::getOrCreateDocument()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc) {
        doc = App::GetApplication().newDocument();
    }

    return doc;
}

App::Document* FileHandler::getOrCreateDocument(const std::string& document)
{
    App::Document *doc = nullptr;
    if (!document.empty()) {
        doc = App::GetApplication().getDocument(document.c_str());
    }
    else {
        doc = App::GetApplication().getActiveDocument();
    }

    if (!doc) {
        doc = App::GetApplication().newDocument(document.c_str());
    }

    return doc;
}

App::Document* FileHandler::createDocumentIfNeeded()
{
    if (docname.empty()) {
        return getOrCreateDocument();
    }

    return getOrCreateDocument(docname);
}

bool FileHandler::activateEditor()
{
    QList<EditorView*> views = getMainWindow()->findChildren<EditorView*>();
    for (const auto& it : views) {
        if (it->fileName() == filename) {
            it->setFocus();
            return true;
        }
    }

    return false;
}

bool FileHandler::openInternal()
{
    if (activateEditor()) {
        return true;
    }

    QFileInfo fi;
    fi.setFile(filename);
    QString ext = fi.suffix().toLower();

    auto hasExtension = [ext](const QStringList& suffixes) {
        return suffixes.contains(ext);
    };

    if (hasExtension(QStringList() << QLatin1String("iv"))) {
        openInventor();
        return true;
    }

    if (hasExtension(QStringList() << QLatin1String("wrl")
                                   << QLatin1String("wrz")
                                   << QLatin1String("vrml"))) {
        openVRML();
        return true;
    }

    if (hasExtension(QStringList() << QLatin1String("py")
                                   << QLatin1String("fcmacro")
                                   << QLatin1String("fcscript"))) {
        openPython();
        return true;
    }

    QStringList supportedFormats;
    auto imageFormats = QImageReader::supportedImageFormats();
    std::transform(imageFormats.cbegin(), imageFormats.cend(),
                   std::back_inserter(supportedFormats), [](const QByteArray& format) {
        return QString::fromLatin1(format);
    });

    if (hasExtension(supportedFormats)) {
        openImage();
        return true;
    }

    return false;
}

void FileHandler::openInternal(const char* type, const char* prop)
{
    App::Document* doc = createDocumentIfNeeded();

    QFileInfo fi;
    fi.setFile(filename);

    QString encBase = Base::Tools::escapeEncodeString(fi.baseName());
    QString encPath = Base::Tools::escapeEncodeString(fi.absoluteFilePath());

    Gui::cmdAppDocumentArgs(doc, "addObject('%s', '%s')", type,  encBase.toStdString());
    Gui::cmdAppDocumentArgs(doc, "ActiveObject.%s = '%s'", prop, encPath.toStdString());
    Gui::cmdAppDocumentArgs(doc, "ActiveObject.Label = '%s'", encBase.toStdString());
    Gui::cmdAppDocument(doc, "recompute()");
}

void FileHandler::openInventor()
{
    openInternal("App::InventorObject", "FileName");
}

void FileHandler::openVRML()
{
    QFileInfo fi;
    fi.setFile(filename);

    // Add this to the search path in order to read inline files (#0002029)
    QByteArray path = fi.absolutePath().toUtf8();
    SoInput::addDirectoryFirst(path.constData());

    openInternal("App::VRMLObject", "VrmlFile");

    SoInput::removeDirectory(path.constData());
}

void FileHandler::openImage()
{
    openInternal("Image::ImagePlane", "ImageFile");
}

void FileHandler::openPython()
{
    auto editor = new PythonEditor();
    editor->setWindowIcon(Gui::BitmapFactory().iconFromTheme("applications-python"));
    auto edit = new PythonEditorView(editor, getMainWindow());
    edit->open(filename);
    edit->resize(400, 300);
    getMainWindow()->addWindow( edit );
}
