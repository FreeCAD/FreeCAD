// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#pragma once

#include <QCoreApplication>
#include <FCGlobal.h>

namespace Gui
{

class GuiExport ModuleIO
{
    Q_DECLARE_TR_FUNCTIONS(Gui::ModuleIO)

public:
    /*!
     * \brief verifyFile
     * Verifies the existence of the file. If it doesn't exist an error dialog
     * pops up and false is returned, otherwise true is returned.
     * \param filename
     * \return
     */
    static bool verifyFile(const QString& filename);
    /*!
     * \brief openFile
     * Opens the file.
     * The handling module is supposed to create a new document.
     * \param filename
     */
    static void openFile(const QString& filename);
    /*!
     * \brief verifyAndOpenFile
     * Verifies the existence of the file and opens it.
     * The handling module is supposed to create a new document.
     * \param filename
     */
    static void verifyAndOpenFile(const QString& filename);
    /*!
     * \brief importFile
     * Imports the files into the given document.
     * The handling module is supposed to create a new document if the passed
     * document doesn't exist.
     * \param filename
     * \param document
     */
    static void importFiles(const QStringList& filenames, const char* document);
};

}  // namespace Gui
