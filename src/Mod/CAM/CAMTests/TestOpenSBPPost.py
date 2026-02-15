# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2022 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2023 Larry Woestman <LarryWoestman2@gmail.com>          *
# *   Copyright (c) 2025 Alan Grover <awgrover@gmail.com>
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import re

import unittest
import FreeCAD
from FreeCAD import Units

import Path
from CAMTests import PathTestUtils
from Path.Post.scripts import opensbp_post as postprocessor
from Path.Post.Processor import PostProcessorFactory


Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())

nl = "\n"

# for testing, the tool is set to:
FeedSpeed = 700  # mm/min
RapidSpeed = FeedSpeed * 3
# verticals are /2


class TestOpenSBPPost(PathTestUtils.PathTestBase):
    """NB: the post-processor has globals,
    which are not reset for each .assertX,
    e.g. for the arguments.
    So, you have to deal with state, e.g. --inches is persistent till the next testX()
    """

    @classmethod
    def setUpClass(cls):
        """setUpClass()...
        This method is called upon instantiation of this test class.  Add code
        and objects here that are needed for the duration of the test() methods
        in this class.  In other words, set up the 'global' test environment
        here; use the `setUp()` method to set up a 'local' test environment.
        This method does not have access to the class `self` reference, but it
        is able to call static methods within this same class.
        """

        # Open existing FreeCAD document with test geometry
        cls.doc = FreeCAD.open(FreeCAD.getHomePath() + "/Mod/CAM/CAMTests/boxtest.fcstd")

    @classmethod
    def fixup_test_context(cls):
        # we fixup the tool-controllers for feed rates
        # we setup setupsheet which will get all the tools for rapid
        for tool_controller in cls.job.Tools.Group:

            # shopbot native is mm/s, freecad is mm/min
            if tool_controller.HorizFeed.Value == 0.0:
                tool_controller.HorizFeed = Units.Quantity(FeedSpeed, "mm/min")
            if tool_controller.VertFeed.Value == 0.0:
                tool_controller.VertFeed = Units.Quantity(FeedSpeed / 2, "mm/min")
            # settings = { p:getattr(tool_controller, p) for p in ['HorizFeed','VertFeed','HorizRapid','VertRapid'] }
            # print(f"### tc setup {settings}")
        setup_sheet = cls.job.SetupSheet
        if setup_sheet.HorizRapid.Value == 0.0:
            setup_sheet.HorizRapid = Units.Quantity(RapidSpeed, "mm/min")
        if setup_sheet.VertRapid.Value == 0.0:
            setup_sheet.VertRapid = Units.Quantity(RapidSpeed / 2, "mm/min")
        cls.doc.recompute()

        if len(cls.job.Operations.Group) > 0:
            # restrict to one job
            cls.job.Operations.Group = [cls.job.Operations.Group[0]]
            # remember the "Profile" operation
            cls.profile_op = cls.job.Operations.Group[0]
            print(f"### Operation {cls.job.Label}.{cls.profile_op.Label}")

    @classmethod
    def tearDownClass(cls):
        """tearDownClass()...
        This method is called prior to destruction of this test class.  Add
        code and objects here that cleanup the test environment after the
        test() methods in this class have been executed.  This method does
        not have access to the class `self` reference.  This method is able
        to call static methods within this same class.
        """
        # Close geometry document without saving
        for name in FreeCAD.listDocuments().keys():
            FreeCAD.closeDocument(name)
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")

    # Setup and tear down methods called before and after each unit test
    def setUp(self):
        """setUp()...
        This method is called prior to each `test()` method.  Add code and
        objects here that are needed for multiple `test()` methods.
        """
        self.maxDiff = None
        self.doc.UnitSystem = "Metric small parts & CNC (mm, mm/min)"

        # in case someone chooses a different job
        self.__class__.job = self.doc.getObject("Job")
        self.__class__.post = PostProcessorFactory.get_post_processor(self.job, "opensbp")
        self.__class__.fixup_test_context()

        self.post.reinitialize()

        postprocessor.UNDER_UNITTEST = (
            True  # because we don't setup a tool-controller in these tests
        )

    def tearDown(self):
        """tearDown()...
        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        """
        if FreeCAD.ActiveDocument and FreeCAD.ActiveDocument.findObjects(Name="testpath"):
            FreeCAD.ActiveDocument.removeObject("testpath")

    def compare_multi(self, *args, remove=None, debug=False, skip=False):
        """Actually as if: ( *gcode, options, expected, debug=True )
        `*gcode` is all str (gcodes), or None to use extant self.job
        `remove` is a pattern to remove lines from both expected and generated
        """

        if skip:
            return

        if args[0] is None:
            self.profile_op.Path = Path.Path([])
        elif isinstance(args[0], str):
            try:
                self.profile_op.Path = Path.Path([Path.Command(x) for x in args[:-2]])
            except ValueError as e:
                try:
                    i = None  # hack: we use the i from the 'for' in the `except`
                    for i, x in enumerate(args[:-2]):
                        Path.Command(x)
                except ValueError:
                    raise ValueError(f"for [{i}] '{x}'") from e
        else:
            # assume Path.Commands
            self.profile_op.Path = Path.Path(args[:-2])
        post_args = args[-2]
        expected = args[-1]

        # need the --.... line
        if post_args != "" and post_args is not None:
            expected = expected.format(ARGS=f"'  {post_args}")

        # print(f"###test multi path str {args[:-2]}")
        # print(f"###test in gcode {[p.toGCode() for p in self.profile_op.Path.Commands]}")
        self.job.PostProcessorArgs = post_args

        rez = self.post.export()
        if rez is None:
            raise Exception("Error processing arguments")
        else:
            _, gcode = rez[0]
        # print(f"### gcode {gcode}")

        if debug:
            print(f"--------{nl}{gcode}--------{nl}")
        if remove:
            expected = nl.join((x for x in expected.split("\n") if not re.match(remove, x)))
            gcode = nl.join((x for x in gcode.split(nl) if not re.match(remove, x)))
        print(f"###E---{expected}---")
        print(f"###G---{gcode}---")
        self.assertEqual(
            expected, gcode
        )  # most other tests have this reversed, and the diff reads wrong to me

    def wrap(
        self,
        expected,
        inches=None,
        preamble="",
        postamble="",
        nativepre="",
        comments=False,
        header=False,
        precision=None,
        postfix="",
        JS=None,  # set to explicit command that isn't our test default
        noHorizRapid=False,
        noVertRapid=False,  # which "no" comment is expected
    ):
        # compare_multi helper
        # wraps the expected path-gcode in std prefix, postfix, no-header
        # make `comments` match with --comments or --no-comments
        # lots-o-fun special cases...

        # hack'ish: adapt to precision
        if precision is not None:
            z = "0" * precision
        elif m := re.search(r"\.(\d+)", expected):
            z = "0" * len(m.group(1))
        else:
            z = "0" * 3

        fmt = lambda v: format(v, f"0.{len(z)}f")
        speed_precision = 2 if inches else 1
        speed_fmt = lambda v: format(v, f"0.{speed_precision}f")

        # remember we are x/s and freecad is x/min (usually)
        speeds = []
        if comments:
            speeds.append("'set speeds: TC: Default Tool")
        if inches:
            speeds.append(f"MS,{speed_fmt(FeedSpeed/60/25.4)},{speed_fmt(FeedSpeed/2/60/25.4)}")
            if JS is None:
                speeds.append(
                    f"JS,{speed_fmt(RapidSpeed/60/25.4)},{speed_fmt(RapidSpeed/2/60/25.4)}"
                )
        else:
            speeds.append(f"MS,{speed_fmt(FeedSpeed/60)},{speed_fmt(FeedSpeed/2/60)}")
            if JS is None:
                speeds.append(f"JS,{speed_fmt(RapidSpeed/60)},{speed_fmt(RapidSpeed/2/60)}")

        if noHorizRapid and comments:
            speeds.append("'no HorizRapid")
        if noVertRapid and comments:
            speeds.append("'no VertRapid")
        if JS is not None and JS != "":  # '' means skip
            speeds.append(JS)

        speeds = "\n".join(speeds)
        # print(f"T### speeds---{speeds}---")

        def ifcomments(text):
            if not comments:
                # remove comment lines
                # clean up multiple blank lines
                # clean up leading \n
                # clean up trailing \n and empty
                text = re.sub(r"^'.+$", "", text, flags=re.MULTILINE)
                text = re.sub(r"\n\n+", "\n", text)
                text = re.sub(r"^\n", "", text)
                if text != "" and not text.endswith("\n"):
                    text += "\n"
                elif text == "\n":
                    text = ""
            return text

        hdr = ""
        if header:
            hdr = """'(Exported by FreeCAD)
'(Post Processor: opensbp_post)
{ARGS}
'(Cam File: boxtest.fcstd)
'Job: Job
"""
        pre = [
            f"""{hdr}'(Begin preamble)
{preamble}SA
IF %(25) = {0 if not inches else 1} THEN GOTO WrongUnits
{nativepre}'(Begin operation: Fixture)
'(Machine units: mm/s)
'(Path: Fixture)
'(G54)
'(Finish operation: Fixture)
'(Begin operation: TC: Default Tool)
'(Machine units: mm/s)
'(Path: TC: Default Tool)
'(TC: Default Tool)
'(Begin toolchange)
C7
""",
            # don't remove these comments
            """'First change tool, should already be #1: TC Default Tool Endmill
&Tool=1
&ToolName="TC Default Tool Endmill"
""",
            f"""{speeds}
'(Finish operation: TC: Default Tool)
'(Begin operation: Profile)
'(Machine units: mm/s)
'(Path: Profile)
""",
        ]
        pre[0] = ifcomments(pre[0])
        pre[2] = ifcomments(pre[2])
        pre = "".join(pre)

        post = f"""'(Finish operation: Profile)
'(Begin postamble)
{postamble}{postfix}GOTO AfterWrongUnits
WrongUnits:
  if %(25) = 0 THEN &shopbot_which="inches"
  if %(25) = 1 THEN &shopbot_which="mm"
    MSGBOX("Post-processor wants {'G21/--metric' if not inches else 'G20/--inches'} but ShopBot is " & &shopbot_which & ". Change Units in ShopBot and try again.",0,"Change Units")
    ENDALL
AfterWrongUnits:
"""
        post = ifcomments(post)

        return f"""{pre}{expected}{post}"""

    def test000(self):
        """Test Output Generation.
        Empty path.  Produces only the preamble and postable.
        """

        self.profile_op.Path = Path.Path([])

        # Test generating with header
        # Header contains a time stamp that messes up diff.
        self.compare_multi(
            None,
            "--no-show-editor",
            self.wrap("", comments=True, header=True),
            remove=r"'\(Output Time:",
        )

        # Test without header

        self.compare_multi(None, "--no-header --no-comments --no-show-editor", self.wrap(""))

    def test005(self):
        """Test native-rapid
        default should skip the jog-speed setting.
        --no-native-rapid-fallback should cause an error.
        """
        setup_sheet = self.job.SetupSheet

        # 0's only for horizrapid
        setup_sheet.HorizRapid = Units.Quantity(0.0, "mm/min")
        self.doc.recompute()
        self.compare_multi(
            None,
            "--no-header --no-show-editor",
            self.wrap("", JS="JS,,17.5", noHorizRapid=True, noVertRapid=False, comments=True),
        )

        # 0's for both rapid:
        setup_sheet.HorizRapid = Units.Quantity(0.0, "mm/min")
        setup_sheet.VertRapid = Units.Quantity(0.0, "mm/min")
        self.doc.recompute()
        # nb: can't test --no-comments, because "noHorizRapid" is always inserted, and .wrap doesn't know that
        self.compare_multi(
            None,
            "--no-header --no-show-editor",
            self.wrap("", JS="", noHorizRapid=True, noVertRapid=True, comments=True),
        )

        with self.assertRaises(ValueError) as context:
            self.compare_multi(
                None, "--no-header --no-show-editor --no-native-rapid-fallback", "should raise"
            )
        self.assertTrue("did not set" in str(context.exception))

    def test010(self):
        """Test command Generation.
        Test Precision
        Test imperial / inches
        """

        # default is metric-mm (internal default)
        self.compare_multi(
            "G0 X10 Y20 Z30",  # simple rapid
            "--no-header --no-comments --no-show-editor --metric --no-native-rapid-fallback --no-abort-on-unknown",
            self.wrap(
                """J3,10.000,20.000,30.000
"""
            ),
        )

        self.compare_multi(
            "G0 X10 Y20 Z30",
            "--no-header --no-comments --precision=2 --no-show-editor",
            self.wrap(
                """J3,10.00,20.00,30.00
"""
            ),
        )

        self.compare_multi(
            "G0 X10 Y20 Z30",
            "--no-header --no-comments --inches --no-show-editor",
            self.wrap(
                """J3,0.3937,0.7874,1.1811
""",
                "inches",
            ),
        )

        self.compare_multi(
            "G0 X10 Y20 Z30",
            "--no-header --no-comments --inches --precision=2 --no-show-editor",
            self.wrap(
                """J3,0.39,0.79,1.18
""",
                "inches",
            ),
        )

    def test015(self):
        """Test precision, and units, with G1, which generates MS commands"""
        f = FeedSpeed / 60.0  # mm/s

        # default is metric-mm (internal default)
        self.compare_multi(
            f"G1 F{f} X10 Y20 Z30",  # simple cut
            "--no-header --no-comments --no-show-editor --metric",
            self.wrap(
                """VS,7.0,9.4
M3,10.000,20.000,30.000
"""
            ),
        )

        self.compare_multi(
            f"G1 F{f} X10 Y20 Z30",
            "--no-header --no-comments --precision=2 --no-show-editor",
            self.wrap(
                """VS,7.0,9.4
M3,10.00,20.00,30.00
"""
            ),
        )

        self.compare_multi(
            f"G1 F{f} X10 Y20 Z30",
            "--no-header --no-comments --inches --no-show-editor",
            self.wrap(
                """VS,0.27,0.37
M3,0.3937,0.7874,1.1811
""",
                "inches",
            ),
        )
        self.compare_multi(
            f"G1 F{f} X10 Y20 Z30",
            "--no-header --no-comments --inches --precision=2 --no-show-editor",
            self.wrap(
                """VS,0.27,0.37
M3,0.39,0.79,1.18
""",
                "inches",
            ),
        )

    def test020(self):
        """Test single axis vs speed"""

        f = f"{FeedSpeed / 60.0:0.1f}"  # mm/s

        # one axis: X
        self.compare_multi(
            f"G1 F{f} X10",
            "--no-header --no-comments --metric --no-show-editor",
            self.wrap(
                f"""MX,10.000
"""
            ),
        )

        # one axis: y
        self.compare_multi(
            f"G1 F{f} Y10",
            "--no-header --no-comments --metric --no-show-editor",
            self.wrap(
                f"""M2,,10.000
"""
            ),
        )

        # one axis: Z
        self.compare_multi(
            f"G1 F{f} Z10",
            "--no-header --no-comments --metric --no-show-editor",
            self.wrap(
                f"""VS,,{f}
M3,,,10.000
"""
            ),
        )

        # this relies on the internal initial position being 0,0,0
        # and probably should be illegal without an initial G0
        self.compare_multi(
            f"G1 F{f} X10 Y0 Z0",
            "--no-header --no-comments --metric --no-show-editor",
            self.wrap(
                f"""M3,10.000,0.000,0.000
"""
            ),
        )

    def test024(self):
        """Test negative directions for speed"""

        f = f"{FeedSpeed / 60.0:0.1f}"  # mm/s

        # each axis
        self.compare_multi(
            "G0 X10 Y10 Z10",
            f"G1 F{f} X1",
            f"G1 F{f} Y1",
            f"G1 F{f} Z1",
            "--no-header --no-comments --metric --no-show-editor",
            self.wrap(
                f"""J3,10.000,10.000,10.000
MX,1.000
M2,,1.000
VS,,{f}
M3,,,1.000
"""
            ),
        )

        # xy
        self.compare_multi(
            "G0 X10 Y10 Z10",
            f"G1 F{f} X1 Y1",
            "--no-header --no-comments --metric --no-show-editor",
            self.wrap(
                f"""J3,10.000,10.000,10.000
M2,1.000,1.000
"""
            ),
        )

        # xyz
        self.compare_multi(
            "G0 X10 Y10 Z10",
            f"G1 F{f} X1 Y1 Z1",
            "--no-header --no-comments --metric --no-show-editor",
            self.wrap(
                """J3,10.000,10.000,10.000
VS,9.6,6.8
M3,1.000,1.000,1.000
"""
            ),
        )

        # XYZ <0
        self.compare_multi(
            "G0 X10 Y10 Z10",
            f"G1 F{f} X-2 Y-3 Z-4",
            "--no-header --no-comments --metric --no-show-editor",
            self.wrap(
                """J3,10.000,10.000,10.000
VS,9.2,7.3
M3,-2.000,-3.000,-4.000
"""
            ),
        )

    def test030(self):
        """Test Pre-amble"""

        # preamble values are verbatim, not unit converted!
        self.compare_multi(
            "(none)",
            "--no-header --no-comments --preamble='G0 Z50\nG1 F700 X20' --no-show-editor --metric",
            self.wrap(
                "",
                preamble="""J3,,,50.000
VS,700.0
MX,20.000
""",
            ),
        )

    def test040(self):
        """Test Post-amble"""
        # postamble is literal gcode, no unit translation
        self.compare_multi(
            "(none)",
            "--no-header --no-comments --postamble='G0 Z55\nG1 F700 X22' --no-show-editor",
            self.wrap(
                "",
                postamble="""J3,,,55.000
VS,700.0
MX,22.000
""",
            ),
        )

    def test050(self):
        """Test inches"""

        # inches
        self.compare_multi(
            "G0 X10 Y20 Z30",  # simple move
            "--no-header --no-comments --no-show-editor --inches",
            self.wrap(
                """J3,0.3937,0.7874,1.1811
""",
                "inches",
            ),
        )

        self.compare_multi(
            "G0 X10 Y20 Z30",  # simple move
            "--no-header --no-comments --no-show-editor --inches --precision 2",
            self.wrap(
                """J3,0.39,0.79,1.18
""",
                "inches",
            ),
        )

    def test060(self):
        """Test test modal
        Suppress the command name if the same as previous
        """
        f = f"{FeedSpeed / 60.0:0.3f}"  # mm/s
        c = f"G1 F{f} X10 Y20 Z30"

        self.compare_multi(
            c,
            c,
            "--no-header --no-comments --no-show-editor",
            # note no second MS, because no delta-position
            self.wrap(
                """VS,7.0,9.4
M3,10.000,20.000,30.000
M3,10.000,20.000,30.000
"""
            ),
        )
        self.compare_multi(
            c,
            c,
            "--no-header --no-comments --modal --no-show-editor",
            self.wrap(
                """VS,7.0,9.4
M3,10.000,20.000,30.000
"""
            ),
        )

        self.compare_multi(
            "G0 X49.845909 Y51.846232 Z54.000000",
            "G1 F38.100000 X49.845909 Y51.846232 Z51.000000",
            "G1 F38.100000 X49.487210 Y51.781909 Z50.828879",
            "G1 F38.100000 X49.147952 Y51.648842 Z50.657758",
            "--no-header --no-comments --modal --axis-modal --no-show-editor",
            self.wrap(
                """J3,49.846,51.846,54.000
VS,,38.1
M3,,,51.000
VS,34.5,16.2
M3,49.487,51.782,50.829
M3,49.148,51.649,50.658
"""
            ),
        )

    def test070(self):
        """Suppress the axis coordinate if the same as previous"""

        # w/o axis-modal
        c = "G0 X10 Y20 Z30"
        self.compare_multi(
            c,
            "G0 X10 Y30 Z30",
            "--no-header --no-comments --no-show-editor",
            self.wrap(
                """J3,10.000,20.000,30.000
J3,10.000,30.000,30.000
"""
            ),
        )

        # g91 relative, not impl yet
        # "G0 X10 Y30 Z30", "G91", "G0 X0 Y31 Z0",

        # diff y
        self.compare_multi(
            c,
            "G0 X10 Y21 Z30",
            "--no-header --no-comments --axis-modal --no-show-editor",
            self.wrap(
                """J3,10.000,20.000,30.000
J2,,21.000
"""
            ),
        )

        # diff z
        self.compare_multi(
            c,
            "G0 X10 Y20 Z31",
            "--no-header --no-comments --axis-modal --no-show-editor",
            self.wrap(
                """J3,10.000,20.000,30.000
J3,,,31.000
"""
            ),
        )

    def test080(self):
        """Test tool change, and spindle"""

        self.__class__.job = self.doc.getObject("Job001")
        self.__class__.post = PostProcessorFactory.get_post_processor(self.job, "opensbp")
        self.__class__.fixup_test_context()

        gcode_in = ["M6 T2", "M3 S3000", "M6 T3"]

        # tool change: manual
        with self.assertRaises(NotImplementedError) as context:
            self.compare_multi(
                *gcode_in,
                "--no-header --no-comments --comments --no-show-editor",
                """SA
&Tool=1
'Change tool to #1: T1, 1/8" two flute002
'First change tool, should already be #1: T1, 1/8" two flute002
&ToolName="T1 1/8 two flute002"
VS,11.7,5.8
JS,35.0,17.5
&Tool=2
'Change tool to #2: T3, Fly Cutter
PAUSE
&ToolName="T3 Fly Cutter"
VS,11.7,5.8
JS,35.0,17.5
TR,3000
C6
PAUSE 3
&Tool=3
'Change tool to #3: T2, 1/8" two flute003
PAUSE
&ToolName="T2 1/8 two flute003"
VS,11.7,5.8
JS,35.0,17.5
""",
            )
        self.assertTrue("2nd tool can't be done," in str(context.exception))

        expect = """SA
IF %(25) = 0 THEN GOTO WrongUnits
C7
&Tool=1
&ToolName="T1 1/8 two flute002"
C9
MS,11.7,5.8
JS,35.0,17.5
C7
&Tool=2
&ToolName="T3 Fly Cutter"
C9
MS,11.7,5.8
JS,35.0,17.5
TR,3000
C6
PAUSE {}
C7
&Tool=3
&ToolName="T2 1/8 two flute003"
C9
MS,11.7,5.8
JS,35.0,17.5
GOTO AfterWrongUnits
WrongUnits:
  if %(25) = 0 THEN &shopbot_which="inches"
  if %(25) = 1 THEN &shopbot_which="mm"
    MSGBOX("Post-processor wants G21/--metric but ShopBot is " & &shopbot_which & ". Change Units in ShopBot and try again.",0,"Change Units")
    ENDALL
AfterWrongUnits:
"""

        # both tool and spindle: auto
        self.compare_multi(
            *gcode_in,
            "--tool_change --no-comments --no-header --no-show-editor",
            expect.format(3),  # wait time
        )

        # auto-spindle with wait
        self.compare_multi(
            *gcode_in,
            "--tool_change --no-comments --wait-for-spindle 2 --no-header --no-show-editor",
            expect.format(2),  # wait time
        )

    def test090(self):
        """Test comment"""

        self.compare_multi("(comment)", "--no-header --no-comments --no-show-editor", self.wrap(""))

        self.compare_multi(
            "(comment)",
            "--no-header --comments --no-show-editor",
            self.wrap("'(comment)\n", comments=True),
        )

    def test100(self):
        """Test A, B axis output for values between 0 and 90 degrees"""
        self.compare_multi(
            f"G1 F{FeedSpeed} X10 Y20 Z30 A40 B50",
            "--no-header --no-comments --no-show-editor",
            self.wrap(
                """VS,211.1,283.2
M5,10.000,20.000,30.000,40.000,50.000
"""
            ),
        )

        self.compare_multi(
            f"G1 F{FeedSpeed} X10 Y20 Z30 A40 B50",
            "--no-header --no-comments --inches --no-show-editor",
            self.wrap(
                """VS,0.38,0.51
M5,0.3937,0.7874,1.1811,40.0000,50.0000
""",
                "inches",
            ),
        )

    @unittest.expectedFailure
    def test105(self):
        """Test A, B axis output for distance, not degrees"""

        # only noticeable for --inches

        self.compare_multi(
            "G1 X10 Y20 Z30 A40 B50",
            "--no-header --no-comments --inches --no-show-editor",
            """M5,0.3937,0.7874,1.1811,40.0000,50.0000
""",
        )

        self.compare_multi(
            "G1 X10 Y20 Z30 A40 B50",
            "--no-header --no-comments --no-show-editor --inches --ab-is-distance",
            """M5,0.3937,0.7874,1.1811,1.5748,1.9685
""",
        )

    def test110(self):
        """Test A, B, & C axis output for 89 degrees"""
        self.compare_multi(
            f"G1 F{FeedSpeed} X10 Y20 Z30 A89 B89",
            "--no-header --no-comments --no-show-editor",
            self.wrap(
                """VS,119.2,159.9
M5,10.000,20.000,30.000,89.000,89.000
"""
            ),
        )
        self.compare_multi(
            f"G1 F{FeedSpeed} X10 Y20 Z30 A89 B89",
            "--no-header --no-comments --inches --no-show-editor",
            self.wrap(
                """VS,0.19,0.26
M5,0.3937,0.7874,1.1811,89.0000,89.0000
""",
                "inches",
            ),
        )

    # FIXME: the use of getPathWithPlacement() causes a yaw-pitch calculation which gives odd AB values
    # so, tests disabled
    # no-other post-procesor tests AB (except linuxcnc which does not do getPathWithPlacement())

    @unittest.expectedFailure
    def test120(self):
        """Test A, B axis output for 90 degrees"""

        # BREAKS:
        # parses the gcode to {'A': 0.0, 'B': 90.0, 'X': 10.0, 'Y': 20.0, 'Z': 30.0}
        # Note the A==0

        self.compare_multi(
            "G1 X10 Y20 Z30 A90 B90",
            "--no-header --no-comments --no-show-editor",
            self.wrap("M5,10.000,20.000,30.000,90.000,90.000"),
        )
        self.compare_multi(
            "G1 X10 Y20 Z30 A90 B90",
            "--no-header --no-comments --inches --no-show-editor",
            self.wrap("M5,0.3937,0.7874,1.1811,90.0000,90.0000"),
        )

    @unittest.expectedFailure
    def test130(self):
        """Test A, B, & C axis output for 91 degrees"""

        self.compare_multi(
            "G1 X10 Y20 Z30 A91 B91",
            "--no-header --no-comments --no-show-editor",
            self.wrap("M5,10.000,20.000,30.000,91.000,91.000"),
        )
        self.compare_multi(
            "G1 X10 Y20 Z30 A91 B91",
            "--no-header --no-comments --inches --no-show-editor",
            self.wrap("M5,0.3937,0.7874,1.1811,91.0000,91.0000"),
        )

    @unittest.expectedFailure
    def test140(self):
        """Test A, B, & C axis output for values between 90 and 180 degrees"""
        self.compare_multi(
            "G1 X10 Y20 Z30 A100 B110",
            "--no-header --no-comments --no-show-editor",
            self.wrap("M5,10.000,20.000,30.000,100.000,110.000"),
        )
        self.compare_multi(
            "G1 X10 Y20 Z30 A100 B110",
            "--no-header --no-comments --inches --no-show-editor",
            self.wrap("M5,0.3937,0.7874,1.1811,100.0000,110.0000"),
        )

    @unittest.expectedFailure
    def test150(self):
        """Test A, B, & C axis output for values between 180 and 360 degrees"""
        self.compare_multi(
            "G1 X10 Y20 Z30 A240 B250",
            "--no-header --no-comments --no-show-editor",
            self.wrap("M5,10.000,20.000,30.000,240.000,250.000"),
        )
        self.compare_multi(
            "G1 X10 Y20 Z30 A240 B250",
            "--no-header --no-comments --inches --no-show-editor",
            self.wrap("M5,0.3937,0.7874,1.1811,240.0000,250.0000"),
        )

    @unittest.expectedFailure
    def test160(self):
        """Test A, B, & C axis output for values greater than 360 degrees"""
        self.compare_multi(
            "G1 X10 Y20 Z30 A440 B450",
            "--no-header --no-comments --no-show-editor",
            self.wrap("M5,10.000,20.000,30.000,440.000,450.000"),
        )
        self.compare_multi(
            "G1 X10 Y20 Z30 A440 B450",
            "--no-header --no-comments --inches --no-show-editor",
            self.wrap("M5,0.3937,0.7874,1.1811,440.0000,450.0000"),
        )

    def test170(self):
        """Test A, B, & C axis output for values between 0 and -90 degrees"""
        self.compare_multi(
            f"G1 F{FeedSpeed} X10 Y20 Z30 A-40 B-50",
            "--no-header --no-comments --no-show-editor",
            self.wrap(
                """VS,211.1,283.2
M5,10.000,20.000,30.000,-40.000,-50.000
"""
            ),
        )
        self.compare_multi(
            f"G1 F{FeedSpeed} X10 Y20 Z30 A-40 B-50",
            "--no-header --no-comments --inches --no-show-editor",
            self.wrap(
                """VS,0.38,0.51
M5,0.3937,0.7874,1.1811,-40.0000,-50.0000
""",
                "inches",
            ),
        )

    @unittest.expectedFailure
    def test180(self):
        """Test A, B, & C axis output for values between -90 and -180 degrees"""
        self.compare_multi(
            "G1 X10 Y20 Z30 A-100 B-110",
            "--no-header --no-comments --no-show-editor",
            self.wrap("M5,10.000,20.000,30.000,-100.000,-110.000"),
        )
        self.compare_multi(
            "G1 X10 Y20 Z30 A-100 B-110",
            "--no-header --no-comments --inches --no-show-editor",
            self.wrap("M5,0.3937,0.7874,1.1811,-100.0000,-110.0000"),
        )

    @unittest.expectedFailure
    def test190(self):
        """Test A, B, & C axis output for values between -180 and -360 degrees"""
        self.compare_multi(
            "G1 X10 Y20 Z30 A-240 B-250",
            "--no-header --no-comments --no-show-editor",
            self.wrap("M5,10.000,20.000,30.000,-240.000,-250.000"),
        )
        self.compare_multi(
            "G1 X10 Y20 Z30 A-240 B-250",
            "--no-header --no-comments --inches --no-show-editor",
            self.wrap("M5,0.3937,0.7874,1.1811,-240.0000,-250.0000"),
        )

    @unittest.expectedFailure
    def test200(self):
        """Test A, B, & C axis output for values below -360 degrees"""
        self.compare_multi(
            "G1 X10 Y20 Z30 A-440 B-450",
            "--no-header --no-comments --no-show-editor",
            self.wrap("M5,10.000,20.000,30.000,-440.000,-450.000"),
        )
        self.compare_multi(
            "G1 X10 Y20 Z30 A-440 B-450",
            "--no-header --no-comments --inches --no-show-editor",
            self.wrap("M5,0.3937,0.7874,1.1811,-440.0000,-450.0000"),
        )

    def test210(self):
        """Test return-to"""

        # return-to is before postamble
        self.compare_multi(
            "(none)",
            "--no-comments --postamble 'G0 X1 Y2 Z3' --return-to='12,34,56' --no-header --no-show-editor",
            self.wrap(
                """J3,12.000,34.000,56.000
J3,1.000,2.000,3.000
"""
            ),
        )

        if False:  # fails in UtilsArguments currently, because empty , isn't allowed
            # allow empty ,
            self.compare_multi(
                "(none)",
                "--no-comments --postamble 'G0 X1 Y2 Z3' --return-to=',34,56' --no-header --no-show-editor",
                self.wrap(
                    """J3,34.000,56.000
J3,1.000,2.000,3.000
"""
                ),
            )

    def test240(self):
        """Test relative & --modal
        We don't do relative, we'd have to track position
        """

        c = "G0 X10 Y20 Z30"

        with self.assertRaises(NotImplementedError) as context:
            self.compare_multi(
                "G91",
                c,
                c,
                "G90",
                c,
                c,
                "--no-header --no-comments --modal --no-show-editor",
                """SR 'RELATIVE
J3,10.000,20.000,30.000
J3,10.000,20.000,30.000
SA 'ABSOLUTE
J3,10.000,20.000,30.000
""",
            )
        self.assertTrue("gcode not handled" in str(context.exception))

    def test250(self):
        """Test G54"""
        # G54 is already in the Job we are using, but, why not:

        self.compare_multi("G54", "--no-header --no-comments --no-show-editor", self.wrap(""))

    def test260(self):
        """Test Arc"""

        # a few un-handled params
        for bad in ("K2", "P3", "R1"):
            with self.assertRaises(ValueError) as context:
                self.compare_multi(
                    f"G2 {bad} X10 Y20 Z40 I1 J2 F99", "--no-show-editor", "shoulld be no output"
                )
            self.assertIn(f"'{bad[0]}'", str(context.exception))

        self.compare_multi(
            "G0 Z5.00",  # the arc-CG plunge is relative, so start from non-0 to test
            f"G2 F{FeedSpeed} X10 Y20 Z40 I1 J2",  # helical segment
            "G0 Z5.01",
            f"G2 F{FeedSpeed-100} Z40.01 I1 J2",  # helical circle
            "G0 Z5.02",
            f"G2 F{FeedSpeed-200} X50 Y60.02 I1 J2",  # segment
            "--no-header --no-comments --no-show-editor",
            self.wrap(
                """J3,,,5.000
VS,137.7,686.3
CG,,10.000,20.000,1.000,2.000,T,1,35.000,,,,3,1,0 ' Z40.000
J3,,,5.010
VS,223.5,556.8
CG,,10.000,20.000,1.000,2.000,T,1,35.000,,,,3,1,0 ' Z40.010
J3,,,5.020
VS,500.0
CG,,50.000,60.020,1.000,2.000,T,1,0.000,,,,0,1,0 ' Z5.020
"""
            ),
        )

    def test270(self):
        """Test M00 w/prompt (pause): dialog box"""

        self.compare_multi(
            "(With Prompt)",
            "M0",
            "--no-header --comments --no-show-editor",
            self.wrap(
                """'(With Prompt)
PAUSE
""",
                comments=True,
            ),
        )

        self.compare_multi(
            # Include the preceding comment even if no-comments
            "(With Prompt)",
            "M0",
            "--no-header --no-comments --no-show-editor",
            self.wrap(
                """'(With Prompt)
PAUSE
"""
            ),
        )

        self.compare_multi(
            "(Doesn't count as prompt)",
            "G0 X0",
            "M0",  # default prompt
            "--no-header --comments --no-show-editor",
            self.wrap(
                """'(Doesn't count as prompt)
JX,0.000
'Continue <Job>.<Profile>?
PAUSE
""",
                comments=True,
            ),
        )

    def test280(self):
        """Test drilling"""

        # We'll trust the other variations (gcode generation tested in TestRefactoredTestPostGCodes.py)
        self.compare_multi(
            "G00 X0 Y0 Z5",
            "G73 X1 Y2 Z0 R5 Q1.5 F123",
            "--no-header --comments --no-show-editor",
            self.wrap(
                """J3,0.000,0.000,5.000
'(G73 X1.00000 Y2.00000 Z0.00000 R5.00000 Q1.50000 F123.00000)
J2,1.000,2.000
VS,,123.0
M3,,,3.500
J3,,,3.750
J3,,,3.575
M3,,,2.000
J3,,,2.250
J3,,,2.075
M3,,,0.500
J3,,,0.750
J3,,,0.575
M3,,,0.000
J3,,,5.000
""",
                comments=True,
            ),
        )

    def test290(self):
        """MC_RUN_COMMAND: native pass-through"""

        self.compare_multi(
            # "G00 X10 Y10 Z5",
            '(MC_RUN_COMMAND PRINT "Hello")',
            "--no-header --comments --no-show-editor",
            self.wrap(
                """'(MC_RUN_COMMAND PRINT "Hello")
PRINT "Hello"
""",
                comments=True,
            ),
        )

    def test310(self):
        """speed-modal"""
        self.compare_multi(
            "G0 X10 Y10 Z10",
            "G1 F100 X100",
            "G1 F100 X200",
            "G1 F100 Z100",
            "G1 F100 Z200",
            "--no-header --no-comments --no-show-editor",
            self.wrap(
                """J3,10.000,10.000,10.000
VS,100.0
MX,100.000
MX,200.000
VS,,100.0
M3,,,100.000
M3,,,200.000
"""
            ),
        )

    def test320(self):
        """--axis-modal"""
        self.compare_multi(
            # all G0's, so no MS
            "G0 X10 Y10 Z10",
            "G0 F100 X100 Y10 Z10",  # dX
            "G0 F100 X101 Z10",  # dx
            "G0 F100 X101 Y102 Z10",  # dY
            "G0 F100 X101 Y102 Z103",  # dz
            "G0 F100 X104 Y105 Z103",  # dxy
            "G0 F100 X106 Y105 Z107",  # dxz
            "--axis-modal --no-header --no-comments --no-show-editor",
            self.wrap(
                """J3,10.000,10.000,10.000
JX,100.000
JX,101.000
J2,,102.000
J3,,,103.000
J2,104.000,105.000
J3,106.000,,107.000
"""
            ),
        )

    def test330(self):
        """Optimization o1 o2 o3"""

        self.compare_multi(
            "G0 X10 Y10 Z10",
            "G0 X10 Y10 Z10",  # should see the repeated command
            "--o1 --no-show-editor",
            self.wrap(
                """J3,10.000,10.000,10.000
J3,10.000,10.000,10.000
"""
            ),
        )

        self.compare_multi(
            "G0 X10 Y10 Z10",
            "G0 X10 Y10 Z10",  # should not see the repeated command
            "--o2 --no-show-editor",
            self.wrap(
                """J3,10.000,10.000,10.000
""",
                comments=True,
                header=True,
            ),
            remove=r"'\(Output Time:",
        )

        self.compare_multi(
            "G0 X10 Y10 Z10",
            "G0 X10 Y10 Z10",  # should not see the repeated command
            "--o3 --no-show-editor",
            self.wrap(
                """J3,10.000,10.000,10.000
"""
            ),
        )

    def test350(self):
        """--allow-unknown"""

        with self.assertRaises(NotImplementedError) as context:
            self.compare_multi("G111", "--no-show-editor", self.wrap("throws"))
        self.assertTrue("gcode not handled" in str(context.exception))

        self.compare_multi(
            "G111",
            "--allow-unknown G111,G777 --no-show-editor --no-comments --no-header",
            self.wrap(""),
        )

    def test360(self):
        """Probe operation"""

        self.compare_multi(
            *"""(Probe009)
(Begin Probing)
(PROBEOPEN probe)
G0 Z56.00000
G0 X-1.000000 Y-1.000000 Z54.000000
G38.2 F0.000000 Z50.000000
G0 Z54.000000
G0 X50.000000 Y-1.000000 Z54.000000
G38.2 F0.000000 Z50.000000
G0 Z54.000000
(PROBECLOSE)
G0 Z56.000000""".split(
                "\n"
            ),
            "--no-show-editor --comments --no-header",
            self.wrap(
                """'(Probe009)
'(Begin Probing)
'(PROBEOPEN probe)
'Load the My_Variables file from Custom Cut 90 in C:\\SbParts\\Custom
C#,90
OPEN "probe.txt" FOR OUTPUT as #1
&hit = 0
J3,,,56.000
J3,-1.000,-1.000,54.000
&hit = 0
ON INPUT(&my_ZzeroInput, 1) GOSUB CaptureZPos
M3,,,50.000
IF &hit = 0 THEN GOTO FailedToTouch
J3,,,54.000
J3,50.000,-1.000,54.000
&hit = 0
ON INPUT(&my_ZzeroInput, 1) GOSUB CaptureZPos
M3,,,50.000
IF &hit = 0 THEN GOTO FailedToTouch
J3,,,54.000
'(PROBECLOSE)
'Clear probe-switch-trigger
ON INPUT(&my_ZzeroInput, 1)
CLOSE #1
J3,,,56.000
""",
                postfix="""GOTO SkipProbeSubRoutines
CaptureZPos:
  ' for g38.2 probe, write the data on probe-contact
  ' and set flag for didn't-fail
  ' xyzab
  WRITE #1; %(1); " "; %(2); " "; %(3); " "; %(4); " "; %(5)
  &hit = 1
  RETURN
FailedToTouch:
  ' for g38.2 probe, when
  ' failed to trigger w/in movement
  MSGBOX(Failed to touch...Exiting,16,Probe Failed)
  END
SkipProbeSubRoutines:
""",
                comments=True,
                precision=3,
            ),
            debug=True,
        )
