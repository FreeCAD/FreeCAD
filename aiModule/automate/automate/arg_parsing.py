# Copied and Modified from PyTorch Lightning
#
# Original Copyright Notice:
#
# ========================================================================
# Copyright The PyTorch Lightning team.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ========================================================================


import inspect
import os
from abc import ABC
from argparse import _ArgumentGroup, ArgumentParser, Namespace
from contextlib import suppress
from functools import wraps
from typing import Any, Callable, Dict, List, Tuple, Type, Union



def str_to_bool_or_str(val: str) -> Union[str, bool]:
    """Possibly convert a string representation of truth to bool. Returns the input otherwise. Based on the python
    implementation distutils.utils.strtobool.
    True values are 'y', 'yes', 't', 'true', 'on', and '1'; false values are 'n', 'no', 'f', 'false', 'off', and '0'.
    """
    lower = val.lower()
    if lower in ("y", "yes", "t", "true", "on", "1"):
        return True
    if lower in ("n", "no", "f", "false", "off", "0"):
        return False
    return val


def str_to_bool(val: str) -> bool:
    """Convert a string representation of truth to bool.
    True values are 'y', 'yes', 't', 'true', 'on', and '1'; false values
    are 'n', 'no', 'f', 'false', 'off', and '0'.
    Raises:
        ValueError:
            If ``val`` isn't in one of the aforementioned true or false values.
    >>> str_to_bool('YES')
    True
    >>> str_to_bool('FALSE')
    False
    """
    val_converted = str_to_bool_or_str(val)
    if isinstance(val_converted, bool):
        return val_converted
    raise ValueError(f"invalid truth value {val_converted}")


def str_to_bool_or_int(val: str) -> Union[bool, int, str]:
    """Convert a string representation to truth of bool if possible, or otherwise try to convert it to an int.
    >>> str_to_bool_or_int("FALSE")
    False
    >>> str_to_bool_or_int("1")
    True
    >>> str_to_bool_or_int("2")
    2
    >>> str_to_bool_or_int("abc")
    'abc'
    """
    val_converted = str_to_bool_or_str(val)
    if isinstance(val_converted, bool):
        return val_converted
    try:
        return int(val_converted)
    except ValueError:
        return val_converted

class ParseArgparserDataType(ABC):
    def __init__(self, *_: Any, **__: Any) -> None:
        pass

    @classmethod
    def parse_argparser(cls, args: "ArgumentParser") -> Any:
        pass


def from_argparse_args(
    cls: Type[ParseArgparserDataType], args: Union[Namespace, ArgumentParser], **kwargs: Any
) -> ParseArgparserDataType:
    """Create an instance from CLI arguments. Eventually use varibles from OS environement which are defined as
    ``"PL_<CLASS-NAME>_<CLASS_ARUMENT_NAME>"``.
    Args:
        cls: Lightning class
        args: The parser or namespace to take arguments from. Only known arguments will be
            parsed and passed to the :class:`Trainer`.
        **kwargs: Additional keyword arguments that may override ones in the parser or namespace.
            These must be valid Trainer arguments.
    Examples:
        >>> from pytorch_lightning import Trainer
        >>> parser = ArgumentParser(add_help=False)
        >>> parser = Trainer.add_argparse_args(parser)
        >>> parser.add_argument('--my_custom_arg', default='something')  # doctest: +SKIP
        >>> args = Trainer.parse_argparser(parser.parse_args(""))
        >>> trainer = Trainer.from_argparse_args(args, logger=False)
    """
    if isinstance(args, ArgumentParser):
        args = cls.parse_argparser(args)

    params = vars(args)

    # we only want to pass in valid Trainer args, the rest may be user specific
    valid_kwargs = inspect.signature(cls.__init__).parameters
    trainer_kwargs = {name: params[name] for name in valid_kwargs if name in params}
    trainer_kwargs.update(**kwargs)

    return cls(**trainer_kwargs)


