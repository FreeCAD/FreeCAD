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

"""
The contract that each machine-based post must obey.

The tests find each shipped post that declares POST_TYPE = "machine" and run
the same checks on it through export2(). A new post gets these checks with no
new test code. A test for the output of one post goes in the test module of
that post, not here.

Each post gets its own test class, TestPostContract_<post>, so that the test
output shows which posts ran which checks.

Behaviour that a post inherits is tested one time, on the base PostProcessor
(TestPostContract_PostProcessor). The pipeline checks run on a post only if the
post replaces a pipeline method of the base class. A post that only sets
properties and defaults gets the checks of its configuration and one export.
"""

import Constants
import Path
from CAMTests import PathTestUtils
from CAMTests import PostTestMocks
from CAMTests import PostTestSupport
from Path.Post.CAMErrors import CAMError
from Path.Post.Processor import PostProcessor, VALID_SCOPES, WrapperPost, property_scope

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

# The name of the base PostProcessor in the list of pipeline subjects.
BASE = "PostProcessor"

# The base methods that set the properties and the defaults of a post. A post
# that replaces only these methods converts a Job as the base class does.
# export() is the entry of a legacy post, which these tests do not run, and
# remote_post() sends the output file to a machine.
_CONFIGURATION_METHODS = frozenset(
    {
        "get_common_property_schema",
        "get_property_schema",
        "init_values",
        "init_arguments",
        "init_arguments_visible",
        "process_arguments",
        "export",
        "remote_post",
        "tooltip",
        "tooltipArgs",
        "units",
        "get_sanity_checks",
        "sanity_check_methods",
    }
)


def pipeline_overrides(post_class):
    """The names of the base pipeline methods that a post class replaces."""
    names = set()
    for klass in post_class.__mro__:
        if klass is PostProcessor:
            break
        names.update(
            name
            for name in vars(klass)
            if not name.startswith("__") and hasattr(PostProcessor, name)
        )
    return names - _CONFIGURATION_METHODS


_DRILL = {"X": 5.0, "Y": 5.0, "Z": -5.0, "R": 2.0, "F": 100.0}

# The parameters that make each command a valid one. A command that is not in
# this table needs no parameters.
_SAMPLE_PARAMETERS = {
    "G0": {"X": 1.0, "Y": 2.0, "Z": 3.0},
    "G1": {"X": 4.0, "Y": 5.0, "Z": -1.0, "F": 100.0},
    "G2": {"X": 10.0, "Y": 0.0, "I": 5.0, "J": 0.0, "F": 100.0},
    "G3": {"X": 10.0, "Y": 0.0, "I": 5.0, "J": 0.0, "F": 100.0},
    "G4": {"P": 1.0},
    "G38.2": {"Z": -5.0, "F": 50.0},
    "G43": {"H": 1.0},
    "G73": {**_DRILL, "Q": 1.0},
    "G74": {**_DRILL, "F": 1.25, "S": 500.0},
    "G81": _DRILL,
    "G82": {**_DRILL, "P": 1.0},
    "G83": {**_DRILL, "Q": 1.0},
    "G84": {**_DRILL, "F": 1.25, "S": 500.0},
    "G85": _DRILL,
    "G88": {**_DRILL, "P": 1.0},
    "G89": {**_DRILL, "P": 1.0},
    "G92": {"X": 0.0, "Y": 0.0, "Z": 0.0},
    "G96": {"S": 100.0},
    "G97": {"S": 1000.0},
    "M3": {"S": 1000.0},
    "M4": {"S": 1000.0},
    "M6": {"T": 2.0},
}

# The rapid move that an operation writes first. The rapid feed is necessary:
# a post that expands a drill cycle reads it.
_APPROACH = "G0 X0 Y0 Z10 F1000"

# The program for the tests that do not look at single commands.
_PROGRAM = [
    _APPROACH,
    "G1 X10 Y0 Z-1 F100",
    "G2 X20 Y10 I0 J10",
    "G3 X10 Y20 I-10 J0",
    "G1 X0 Y20",
    "G0 Z10",
]


def _each_output_option(machine):
    """Set the output options that change how the post writes a line."""
    machine.output.output_header = True
    machine.output.comments.enabled = True
    machine.output.formatting.line_numbers = True
    machine.output.duplicates.commands = False
    machine.output.duplicates.parameters = False
    machine.processing.filter_inefficient_moves = True
    machine.processing.tool_change_format = Constants.ToolChangeFormat.M6_T
    machine.processing.xy_before_z_after_tool_change = True


# The machines for the test of single commands: a label and a function that
# changes the default machine.
_MACHINE_VARIANTS = (
    ("default", None),
    ("each output option", _each_output_option),
)


def _canonical(name):
    """G01 and G1 are one command: the name with no leading zero."""
    if len(name) > 2 and name[1] == "0" and name[2].isdigit():
        return name[0] + name[2:]
    return name


def sample_command(name):
    """A valid Path.Command for a command name, as an operation writes it."""
    command = Path.Command(name, dict(_SAMPLE_PARAMETERS.get(_canonical(name), {})))
    if name in Constants.GCODE_MOVE_TAP:
        command.addAnnotations({"operation": "tapping", "rigid": "False"})
    return command


