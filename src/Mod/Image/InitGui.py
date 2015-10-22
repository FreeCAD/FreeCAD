# Image gui init module
# (c) 2003 Juergen Riegel
#
# Gathering all the information to start FreeCAD
# This is the second one of three init scripts, the third one
# runs when the gui is up

#***************************************************************************
#*   (c) Juergen Riegel (juergen.riegel@web.de) 2002                       *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Lesser General Public License for more details.                   *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#*   Juergen Riegel 2002                                                   *
#***************************************************************************/



class ImageWorkbench ( Workbench ):
	"Image workbench object"
	Icon = """
			/* XPM */
			static const char *colors[]={
			"16 16 134 2",
			"Qt c None",
			".# c #000000",
			"#y c #000000",
			".L c #000000",
			".e c #000000",
			"#X c #000000",
			"#M c #000000",
			"#z c #000000",
			"#L c #120000",
			".Y c #000000",
			".d c #0c0c00",
			".K c #140a00",
			"ad c #120900",
			".a c #1b0909",
			"ab c #0f0800",
			".A c #231106",
			"ac c #1b0b00",
			".c c #291a0f",
			".b c #2e2012",
			".X c #311f09",
			"#l c #362104",
			"#Y c #241000",
			"## c #2b1900",
			".l c #402300",
			".J c #3f2611",
			".M c #372000",
			"#7 c #3b1f00",
			"#. c #503511",
			".s c #51361e",
			"#6 c #542d00",
			"#N c #462700",
			".f c #603805",
			"#m c #4d2b00",
			"aa c #693900",
			"#A c #583000",
			".k c #71553d",
			".B c #6b4100",
			".t c #754800",
			".G c #764d27",
			"#x c #94651a",
			".W c #946b35",
			"#8 c #8e5100",
			".I c #a07749",
			"#W c #a5660d",
			"#Z c #945200",
			".R c #9f6932",
			".Z c #9d5d00",
			"#K c #ba801a",
			".j c #c4aa92",
			".V c #cea05c",
			"a# c #c67700",
			".F c #c69869",
			".g c #cda881",
			"#9 c #c57700",
			"#O c #b87700",
			".9 c #d4a558",
			".z c #dab68c",
			"#k c #c99c51",
			"#a c #d47c00",
			"a. c #d58300",
			".i c #e6d2bf",
			".m c #e88400",
			"#B c #d88300",
			".h c #ead8c7",
			"#n c #e88600",
			".S c #e8b877",
			".N c #e98600",
			"#0 c #e98d00",
			".U c #efc27c",
			".Q c #f0bd7e",
			".H c #ecc28c",
			"#5 c #f3a204",
			".r c #f2d9bb",
			".x c #f3d3b0",
			".y c #f4d6b1",
			".3 c #fcc478",
			".4 c #fdc878",
			"#D c #0ccd06",
			"#p c #13950d",
			"#v c #4547cc",
			"#u c #5e589f",
			"#C c #62a200",
			"#q c #69af39",
			"#i c #6e6ba1",
			"#I c #7a668f",
			"#P c #80ac00",
			"#j c #9084a1",
			"#E c #91831e",
			"#d c #95a24e",
			"#Q c #a08500",
			"#h c #a28d84",
			"#F c #a62f2f",
			"#t c #a8745f",
			"#w c #ae9381",
			"#J c #af8e6c",
			"#R c #b21e07",
			"#H c #b56e49",
			"#G c #b93932",
			"#o c #bd8900",
			"#s c #c77d4b",
			"#S c #cc0d12",
			"#T c #da2d17",
			"#2 c #da5100",
			"#3 c #db3b01",
			"#c c #de8f0e",
			"#e c #e0bb65",
			"#1 c #ea8300",
			"#r c #ebb04b",
			"#U c #eda11a",
			".8 c #edc882",
			"#4 c #ee9603",
			".7 c #f3cc83",
			"#g c #fcc664",
			".C c #fe8c00",
			".O c #ff8500",
			".D c #ff890a",
			".u c #ff8a00",
			".1 c #ff8c05",
			".0 c #ff8d00",
			"#b c #ff8e00",
			".v c #ff9f38",
			".P c #ffa336",
			".2 c #ffb352",
			"#V c #ffbf1c",
			"#f c #ffc563",
			".n c #ffc683",
			".E c #ffc885",
			".5 c #ffcd7c",
			".6 c #ffd17d",
			".T c #ffd894",
			".w c #ffe1bf",
			".p c #fff3e2",
			".o c #fff3e4",
			".q c #fff6e4",
			"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
			"QtQt.#.a.b.c.d.#QtQtQtQtQtQtQtQt",
			"Qt.e.f.g.h.i.j.k.dQtQtQtQtQtQtQt",
			"Qt.l.m.n.o.p.q.r.sQtQtQtQtQtQtQt",
			"Qt.t.u.v.w.x.y.z.AQtQtQtQtQtQtQt",
			"Qt.B.C.D.E.F.G.H.I.J.K.LQtQtQtQt",
			"Qt.M.N.O.P.Q.R.S.T.U.V.W.X.#QtQt",
			"Qt.Y.Z.0.1.2.3.4.5.6.7.8.9#..#Qt",
			"QtQt###a#b#c#d#e#f#g#h#i#j#k#lQt",
			"QtQt.L#m#n#o#p#q#r#s#t#u#v#w#x#y",
			"QtQtQt#z#A#B#C#D#E#F#G#H#I#J#K#L",
			"QtQtQtQt#M#N#O#P#Q#R#S#T#U#V#W#X",
			"QtQtQtQtQt#y#Y#Z#0#1#2#3#4#5#6Qt",
			"QtQtQtQtQtQtQt.Y#7#8#9a.a#aa#zQt",
			"QtQtQtQtQtQtQtQtQt#zabacad#yQtQt",
			"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt"};
			"""
	MenuText = "Image"
	ToolTip = "Image workbench"

	def Initialize(self):
		# load the module
		import ImageGui
	def GetClassName(self):
		return "ImageGui::Workbench"

Gui.addWorkbench(ImageWorkbench())

# Append the open handler
FreeCAD.EndingAdd("Image formats (*.bmp *.jpg *.png *.xpm)","ImageGui")
