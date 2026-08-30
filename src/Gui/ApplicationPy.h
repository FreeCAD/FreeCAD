// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2004 Jürgen Riegel <juergen.riegel@web.de>
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the     *
 *   License, or (at your option) any later version.                          *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful, but           *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Lesser General Public License for more details.                      *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD.  If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                         *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <Base/PyObjectBase.h>

namespace Gui
{

/** The ApplicationPy class
 * This is the Python wrapper class of Application.
 * @author Jürgen Riegel, Werner Mayer
 */
class GuiExport ApplicationPy
{
public:
    // clang-format off
    //---------------------------------------------------------------------
    // python exports goes here +++++++++++++++++++++++++++++++++++++++++++
    //---------------------------------------------------------------------
    // static python wrapper of the exported functions
    static PyObject* sActivateWorkbench        (PyObject *self,PyObject *args); // activates a workbench object
    static PyObject* sAddWorkbench             (PyObject *self,PyObject *args); // adds a new workbench handler to a list
    static PyObject* sRemoveWorkbench          (PyObject *self,PyObject *args); // removes a workbench handler from the list
    static PyObject* sGetWorkbench             (PyObject *self,PyObject *args); // retrieves the workbench handler
    static PyObject* sListWorkbenches          (PyObject *self,PyObject *args); // retrieves a list of all workbench handlers
    static PyObject* sActiveWorkbench          (PyObject *self,PyObject *args); // retrieves the active workbench object
    static PyObject* sAddResourcePath          (PyObject *self,PyObject *args); // adds a path where to find resources
    static PyObject* sAddLanguagePath          (PyObject *self,PyObject *args); // adds a path to a qm file
    static PyObject* sAddIconPath              (PyObject *self,PyObject *args); // adds a path to an icon file
    static PyObject* sAddIcon                  (PyObject *self,PyObject *args); // adds an icon to the cache
    static PyObject* sGetIcon                  (PyObject *self,PyObject *args); // get an icon from the cache
    static PyObject* sIsIconCached             (PyObject *self,PyObject *args); // check if an icon is cached

    static PyObject* sSendMsgToActiveView      (PyObject *self,PyObject *args);
    static PyObject* sSendMsgToFocusView       (PyObject *self,PyObject *args);

    static PyObject* sGetMainWindow            (PyObject *self,PyObject *args);
    static PyObject* sUpdateGui                (PyObject *self,PyObject *args);
    static PyObject* sUpdateLocale             (PyObject *self,PyObject *args);
    static PyObject* sGetLocale                (PyObject *self,PyObject *args);
    static PyObject* sSetLocale                (PyObject *self,PyObject *args);
    static PyObject* sSupportedLocales         (PyObject *self,PyObject *args);
    static PyObject* sCreateDialog             (PyObject *self,PyObject *args);
    static PyObject* sAddPreferencePage        (PyObject *self,PyObject *args);

    static PyObject* sRunCommand               (PyObject *self,PyObject *args);
    static PyObject* sAddCommand               (PyObject *self,PyObject *args);

    static PyObject* sHide                     (PyObject *self,PyObject *args); // deprecated
    static PyObject* sShow                     (PyObject *self,PyObject *args); // deprecated
    static PyObject* sHideObject               (PyObject *self,PyObject *args); // hide view provider object
    static PyObject* sShowObject               (PyObject *self,PyObject *args); // show view provider object

    static PyObject* sOpen                     (PyObject *self,PyObject *args); // open Python scripts
    static PyObject* sInsert                   (PyObject *self,PyObject *args); // open Python scripts
    static PyObject* sExport                   (PyObject *self,PyObject *args);
    static PyObject* sReload                   (PyObject *self,PyObject *args); // reload FCStd file
    static PyObject* sLoadFile                 (PyObject *self,PyObject *args); // open all types of files

    static PyObject* sCoinRemoveAllChildren    (PyObject *self,PyObject *args);
    static PyObject* sApplyElementColorOverride(PyObject *self,PyObject *args);
    static PyObject* sClearElementColorOverride(PyObject *self,PyObject *args);

    static PyObject* sActiveDocument           (PyObject *self,PyObject *args);
    static PyObject* sSetActiveDocument        (PyObject *self,PyObject *args);
    static PyObject* sActiveView               (PyObject *self,PyObject *args);
    static PyObject* sActivateView             (PyObject *self,PyObject *args);
    static PyObject* sGetDocument              (PyObject *self,PyObject *args);
    static PyObject* sEditDocument             (PyObject *self,PyObject *args);

    static PyObject* sDoCommand                (PyObject *self,PyObject *args);
    static PyObject* sDoCommandGui             (PyObject *self,PyObject *args);
    static PyObject* sDoCommandEval            (PyObject *self,PyObject *args);
    static PyObject* sDoCommandSkip            (PyObject *self,PyObject *args);
    static PyObject* sAddModule                (PyObject *self,PyObject *args);

    static PyObject* sShowDownloads            (PyObject *self,PyObject *args);
    static PyObject* sShowPreferences          (PyObject *self,PyObject *args);
    static PyObject* sShowPreferencesByName    (PyObject *self,PyObject *args);

    static PyObject* sCreateViewer             (PyObject *self,PyObject *args);
    static PyObject* sGetMarkerIndex           (PyObject *self,PyObject *args);

    static PyObject* sAddDocumentObserver      (PyObject *self,PyObject *args);
    static PyObject* sRemoveDocumentObserver   (PyObject *self,PyObject *args);

    static PyObject* sAddWorkbenchManipulator  (PyObject *self,PyObject *args);
    static PyObject* sRemoveWorkbenchManipulator(PyObject *self,PyObject *args);

    static PyObject* sListUserEditModes        (PyObject *self,PyObject *args);
    static PyObject* sGetUserEditMode          (PyObject *self,PyObject *args);
    static PyObject* sSetUserEditMode          (PyObject *self,PyObject *args);

    static PyObject* sSuspendWaitCursor        (PyObject *self, PyObject *args);
    static PyObject* sResumeWaitCursor         (PyObject *self, PyObject *args);
    static PyObject* sSubgraphFromObject       (PyObject *self, PyObject *args);
    static PyObject* sExportSubgraph           (PyObject *self, PyObject *args);
    static PyObject* sGetSoDBVersion           (PyObject *self, PyObject *args);
    static PyObject* sShowMainWindow           (PyObject *self, PyObject *args);
    static PyObject* sExec_loop                (PyObject *self, PyObject *args);
    static PyObject* sSetupWithoutGUI          (PyObject *self, PyObject *args);
    static PyObject* sEmbedToWindow            (PyObject *self, PyObject *args);
    // clang-format on
};

}  // namespace Gui