class _ContractBase:
    """The post under test. A generated class sets POST_NAME."""

    POST_NAME = None

    def _subject(self, commands=(), configure=None):
        """A new post on a new mock job with a default machine.

        Returns the post and the operation that holds its path.
        """
        job, profile_op, _ = PostTestMocks.create_default_job_with_operation()
        profile_op.Path = Path.Path(PostTestSupport.as_commands(commands))
        if self.POST_NAME == BASE:
            post = PostProcessor(job, "", "", "mm")
        else:
            post = PostTestSupport.load_post(job, self.POST_NAME)
        machine = PostTestSupport.make_machine(f"Test {self.POST_NAME}")
        # The date is the one part of the default output that changes with each run.
        machine.output.header.include_date = False
        if configure is not None:
            configure(machine)
        post._machine = machine
        post.apply_configuration_bundle()
        return post, profile_op

    def _post(self, commands=(), configure=None):
        return self._subject(commands, configure)[0]

    def test_export_gives_output(self):
        """export2() of a small program returns (name, text) sections with text in them."""
        sections = self._post(_PROGRAM).export2()
        self.assertTrue(sections)
        for section in sections:
            self.assertEqual(2, len(section))
            self.assertIsInstance(section[1], str)
        self.assertTrue(PostTestSupport.all_gcode(sections).strip())


class _ConfigurationContract(_ContractBase):
    """The checks of the properties and the defaults of a post. Each post gets them."""

    def test_post_loads_as_a_machine_post(self):
        self.assertNotIsInstance(self._post(), WrapperPost)

    def test_property_schema_is_well_formed(self):
        """Each property has a name, a valid scope and a default that is one of its choices."""
        names = set()
        for prop in self._post().get_full_property_schema():
            with self.subTest(property=prop.get("name")):
                self.assertTrue(prop.get("name"))
                self.assertNotIn(prop["name"], names, "the name is in the schema twice")
                names.add(prop["name"])
                self.assertIn(property_scope(prop), VALID_SCOPES)
                if prop.get("type") == "choice":
                    self.assertIn(prop["default"], prop["choices"])

    def test_schema_properties_reach_the_values(self):
        """The post can read each schema property from its values after configuration."""
        post = self._post()
        for prop in post.get_full_property_schema():
            with self.subTest(property=prop["name"]):
                self.assertIn(prop["name"].upper(), post.values)

    def test_file_extension(self):
        """The file extension is text with no leading dot."""
        extension = self._post().get_file_extension()
        self.assertIsInstance(extension, str)
        self.assertTrue(extension)
        self.assertFalse(extension.startswith("."), extension)


class _PipelineContract(_ContractBase):
    """The checks of the conversion.

    The base PostProcessor gets them, and each post that replaces a pipeline method.
    """

    def test_output_is_deterministic(self):
        """Two posts of the same name give the same text for the same program."""
        first = PostTestSupport.all_gcode(self._post(_PROGRAM).export2())
        second = PostTestSupport.all_gcode(self._post(_PROGRAM).export2())
        self.assertEqual(first, second)

    def test_second_export_gives_the_same_output(self):
        """A post keeps no state from one export2() to the next."""
        post = self._post(_PROGRAM)
        first = PostTestSupport.all_gcode(post.export2())
        second = PostTestSupport.all_gcode(post.export2())
        self.assertEqual(first, second)

    def test_each_command_is_converted_or_refused(self):
        """A post converts a command that CAM can give it, or refuses it with a CAMError.

        Any other exception is a defect in the post.
        """
        allowed = sorted(
            set(
                Constants.GCODE_SUPPORTED
                + Constants.GCODE_FIXTURES
                + Constants.MCODE_SUPPORTED
                + Constants.GCODE_NON_CONFORMING
            )
        )
        for label, configure in _MACHINE_VARIANTS:
            post, profile_op = self._subject(configure=configure)
            for command_name in allowed:
                with self.subTest(machine=label, command=command_name):
                    profile_op.Path = Path.Path(
                        PostTestSupport.as_commands([_APPROACH, sample_command(command_name)])
                    )
                    post.reinitialize()
                    post.apply_configuration_bundle()
                    try:
                        post.export2()
                    except CAMError:
                        pass


def _replaces_pipeline(name):
    """True if the post of this name replaces a pipeline method of the base class."""
    try:
        post = PostTestSupport.load_post(PostTestMocks.MockJob(), name)
    except Exception:
        # The post did not load. Its configuration class reports that as a test failure.
        return False
    return bool(pipeline_overrides(type(post)))


def _contract_class(name, *contracts):
    """A test class that runs the contracts on one post."""
    return type(
        f"TestPostContract_{name}",
        contracts + (PathTestUtils.PathTestBase,),
        {
            "POST_NAME": name,
            "__module__": __name__,
            "__doc__": f"The post contract for {name}: "
            + ", ".join(contract.__name__.strip("_") for contract in contracts)
            + ".",
        },
    )


POST_NAMES = PostTestSupport.machine_post_names()
PIPELINE_POST_NAMES = [name for name in POST_NAMES if _replaces_pipeline(name)]

# The test loader finds the test classes in the module namespace.
globals()[f"TestPostContract_{BASE}"] = _contract_class(BASE, _PipelineContract)
for _name in POST_NAMES:
    if _name in PIPELINE_POST_NAMES:
        _class = _contract_class(_name, _ConfigurationContract, _PipelineContract)
    else:
        _class = _contract_class(_name, _ConfigurationContract)
    globals()[_class.__name__] = _class
del _name, _class


class TestPostContractSubjects(PathTestUtils.PathTestBase):
    """The search for the posts gives the posts that the contract classes test."""

    def test_posts_are_found(self):
        """An empty list gives no contract classes, and no test fails."""
        self.assertIn("generic", POST_NAMES)
        self.assertIn("linuxcnc", POST_NAMES)

    def test_pipeline_posts(self):
        """A post gets the pipeline checks only if it replaces a pipeline method."""
        # generic sets properties only; linuxcnc replaces _convert_drill_cycle().
        self.assertNotIn("generic", PIPELINE_POST_NAMES)
        self.assertIn("linuxcnc", PIPELINE_POST_NAMES)
