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
QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP

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


def createWorkplane(job, base=None, sub=None, label=None, placement=None):
    """createWorkplane(job, base=None, sub=None, label=None, placement=None)
    ... add a named work plane to job and return it.

    With *base* and *sub* naming a planar face, the plane is attached to it in
    FlatFace mode: its +Z is the face normal and it follows the face if the
    model changes. *base* may be the user's original object or the Job's clone
    of it; it is resolved to the clone either way.

    With *placement* and no face, the plane is created unattached at that
    placement. With neither it sits at the Job origin, aligned with the Job's
    axes, for the user to position by hand.

    Any plane can be created on any Job. Whether the machine can reach it
    is not decided here: the operation refuses to solve a tilted plane
    without rotary axes, and the post refuses to emit one, each at the
    moment it matters and naming what is missing. A plane parallel to the
    table - a datum for depths, a turned X - is useful on every machine.

    The plane carries an optional Fixture, empty by default: see
    ensureFixtureProperty()."""
    doc = job.Document
    workplane = doc.addObject("Part::LocalCoordinateSystem", "Workplane")
    workplane.Label = label or "Workplane"
    ensureFixtureProperty(workplane)

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

    if base is not None and sub:
        # FlatFace puts the origin where the global origin projects onto the
        # face, which for a tilted face is usually off the face entirely. A
        # machinist sets zero on the feature: default it to the face's
        # centroid, as an offset in the attached frame so it still follows
        # the face. Users move it from there with the attachment editor.
        try:
            face = model.Shape.getElement(sub)
            local = workplane.Placement.inverse().multVec(face.CenterOfMass)
            workplane.AttachmentOffset = FreeCAD.Placement(local, FreeCAD.Rotation())
            doc.recompute()
        except Exception as e:
            Path.Log.warning("Could not place the work plane origin on the face: %s" % e)

    return workplane


FixtureProperty = "Fixture"


def ensureFixtureProperty(workplane):
    """ensureFixtureProperty(workplane) ... give workplane its Fixture property
    if it lacks one, as a plane from an older document does.

    A work plane may name the Fixture - the work coordinate system, G54 to
    G59.9 or G54.1 Pn - the control is to have selected while its
    operations run. Empty, the default, means the Job's own Fixture: the
    plane is expressed in whatever coordinate system the Job's output is
    in. Set, the post selects it before positioning for the plane and
    returns to the Job's Fixture afterwards. The Job's Fixtures list is
    untouched by this: that list repeats the program for several parts,
    and a plane's Fixture is selected inside each repetition."""
    if not hasattr(workplane, FixtureProperty):
        workplane.addProperty(
            "App::PropertyString",
            FixtureProperty,
            "Workplane",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Work coordinate system (G54-G59.9, G54.1 Pn) selected while operations on "
                "this plane run. Empty: the Job's own fixture.",
            ),
        )
    return workplane


def fixtureOf(workplane):
    """fixtureOf(workplane) ... the Fixture a work plane names, or None."""
    if workplane is None:
        return None
    value = (getattr(workplane, FixtureProperty, "") or "").strip()
    return value or None


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
