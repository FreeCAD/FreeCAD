// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 Kacper Donat <kacper@kadet.net>                     *
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

#include "PreviewUpdateScheduler.h"

using namespace PartGui;

QtPreviewUpdateScheduler::QtPreviewUpdateScheduler(QObject* parent)
    : QObject(parent)
{}

inline void QtPreviewUpdateScheduler::schedulePreviewRecompute(App::DocumentObject* object)
{
    if (!object) {
        return;
    }

    toBeUpdated.emplace(object);

    // if method call was already scheduled there is no need to queue another one
    if (scheduled) {
        return;
    }

    QMetaObject::invokeMethod(this, &QtPreviewUpdateScheduler::flush, Qt::QueuedConnection);
}

void QtPreviewUpdateScheduler::flush()
{
    scheduled = false;

    // use std::exchange to prevent race conditions on updates that could occur during a flush
    for (auto objects = std::exchange(this->toBeUpdated, {}); auto& object : objects) {
        if (object.expired()) {
            continue;
        }

        if (auto* previewExtension = object->getExtensionByType<Part::PreviewExtension>(true)) {
            previewExtension->updatePreview();
        }
    }
}

#include "moc_PreviewUpdateScheduler.cpp"
