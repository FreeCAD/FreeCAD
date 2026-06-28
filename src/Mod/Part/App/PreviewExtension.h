// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Kacper Donat <kacper@kadet.net>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#pragma once

#include "PropertyTopoShape.h"

#include <App/DocumentObject.h>
#include <App/DocumentObjectExtension.h>
#include <App/ExtensionPython.h>

#include <Mod/Part/PartGlobal.h>

namespace Part
{

class PartExport PreviewExtension: public App::DocumentObjectExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(Part::PreviewExtension);
    using inherited = DocumentObjectExtension;

public:
    /// Shape displayed as a semi-transparent preview, should be a delta between current state and
    /// previous one
    PropertyPartShape PreviewShape;

    PreviewExtension();

    bool isPreviewFresh() const
    {
        return _isPreviewFresh;
    }

    void updatePreview();

    virtual bool mustRecomputePreview() const;

protected:
    void extensionOnChanged(const App::Property* prop) override;

    virtual App::DocumentObjectExecReturn* recomputePreview()
    {
        return App::DocumentObject::StdReturn;
    };

private:
    bool _isPreviewFresh {false};
};

/**
 * Service interface for update scheduler implementation.
 *
 * The scheduler manages the timing of preview recomputations. It is designed to debounce
 * multiple requests—such as those occurring during batch property updates—ensuring that
 * expensive preview computations are only performed when the system is idle or at a
 * more convenient time, rather than for every intermediate step.
 */
class PartExport PreviewUpdateScheduler
{
public:
    PreviewUpdateScheduler() = default;
    virtual ~PreviewUpdateScheduler() = default;

    FC_DISABLE_COPY_MOVE(PreviewUpdateScheduler);

    /**
     * Schedules a preview recompute for the given object.
     *
     * Instead of triggering an immediate update, this method registers the object
     * with the scheduler. If multiple updates are requested in rapid succession,
     * the scheduler should collapse them into a single recomputation to improve performance.
     *
     * @param object The preview extension of the object that requires an update.
     */
    virtual void schedulePreviewRecompute(App::DocumentObject* object) = 0;
};

template<typename ExtensionT>
class PreviewExtensionPythonT: public ExtensionT
{

public:
    PreviewExtensionPythonT() = default;
    ~PreviewExtensionPythonT() override = default;
};

using PreviewExtensionPython = App::ExtensionPythonT<PreviewExtensionPythonT<PreviewExtension>>;

}  // namespace Part
