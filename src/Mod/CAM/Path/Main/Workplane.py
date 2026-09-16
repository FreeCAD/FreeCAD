# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""Named work planes held by a Job and shared between operations.

A work plane is a plain ``Part::LocalCoordinateSystem``. That object already
carries an attachment to the geometry it was derived from, the map modes that
describe a plane from a face or three points, an editable offset, a placement
that recomputes when the model moves, and a view provider that draws labelled
axes. This module creates them, files them under the Job, and resolves the
object a user picked to the Job's own copy of it.
"""

import FreeCAD
import Path
import Path.Base.Util as PathUtil

translate = FreeCAD.Qt.translate

__title__ = "CAM Workplane"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Named work planes shared between a Job's operations."


def resolveToJobModel(job, obj):
    """resolveToJobModel(job, obj) ... the Job's model that corresponds to obj.

    Operations reference the Job's Resource Clones, not the user's original
    objects. A work plane attached to the original would follow an object the
    operations never look at. Returns obj itself if it is already one of the
    Job's models, the clone of it if it is an original, and None if it belongs
    to neither."""
    if job is None or not getattr(job, "Model", None):
        return None
    for model in job.Model.Group:
        if model == obj:
            return model
        if job.Proxy.baseObject(job, model) == obj:
            return model
    return None


def createWorkplane(job, base=None, sub=None, label=None, placement=None, check_machine=True):
    """createWorkplane(job, base=None, sub=None, label=None, placement=None, check_machine=True)
    ... add a named work plane to job and return it.

    With *base* and *sub* naming a planar face, the plane is attached to it in
    FlatFace mode: its +Z is the face normal and it follows the face if the
    model changes. *base* may be the user's original object or the Job's clone
    of it; it is resolved to the clone either way.

    With *placement* and no face, the plane is created unattached at that
    placement. With neither it sits at the Job origin, aligned with the Job's
    axes, for the user to position by hand.

    On a Job whose machine has no rotary axes the plane must be parallel to
    the table; a tilted one is refused with ValueError and not created, since
    nothing could point the tool along it. Parallel planes are useful there
    too: a datum for depths, and a turned X. *check_machine* False skips
    that, for migrating a frame an older document already holds: the
    operation, not the migration, is where an unreachable plane fails."""
    doc = job.Document
    workplane = doc.addObject("Part::LocalCoordinateSystem", "Workplane")
    workplane.Label = label or "Workplane"

    if base is not None and sub:
        model = resolveToJobModel(job, base)
        if model is None:
            Path.Log.warning(
                "%s is not part of %s; attaching the work plane to it anyway"
                % (base.Label, job.Label)
            )
            model = base
        workplane.AttachmentSupport = [(model, sub)]
        workplane.MapMode = "FlatFace"
    elif placement is not None:
        workplane.Placement = FreeCAD.Placement(placement)

    job.Proxy.setupWorkplanes(job)
    job.Workplanes.addObject(workplane)
    doc.recompute()
    configureView(workplane)

    if check_machine and not PathUtil.jobHasRotaryMachine(job) and not _parallelToTable(workplane):
        doc.removeObject(workplane.Name)
        raise ValueError(
            translate(
                "CAM",
                "{plane} is tilted, and the Job's machine has no rotary axes to point the "
                "tool along it. Without rotary axes a work plane must be parallel to the table.",
            ).format(plane=label or (sub and base and "%s.%s" % (base.Label, sub)) or "The plane")
        )
    return workplane


def _parallelToTable(workplane):
    z_up = FreeCAD.Vector(0, 0, 1)
    return workplane.Placement.Rotation.multVec(z_up).isEqual(z_up, 1e-6)


def configureView(workplane):
    """configureView(workplane) ... show a work plane as a frame the user can
    see but not click on.

    A Part::LocalCoordinateSystem draws three datum planes as well as its
    axes and origin, and every one of them is pickable in the 3D view. A
    plane made from a face sits on that face, so a click meant for the face
    - to name a depth on the Heights page, say - lands on the datum plane
    instead. The frame is for looking at; its attachment is edited from the
    tree. So the datum planes are hidden and nothing in it is selectable in
    the view. Harmless without a GUI and on a plane already configured."""
    if not FreeCAD.GuiUp:
        return
    for child in getattr(workplane, "OriginFeatures", []) or []:
        view = getattr(child, "ViewObject", None)
        if view is None:
            continue
        if hasattr(view, "Selectable"):
            view.Selectable = False
        if getattr(child, "Role", "").endswith("_Plane"):
            view.Visibility = False


def createWorkplaneFromToolAxis(job, axis, origin=None, label=None):
    """createWorkplaneFromToolAxis(job, axis, origin=None, label=None) ... an
    unattached work plane whose +Z is axis. Convenience for callers that name
    a frame by direction alone, tests in particular."""
    return createWorkplane(job, label=label, placement=PathUtil.placementFromToolAxis(axis, origin))


def workplanesOf(job):
    """workplanesOf(job) ... the Job's named work planes, in group order."""
    group = getattr(job, "Workplanes", None)
    return list(getattr(group, "Group", []) or [])


def operationsUsing(job, workplane):
    """operationsUsing(job, workplane) ... operations of job linked to workplane."""
    return [
        op
        for op in job.Operations.Group
        if getattr(op, "Workplane", None) is not None and op.Workplane == workplane
    ]
