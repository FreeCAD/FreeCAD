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


#ifndef GUI_DIALOG_DOCUMENTRECOVERY_H
#define GUI_DIALOG_DOCUMENTRECOVERY_H

#include <QDialog>
#include <QScopedPointer>
#include <QList>
#include <QFileInfo>
#include <string>

namespace Gui { namespace Dialog {

class DocumentRecoveryPrivate;

/*!
 @author Werner Mayer
 */
class DocumentRecovery : public QDialog
{
    Q_OBJECT

public:
    DocumentRecovery(const QList<QFileInfo>&, QWidget* parent = 0);
    virtual ~DocumentRecovery();

    void accept();
    bool foundDocuments() const;

protected:
    void closeEvent(QCloseEvent*);
    void contextMenuEvent(QContextMenuEvent*);
    QString createProjectFile(const QString&);
    void clearDirectory(const QFileInfo&);

protected Q_SLOTS:
    void on_buttonCleanup_clicked();
    void onDeleteSection();

private:
    static std::string doctools;
    QScopedPointer<DocumentRecoveryPrivate> d_ptr;
    Q_DISABLE_COPY(DocumentRecovery)
    Q_DECLARE_PRIVATE(DocumentRecovery)
};

} //namespace Dialog

} //namespace Gui


#endif //GUI_DIALOG_DOCUMENTRECOVERY_H
