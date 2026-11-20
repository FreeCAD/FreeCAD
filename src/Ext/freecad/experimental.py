# SPDX-License-Identifier: LGPL-2.1-or-later

"""
Python API metadata decorators for experimental features.
"""

import functools

import FreeCAD as App


def experimental(reason: str | None = None):
    """
    A decorator that marks functions, methods, and classes as experimental.

    - Functions/Methods: Emits a warning when called.
    - Classes: Emits a warning when instantiated.

    Args:
        reason (str, optional): Additional context or explanation for why it's experimental.
    """

    def decorator(target):
        target_name = getattr(target, "__name__", "Object")
        msg = f"'{target_name}' is experimental and may change or be removed in future versions."
        if reason:
            msg += f" Reason: {reason}"
        msg += "\n"

        if isinstance(target, type):
            original_init = target.__dict__.get("__init__")

            if original_init is not None:

                @functools.wraps(original_init)
                def wrapped_init(self, *args, **kwargs):
                    App.Console.PrintDeveloperWarning(msg)
                    return original_init(self, *args, **kwargs)

                target.__init__ = wrapped_init
            else:

                def wrapped_init(self, *args, **kwargs):
                    App.Console.PrintDeveloperWarning(msg)
                    super(target, self).__init__(*args, **kwargs)  # ty: ignore[invalid-super-argument]

                target.__init__ = wrapped_init

            return target

        # Handle Functions and Methods: Warn upon call
        @functools.wraps(target)
        def wrapper(*args, **kwargs):
            App.Console.PrintDeveloperWarning(msg)
            return target(*args, **kwargs)

        return wrapper

    # Support both @experimental and @experimental(reason="...") syntax
    if callable(reason):
        func = reason
        reason = None
        return decorator(func)

    return decorator