def parse_argparser(cls: Type["pl.Trainer"], arg_parser: Union[ArgumentParser, Namespace]) -> Namespace:
    """Parse CLI arguments, required for custom bool types."""
    args = arg_parser.parse_args() if isinstance(arg_parser, ArgumentParser) else arg_parser

    types_default = {arg: (arg_types, arg_default) for arg, arg_types, arg_default in get_init_arguments_and_types(cls)}

    modified_args = {}
    for k, v in vars(args).items():
        if k in types_default and v is None:
            # We need to figure out if the None is due to using nargs="?" or if it comes from the default value
            arg_types, arg_default = types_default[k]
            if bool in arg_types and isinstance(arg_default, bool):
                # Value has been passed as a flag => It is currently None, so we need to set it to True
                # We always set to True, regardless of the default value.
                # Users must pass False directly, but when passing nothing True is assumed.
                # i.e. the only way to disable something that defaults to True is to use the long form:
                # "--a_default_true_arg False" becomes False, while "--a_default_false_arg" becomes None,
                # which then becomes True here.

                v = True

        modified_args[k] = v
    return Namespace(**modified_args)


def parse_env_variables(cls: Type["pl.Trainer"], template: str = "PL_%(cls_name)s_%(cls_argument)s") -> Namespace:
    """Parse environment arguments if they are defined.
    Examples:
        >>> from pytorch_lightning import Trainer
        >>> parse_env_variables(Trainer)
        Namespace()
        >>> import os
        >>> os.environ["PL_TRAINER_GPUS"] = '42'
        >>> os.environ["PL_TRAINER_BLABLABLA"] = '1.23'
        >>> parse_env_variables(Trainer)
        Namespace(gpus=42)
        >>> del os.environ["PL_TRAINER_GPUS"]
    """
    cls_arg_defaults = get_init_arguments_and_types(cls)

    env_args = {}
    for arg_name, _, _ in cls_arg_defaults:
        env = template % {"cls_name": cls.__name__.upper(), "cls_argument": arg_name.upper()}
        val = os.environ.get(env)
        if not (val is None or val == ""):
            # todo: specify the possible exception
            with suppress(Exception):
                # converting to native types like int/float/bool
                val = eval(val)
            env_args[arg_name] = val
    return Namespace(**env_args)


def get_init_arguments_and_types(cls: Any) -> List[Tuple[str, Tuple, Any]]:
    r"""Scans the class signature and returns argument names, types and default values.
    Returns:
        List with tuples of 3 values:
        (argument name, set with argument types, argument default value).
    Examples:
        >>> from pytorch_lightning import Trainer
        >>> args = get_init_arguments_and_types(Trainer)
    """
    cls_default_params = inspect.signature(cls).parameters
    name_type_default = []
    for arg in cls_default_params:
        arg_type = cls_default_params[arg].annotation
        arg_default = cls_default_params[arg].default
        try:
            arg_types = tuple(arg_type.__args__)
        except (AttributeError, TypeError):
            arg_types = (arg_type,)

        name_type_default.append((arg, arg_types, arg_default))

    return name_type_default


def _get_abbrev_qualified_cls_name(cls: Any) -> str:
    assert isinstance(cls, type), repr(cls)
    if cls.__module__.startswith("pytorch_lightning."):
        # Abbreviate.
        return f"pl.{cls.__name__}"
    # Fully qualified.
    return f"{cls.__module__}.{cls.__qualname__}"


