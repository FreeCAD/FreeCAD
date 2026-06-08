# SPDX-License-Identifier: LGPL-2.1-or-later

"""Runtime Draft view-policy registry and resolution helpers."""

import collections as coll
from dataclasses import dataclass

DOCUMENT_CONTEXT_PROPERTY = "Context"


class GridVisibility:
    """Visibility modes that one Draft view policy can request for the grid."""

    HIDDEN = "hidden"
    DURING_COMMAND = "during_command"
    ALWAYS = "always"


@dataclass(slots=True)
class DraftViewPolicy:
    """Runtime grid policy overrides resolved from document and workbench state."""

    spacing: float | None = None
    mainlines: int | None = None
    size: int | None = None
    visibility: str | None = None

    def as_overrides(self):
        return {
            "space": self.spacing,
            "mainlines": self.mainlines,
            "numlines": self.size,
        }

    def overlay(self, other):
        for attr in ("spacing", "mainlines", "size", "visibility"):
            value = getattr(other, attr)
            if value is not None:
                setattr(self, attr, value)
        return self


def get_document_view_context(document):
    if document is None:
        return ""
    return getattr(document, DOCUMENT_CONTEXT_PROPERTY, "")


class DraftViewPolicyRegistry:
    """Stores and resolves Draft view policies for context and tool scopes."""

    def __init__(self):
        self._context_policies = {}
        self._active_policies = coll.OrderedDict()

    def resolve(self, document):
        profile = self._get_context_policy(document)
        if not self._active_policies:
            return profile
        if profile is None:
            profile = DraftViewPolicy()
        for active_policy in self._active_policies.values():
            profile.overlay(active_policy)
        return profile

    def register_context_policy(self, context, owner, policy):
        policies = self._get_context_policies(context, create=True)
        policies[owner] = policy
        policies.move_to_end(owner)

    def clear_context_policy(self, context, owner):
        policies = self._get_context_policies(context)
        if not policies or owner not in policies:
            return
        del policies[owner]
        if not policies:
            self._context_policies.pop(context, None)

    def push_policy(self, owner, policy):
        self._active_policies[owner] = policy
        self._active_policies.move_to_end(owner)

    def pop_policy(self, owner):
        self._active_policies.pop(owner, None)

    def _get_context_policies(self, context, create=False):
        if not context:
            return None
        if create:
            return self._context_policies.setdefault(context, coll.OrderedDict())
        return self._context_policies.get(context)

    def _get_context_policy(self, document):
        policies = self._get_context_policies(get_document_view_context(document))
        if not policies:
            return None
        profile = DraftViewPolicy()
        for active_policy in policies.values():
            profile.overlay(active_policy)
        return profile


registry = DraftViewPolicyRegistry()
