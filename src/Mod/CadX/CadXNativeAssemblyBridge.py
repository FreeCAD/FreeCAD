# SPDX-License-Identifier: LGPL-2.1-or-later

"""Small Python seam for the native CadX Assembly constraint operations.

The C++ coordinator owns validation, document transactions, recompute,
postconditions, graph publication, and audit.  This module exists only
because FreeCAD Assembly's exact JointObject proxy and view-provider
registration are Python-backed.
"""

from __future__ import annotations

import FreeCAD as App


def _document(name):
    document = App.getDocument(name)
    if document is None:
        raise RuntimeError("document is unavailable")
    return document


def _assembly(document, name):
    assembly = document.getObject(name)
    if assembly is None or str(getattr(assembly, "TypeId", "")) != "Assembly::AssemblyObject":
        raise RuntimeError("assembly is not an Assembly::AssemblyObject")
    return assembly


def _joint_group(assembly):
    import UtilsAssembly

    group = UtilsAssembly.getJointGroup(assembly)
    if group is None:
        raise RuntimeError("Assembly has no JointGroup")
    return group


def create_grounded_joint(document_name, assembly_name, component_name):
    """Create the canonical GroundedJoint proxy and view provider."""

    import JointObject

    document = _document(document_name)
    assembly = _assembly(document, assembly_name)
    component = document.getObject(component_name)
    if component is None:
        raise RuntimeError("grounding component is unavailable")
    joint = _joint_group(assembly).newObject("App::FeaturePython", "GroundedJoint")
    if joint is None:
        raise RuntimeError("FreeCAD could not create GroundedJoint")
    JointObject.GroundedJoint(joint, component)
    if hasattr(JointObject, "ViewProviderGroundedJoint"):
        JointObject.ViewProviderGroundedJoint(joint.ViewObject)
    joint.recompute()
    return str(joint.Name)


def create_regular_joint(
    document_name,
    assembly_name,
    joint_type,
    label,
    first_component,
    first_connector,
    second_component,
    second_connector,
    reverse,
):
    """Create and initialize the canonical Joint proxy/view provider.

    ``setJointConnectors`` and ``flipOnePart`` are proxy operations rather
    than graph or transaction policy; the C++ caller remains responsible for
    every surrounding mutation decision.
    """

    import JointObject
    import UtilsAssembly

    type_indexes = {"fixed": 0, "revolute": 1}
    if joint_type not in type_indexes:
        raise ValueError("unsupported joint type")
    document = _document(document_name)
    assembly = _assembly(document, assembly_name)
    first = document.getObject(first_component)
    second = document.getObject(second_component)
    if first is None or second is None:
        raise RuntimeError("joint connector component is unavailable")
    joint = _joint_group(assembly).newObject("App::FeaturePython", "Joint")
    if joint is None:
        raise RuntimeError("FreeCAD could not create Joint")
    joint.Label = label
    JointObject.Joint(joint, type_indexes[joint_type])
    JointObject.ViewProviderJoint(joint.ViewObject)
    joint.Proxy.setJointConnectors(
        joint,
        [[first, [first_connector]], [second, [second_connector]]],
    )
    if reverse:
        joint.Proxy.flipOnePart(joint)
    joint.recompute()
    return str(joint.Name)


def verify_grounded_proxy(document_name, joint_name):
    import JointObject

    joint = _document(document_name).getObject(joint_name)
    return bool(
        joint is not None
        and isinstance(getattr(joint, "Proxy", None), JointObject.GroundedJoint)
        and isinstance(
            getattr(getattr(joint, "ViewObject", None), "Proxy", None),
            JointObject.ViewProviderGroundedJoint,
        )
    )


def verify_regular_proxy(document_name, joint_name):
    import JointObject

    joint = _document(document_name).getObject(joint_name)
    return bool(
        joint is not None
        and isinstance(getattr(joint, "Proxy", None), JointObject.Joint)
        and isinstance(
            getattr(getattr(joint, "ViewObject", None), "Proxy", None),
            JointObject.ViewProviderJoint,
        )
    )
