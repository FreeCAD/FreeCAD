# MachiningDistortion Gui stuff


class MachiningDistortionWorkbench ( Workbench ):
    "Test workbench object"
    Icon = """
        /* XPM */
        static const char *test_icon[]={
"13 16 88 1",
" 	c None",
".	c #000000",
"+	c #130D02",
"@	c #372704",
"#	c #5C4107",
"$	c #5C3F07",
"%	c #5B3D07",
"&	c #5B3C07",
"*	c #5B3A07",
"=	c #4D3106",
"-	c #5B4207",
";	c #9A7E08",
">	c #E4B30C",
",	c #E7B00C",
"'	c #E3A70B",
")	c #E09E0B",
"!	c #DD950A",
"~	c #5D3B07",
"{	c #5D4707",
"]	c #F1CC0E",
"^	c #AF900A",
"/	c #BF970B",
"(	c #E7B10C",
"_	c #E4A90C",
":	c #E1A00B",
"<	c #5D3E07",
"[	c #5F4807",
"}	c #F5D70E",
"|	c #D7B70C",
"1	c #504204",
"2	c #EBBC0D",
"3	c #E8B30C",
"4	c #E5AB0C",
"5	c #5E4007",
"6	c #604A08",
"7	c #F9E10F",
"8	c #F6D90E",
"9	c #574A05",
"0	c #DBB70C",
"a	c #ECBE0D",
"b	c #E9B50C",
"c	c #604307",
"d	c #604B08",
"e	c #FDEC10",
"f	c #FAE30F",
"g	c #F4D70F",
"h	c #F3D20E",
"i	c #F0C90E",
"j	c #EDC00D",
"k	c #604507",
"l	c #FFF110",
"m	c #FEEE10",
"n	c #FBE50F",
"o	c #726607",
"p	c #EBCD0E",
"q	c #F1CB0E",
"r	c #614608",
"s	c #FFF010",
"t	c #A3960A",
"u	c #958509",
"v	c #F5D60E",
"w	c #614908",
"x	c #F9EB10",
"y	c #716807",
"z	c #F9E00F",
"A	c #634B08",
"B	c #FDEB10",
"C	c #634D08",
"D	c #5C4808",
"E	c #B9AF0C",
"F	c #A69D0A",
"G	c #655008",
"H	c #584407",
"I	c #EFE20F",
"J	c #595406",
"K	c #5B4708",
"L	c #716B07",
"M	c #2A2104",
"N	c #1B1502",
"O	c #E9DC0F",
"P	c #2C2303",
"Q	c #201803",
"R	c #6B5608",
"S	c #1E1802",
"T	c #201A02",
"U	c #695508",
"V	c #362C04",
"W	c #0C0901",
"...  +@#$%&*=",
"     -;>,')!~",
" .   {]^/(_:<",
" .   [}|12345",
"     67890abc",
"  .  defghijk",
"   . dlmnopqr",
"     dllstuvw",
"     dlllxyzA",
"    .dlllllBC",
"    .DllllEFG",
"     HllllIJG",
"     KlllllLG",
"     MllllllG",
"     NlllllOP",
"     QRPSTUVW"};
        """
    MenuText = "Machining Distortion"
    ToolTip = "MachiningDistortion workbench"
    
    def Initialize(self):
        import MachiningDistortionCommands
        import MachDistMaterial
        import machdist_rc
        CmdList = ["MachiningDistortion_StartGUI","MachiningDistortion_StartPostprocess"]
        self.appendToolbar("MachiningDistortionTools",CmdList)
        self.appendMenu("Machining Distortion",CmdList)
        self.appendToolbar("MachiningDistortionTools2",["MachDist_Material"])
        self.appendMenu("Machining Distortion2",["MachDist_Material"])
        
        Gui.addPreferencePage(":/ui/userprefs-base.ui","Machining Distortion")


        Log ('Loading MachiningDistortion module... done\n')
    def Activated(self):
        Msg("MachiningDistortionWorkbench::Activated()\n")
    def Deactivated(self):
        Msg("MachiningDistortionWorkbench::Deactivated()\n")

Gui.addWorkbench(MachiningDistortionWorkbench)
