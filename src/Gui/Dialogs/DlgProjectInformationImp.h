// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

namespace App
{
class Document;
}

namespace Gui
{

namespace Dialog
{

class Ui_DlgProjectInformation;
class DlgProjectInformationImp: public QDialog
{
    Q_OBJECT

public:
    DlgProjectInformationImp(
        App::Document* doc,
        QWidget* parent = nullptr,
        Qt::WindowFlags fl = Qt::WindowFlags()
    );
    ~DlgProjectInformationImp() override;
    void accept() override;

    /*!
     * Discards the edits, but still honors a checked "do not ask again": someone who
     * wants to be rid of the dialog is as likely to dismiss it as to confirm it.
     */
    void reject() override;

    /*!
     * Shows the options that are only offered when a document has just been created:
     * remembering the creator, the company and the license as defaults, and
     * suppressing the dialog for future new documents. The file section is hidden
     * because none of it is known until the document has been written.
     */
    void setNewDocumentMode();

    /*!
     * Offers this dialog for a document that has just been created, unless the user
     * has turned that off. Does nothing without a GUI.
     *
     * @param[in] doc The document that was just created.
     */
    static void showForNewDocument(App::Document* doc);

private Q_SLOTS:
    void open_url();
    void onLicenseTypeChanged(int index);

private:
    void applyNewDocumentOptions();
    void applyDontShowAgain();

    App::Document* _doc;
    Ui_DlgProjectInformation* ui;
    bool _newDocumentMode {false};
};

}  // namespace Dialog
}  // namespace Gui
