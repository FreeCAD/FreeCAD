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

#include "PreviewExtension.h"

#include <App/DocumentObject.h>
#include <App/ExtensionPython.h>
#include <Base/PyObjectBase.h>

EXTENSION_PROPERTY_SOURCE(Part::PreviewExtension, App::DocumentObjectExtension)

namespace Part
{

template<typename ExtensionT>
App::DocumentObjectExecReturn* PreviewExtensionPythonT<ExtensionT>::recomputePreview()
{
    EXTENSION_PROXY_NOARG(recomputePreview)

    if (!result.isNone() && result.isString()) {
        return new App::DocumentObjectExecReturn(static_cast<std::string>(Py::String(result)));
    }

    if (!result.isNone()) {
        return App::DocumentObject::StdReturn;
    }

    return ExtensionT::recomputePreview();
}

template class PartExport PreviewExtensionPythonT<PreviewExtension>;

}  // namespace Part

EXTENSION_PROPERTY_SOURCE_TEMPLATE(Part::PreviewExtensionPython, Part::PreviewExtension)
template class PartExport App::ExtensionPythonT<Part::PreviewExtensionPythonT<Part::PreviewExtension>>;

Part::PreviewExtension::PreviewExtension()
{
    initExtensionType(getExtensionClassTypeId());

    EXTENSION_ADD_PROPERTY(PreviewShape, (TopoShape()));

    PreviewShape.setStatus(App::Property::Output, true);
    PreviewShape.setStatus(App::Property::Transient, true);
    PreviewShape.setStatus(App::Property::Hidden, true);
}

void Part::PreviewExtension::updatePreview()
{
    if (_isPreviewFresh) {
        return;
    }

    recomputePreview();

    _isPreviewFresh = true;
}

bool Part::PreviewExtension::mustRecomputePreview() const
{
    return getExtendedObject()->mustRecompute();
}

void Part::PreviewExtension::extensionOnChanged(const App::Property* prop)
{
    DocumentObjectExtension::extensionOnChanged(prop);

    // Invalidate the preview on any input property change. The PreviewShape itself is the
    // output of recomputePreview, so changing it does not require another recompute.
    const bool isInputProp = prop != &PreviewShape && !prop->testStatus(App::Property::Output);

    if (isInputProp || mustRecomputePreview()) {
        _isPreviewFresh = false;
    }
}