def add_argparse_args(
    cls: Type["pl.Trainer"], parent_parser: ArgumentParser, *, use_argument_group: bool = True
) -> Union[_ArgumentGroup, ArgumentParser]:
    r"""Extends existing argparse by default attributes for ``cls``.
    Args:
        cls: Lightning class
        parent_parser:
            The custom cli arguments parser, which will be extended by
            the class's default arguments.
        use_argument_group:
            By default, this is True, and uses ``add_argument_group`` to add
            a new group.
            If False, this will use old behavior.
    Returns:
        If use_argument_group is True, returns ``parent_parser`` to keep old
        workflows. If False, will return the new ArgumentParser object.
    Only arguments of the allowed types (str, float, int, bool) will
    extend the ``parent_parser``.
    Raises:
        RuntimeError:
            If ``parent_parser`` is not an ``ArgumentParser`` instance
    Examples:
        >>> # Option 1: Default usage.
        >>> import argparse
        >>> from pytorch_lightning import Trainer
        >>> parser = argparse.ArgumentParser()
        >>> parser = Trainer.add_argparse_args(parser)
        >>> args = parser.parse_args([])
        >>> # Option 2: Disable use_argument_group (old behavior).
        >>> import argparse
        >>> from pytorch_lightning import Trainer
        >>> parser = argparse.ArgumentParser()
        >>> parser = Trainer.add_argparse_args(parser, use_argument_group=False)
        >>> args = parser.parse_args([])
    """
    if isinstance(parent_parser, _ArgumentGroup):
        raise RuntimeError("Please only pass an ArgumentParser instance.")
    if use_argument_group:
        group_name = _get_abbrev_qualified_cls_name(cls)
        parser: Union[_ArgumentGroup, ArgumentParser] = parent_parser.add_argument_group(group_name)
    else:
        parser = ArgumentParser(parents=[parent_parser], add_help=False)

    ignore_arg_names = ["self", "args", "kwargs"]
    if hasattr(cls, "get_deprecated_arg_names"):
        ignore_arg_names += cls.get_deprecated_arg_names()

    allowed_types = (str, int, float, bool)

    # Get symbols from cls or init function.
    for symbol in (cls, cls.__init__):
        args_and_types = get_init_arguments_and_types(symbol)
        args_and_types = [x for x in args_and_types if x[0] not in ignore_arg_names]
        if len(args_and_types) > 0:
            break

    args_help = _parse_args_from_docstring(cls.__init__.__doc__ or cls.__doc__ or "")

    for arg, arg_types, arg_default in args_and_types:
        arg_types = tuple(at for at in allowed_types if at in arg_types)
        if not arg_types:
            # skip argument with not supported type
            continue
        arg_kwargs: Dict[str, Any] = {}
        if bool in arg_types:
            arg_kwargs.update(nargs="?", const=True)
            # if the only arg type is bool
            if len(arg_types) == 1:
                use_type: Callable[[str], Union[bool, int, float, str]] = str_to_bool
            elif int in arg_types:
                use_type = str_to_bool_or_int
            elif str in arg_types:
                use_type = str_to_bool_or_str
            else:
                # filter out the bool as we need to use more general
                use_type = [at for at in arg_types if at is not bool][0]
        else:
            use_type = arg_types[0]

        if arg == "gpus" or arg == "tpu_cores":
            use_type = _gpus_allowed_type

        # hack for types in (int, float)
        if len(arg_types) == 2 and int in set(arg_types) and float in set(arg_types):
            use_type = _int_or_float_type

        # hack for track_grad_norm
        if arg == "track_grad_norm":
            use_type = float

        # hack for precision
        if arg == "precision":
            use_type = _precision_allowed_type

        parser.add_argument(
            f"--{arg}", dest=arg, default=arg_default, type=use_type, help=args_help.get(arg), **arg_kwargs
        )

    if use_argument_group:
        return parent_parser
    return parser


def _parse_args_from_docstring(docstring: str) -> Dict[str, str]:
    arg_block_indent = None
    current_arg = ""
    parsed = {}
    for line in docstring.split("\n"):
        stripped = line.lstrip()
        if not stripped:
            continue
        line_indent = len(line) - len(stripped)
        if stripped.startswith(("Args:", "Arguments:", "Parameters:")):
            arg_block_indent = line_indent + 4
        elif arg_block_indent is None:
            continue
        elif line_indent < arg_block_indent:
            break
        elif line_indent == arg_block_indent:
            current_arg, arg_description = stripped.split(":", maxsplit=1)
            parsed[current_arg] = arg_description.lstrip()
        elif line_indent > arg_block_indent:
            parsed[current_arg] += f" {stripped}"
    return parsed


def _gpus_allowed_type(x: str) -> Union[int, str]:
    if "," in x:
        return str(x)
    return int(x)


def _int_or_float_type(x: Union[int, float, str]) -> Union[int, float]:
    if "." in str(x):
        return float(x)
    return int(x)


def _precision_allowed_type(x: Union[int, str]) -> Union[int, str]:
    """
    >>> _precision_allowed_type("32")
    32
    >>> _precision_allowed_type("bf16")
    'bf16'
    """
    try:
        return int(x)
    except ValueError:
        return x


def _defaults_from_env_vars(fn: Callable) -> Callable:
    @wraps(fn)
    def insert_env_defaults(self: Any, *args: Any, **kwargs: Any) -> Any:
        cls = self.__class__  # get the class
        if args:  # in case any args passed move them to kwargs
            # parse only the argument names
            cls_arg_names = [arg[0] for arg in get_init_arguments_and_types(cls)]
            # convert args to kwargs
            kwargs.update(dict(zip(cls_arg_names, args)))
        env_variables = vars(parse_env_variables(cls))
        # update the kwargs by env variables
        kwargs = dict(list(env_variables.items()) + list(kwargs.items()))

        # all args were already moved to kwargs
        return fn(self, **kwargs)

    return insert_env_defaults