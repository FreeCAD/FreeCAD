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

#include <QDialog>
#include <QFileInfo>
#include <QFileInfoList>
#include <QList>
#include <QScopedPointer>


namespace Gui
{
namespace Dialog
{

class DocumentRecoveryPrivate;

/*!
 @author Werner Mayer
 */
class DocumentRecovery: public QDialog
{
    Q_OBJECT

public:
    explicit DocumentRecovery(const QList<QFileInfo>&, QWidget* parent = nullptr);
    ~DocumentRecovery() override;

    void accept() override;
    bool foundDocuments() const;

protected:
    void closeEvent(QCloseEvent*) override;
    void contextMenuEvent(QContextMenuEvent*) override;
    QString createProjectFile(const QString&);
    void cleanup(QDir&, const QList<QFileInfo>&, const QString&);

protected:
    void onButtonCleanupClicked();
    void onDeleteSection();

private:
    static std::string doctools;
    QScopedPointer<DocumentRecoveryPrivate> d_ptr;
    Q_DISABLE_COPY(DocumentRecovery)
    Q_DECLARE_PRIVATE(DocumentRecovery)
};

class DocumentRecoveryFinder
{
public:
    bool checkForPreviousCrashes();

private:
    void checkDocumentDirs(QDir&, const QList<QFileInfo>&, const QString&);
    bool showRecoveryDialogIfNeeded();

private:
    QList<QFileInfo> restoreDocFiles;
};

class DocumentRecoveryHandler
{
public:
    void checkForPreviousCrashes(
        const std::function<void(QDir&, const QList<QFileInfo>&, const QString&)>& callableFunc
    ) const;
};

class DocumentRecoveryCleaner
{
public:
    void clearDirectory(const QFileInfo& dir);
    void setIgnoreFiles(const QStringList&);
    void setIgnoreDirectories(const QFileInfoList&);

private:
    void subtractFiles(QStringList&);
    void subtractDirs(QFileInfoList&);

private:
    QStringList ignoreFiles;
    QFileInfoList ignoreDirs;
};

}  // namespace Dialog

}  // namespace Gui
