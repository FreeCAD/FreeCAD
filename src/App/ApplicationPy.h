// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <Base/PyObjectBase.h>

namespace App
{

/** The ApplicationPy class
 * This is the Python wrapper class of Application.
 * @author Jürgen Riegel, Werner Mayer
 */
class AppExport ApplicationPy
{
public:
    // clang-format off
    // static python wrapper of the exported functions
    static PyObject* sParamGet               (PyObject *self, PyObject *args);
    static PyObject* sSaveParameter          (PyObject *self, PyObject *args);
    static PyObject* sVersion                (PyObject *self, PyObject *args);
    static PyObject* sConfigGet              (PyObject *self, PyObject *args);
    static PyObject* sConfigSet              (PyObject *self, PyObject *args);
    static PyObject* sConfigDump             (PyObject *self, PyObject *args);
    static PyObject* sAddImportType          (PyObject *self, PyObject *args);
    static PyObject* sChangeImportModule     (PyObject *self, PyObject *args);
    static PyObject* sGetImportType          (PyObject *self, PyObject *args);
    static PyObject* sAddExportType          (PyObject *self, PyObject *args);
    static PyObject* sAddTranslatableExportType (PyObject *self, PyObject *args);
    static PyObject* sChangeExportModule     (PyObject *self, PyObject *args);
    static PyObject* sGetExportType          (PyObject *self, PyObject *args);
    static PyObject* sGetResourceDir         (PyObject *self, PyObject *args);
    static PyObject* sGetLibraryDir          (PyObject *self, PyObject *args);
    static PyObject* sGetTempPath            (PyObject *self, PyObject *args);
    static PyObject* sGetUserCachePath       (PyObject *self, PyObject *args);
    static PyObject* sGetUserConfigDir       (PyObject *self, PyObject *args);
    static PyObject* sGetUserAppDataDir      (PyObject *self, PyObject *args);
    static PyObject* sGetUserMacroDir        (PyObject *self, PyObject *args);
    static PyObject* sGetHelpDir             (PyObject *self, PyObject *args);
    static PyObject* sGetHomePath            (PyObject *self, PyObject *args);

    static PyObject* sLoadFile               (PyObject *self,PyObject *args);
    static PyObject* sOpen                   (PyObject *self,PyObject *args, PyObject *kwd);
    static PyObject* sOpenDocument           (PyObject *self,PyObject *args, PyObject *kwd);
    static PyObject* sSaveDocument           (PyObject *self,PyObject *args);
    static PyObject* sWriteRecoverySnapshotToTransientDir
                                               (PyObject *self,PyObject *args, PyObject *kwd);
    static PyObject* sNewDocument            (PyObject *self,PyObject *args, PyObject *kwd);
    static PyObject* sCloseDocument          (PyObject *self,PyObject *args);
    static PyObject* sActiveDocument         (PyObject *self,PyObject *args);
    static PyObject* sSetActiveDocument      (PyObject *self,PyObject *args);
    static PyObject* sGetDocument            (PyObject *self,PyObject *args);
    static PyObject* sListDocuments          (PyObject *self,PyObject *args);
    static PyObject* sAddDocumentObserver    (PyObject *self,PyObject *args);
    static PyObject* sRemoveDocumentObserver (PyObject *self,PyObject *args);
    static PyObject *sIsRestoring            (PyObject *self,PyObject *args);

    static PyObject *sSetLogLevel            (PyObject *self,PyObject *args);
    static PyObject *sGetLogLevel            (PyObject *self,PyObject *args);

    static PyObject *sCheckLinkDepth         (PyObject *self,PyObject *args);
    static PyObject *sGetLinksTo             (PyObject *self,PyObject *args);

    static PyObject *sGetDependentObjects    (PyObject *self,PyObject *args);

    static PyObject *sSetActiveTransaction   (PyObject *self,PyObject *args);
    static PyObject *sGetActiveTransaction   (PyObject *self,PyObject *args);
    static PyObject *sCloseActiveTransaction (PyObject *self,PyObject *args);
    static PyObject *sCheckAbort             (PyObject *self,PyObject *args);
    // clang-format on
};

}
