# SPDX-License-Identifier: LGPL-2.1-or-later
"""Tool ownership and a compatibility bridge for the session API."""

import weakref


class ToolController:
    def __init__(self, session):
        self._session = weakref.ref(session)

    @property
    def session(self):
        session = self._session()
        if session is None:
            raise ReferenceError("The Forms edit session has closed")
        return session


class ToolField:
    def __init__(self, tool, name):
        self.tool, self.name = tool, name

    def __get__(self, instance, owner=None):
        if instance is None:
            return self
        return getattr(instance._tool(self.tool), self.name)

    def __set__(self, instance, value):
        setattr(instance._tool(self.tool), self.name, value)


class ToolMethod:
    def __init__(self, tool, name):
        self.tool, self.name = tool, name

    def __get__(self, instance, owner=None):
        if instance is None:
            def unbound(session, *args, **kwargs):
                return getattr(session._tool(self.tool), self.name)(*args, **kwargs)
            return unbound
        return getattr(instance._tool(self.tool), self.name)

    def __set__(self, instance, value):
        # Preserve session-level overrides used by callers and interaction tests.
        setattr(instance._tool(self.tool), self.name, value)
