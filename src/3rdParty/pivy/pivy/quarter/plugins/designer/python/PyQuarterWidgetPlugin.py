###
# Copyright (c) 2002-2008 Kongsberg SIM
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

"""
PyQuarterWidgetPlugin - Qt Python Designer Plugin

To use the PyQuarterWidgetPlugin append
$install_prefix/site-packages/pivy/quarter/plugins to the
QT_PLUGIN_PATH environment variable.
"""

from pivy.qt import QtGui, QtCore, QtDesigner
from pivy import coin, quarter

class PyQuarterWidgetPlugin(QtDesigner.QPyDesignerCustomWidgetPlugin):

    def __init__(self, parent=None):
        QtDesigner.QPyDesignerCustomWidgetPlugin.__init__(self)

        self._initialized = False
        self._firstwidget = None

    def initialize(self, formEditor):
        if self._initialized:
            return

        self._initialized = True

    def isInitialized(self):
        return self._initialized

    def createWidget(self, parent):
        widget = quarter.QuarterWidget(parent, self._firstwidget)
        
        if not self._firstwidget:
            self._firstwidget = widget
            self.connect(widget, QtCore.SIGNAL("destroyed(QObject*)"),
                         self, QtCore.SLOT("widgetDestroyed(QObject*)"))        
        
        widget.setSceneGraph(coin.SoCube())
        return widget

    def name(self):
        return "PyQuarterWidget"

    def group(self):
        return "Display Widgets [Coin3D]"

    def icon(self):
        return QtGui.QIcon(_coin_logo_pixmap)

    def toolTip(self):
        return "Quarter Python widget for Coin"

    def whatsThis(self):
        return "The QuarterWidget displays Open Inventor scene graphs."

    def isContainer(self):
        return False

    def domXml(self):
        return '<widget class="QuarterWidget" name="quarterWidget">\n' \
               ' <property name=\"geometry\">\n' \
               '  <rect>\n' \
               '   <x>0</x>\n' \
               '   <y>0</y>\n' \
               '   <width>100</width>\n'\
               '   <height>100</height>\n' \
               '  </rect>\n' \
               ' </property>\n' \
               ' <property name=\"toolTip\" >\n' \
               '  <string>Quarter Python widget for Coin</string>\n' \
               ' </property>\n' \
               ' <property name=\"whatsThis\" >\n' \
               '  <string>The QuarterWidget displays Open Inventor scene graphs.</string>\n' \
               ' </property>\n' \
               '</widget>\n'

    def includeFile(self):
        return "pivy.quarter.QuarterWidget"

_coin_logo_24x24_xpm = [
    "24 24 337 2",
    "  	c None",
    ". 	c #000000",
    "+ 	c #232A2C",
    "@ 	c #101314",
    "# 	c #171E21",
    "$ 	c #526973",
    "% 	c #809EAC",
    "& 	c #A7CAD9",
    "* 	c #B0D1DE",
    "= 	c #B8D7E3",
    "- 	c #C0DCE7",
    "; 	c #A1B7BF",
    "> 	c #738186",
    ", 	c #232728",
    "' 	c #0B0F11",
    ") 	c #4E6A79",
    "! 	c #89B3C8",
    "~ 	c #95BCCF",
    "{ 	c #A0C4D5",
    "] 	c #AACCDB",
    "^ 	c #B3D3E0",
    "/ 	c #BCD9E5",
    "( 	c #C3DFE9",
    "_ 	c #CAE4ED",
    ": 	c #CFE8F0",
    "< 	c #D3EBF2",
    "[ 	c #869599",
    "} 	c #141616",
    "| 	c #162127",
    "1 	c #6A97AF",
    "2 	c #7EABC1",
    "3 	c #8BB5C9",
    "4 	c #97BED0",
    "5 	c #A2C6D6",
    "6 	c #ACCEDC",
    "7 	c #B6D5E1",
    "8 	c #BEDBE6",
    "9 	c #C6E1EA",
    "0 	c #CCE6EE",
    "a 	c #D2EAF1",
    "b 	c #D6EDF4",
    "c 	c #DAF0F5",
    "d 	c #CEE2E7",
    "e 	c #2F3435",
    "f 	c #131F25",
    "g 	c #5D8DA7",
    "h 	c #71A2BA",
    "i 	c #7FACC2",
    "j 	c #8CB5CA",
    "k 	c #98BFD1",
    "l 	c #A3C7D7",
    "m 	c #90A9B4",
    "n 	c #9DA9AE",
    "o 	c #AEB2B3",
    "p 	c #B6B6B6",
    "q 	c #B4B6B6",
    "r 	c #ACB3B5",
    "s 	c #A5B3B7",
    "t 	c #DBF1F6",
    "u 	c #DDF2F7",
    "v 	c #D0E4E8",
    "w 	c #070C0F",
    "x 	c #4E839F",
    "y 	c #6397B2",
    "z 	c #85A7B6",
    "A 	c #9BA4A8",
    "B 	c #DDDDDD",
    "C 	c #F2F2F2",
    "D 	c #F7F7F7",
    "E 	c #F9F9F9",
    "F 	c #F6F6F6",
    "G 	c #C0C0C0",
    "H 	c #5C6364",
    "I 	c #DDF2F8",
    "J 	c #DEF3F8",
    "K 	c #CEE3E7",
    "L 	c #2B5065",
    "M 	c #528AA9",
    "N 	c #6296B2",
    "O 	c #70A1BA",
    "P 	c #7EABC2",
    "Q 	c #7E99A5",
    "R 	c #BEC1C2",
    "S 	c #E8E8E8",
    "T 	c #F0F0F0",
    "U 	c #FBFBFB",
    "V 	c #FDFDFD",
    "W 	c #C7C7C7",
    "X 	c #8F8F8F",
    "Y 	c #A3A3A3",
    "Z 	c #8F9A9D",
    "` 	c #DCF1F7",
    " .	c #DBF0F6",
    "..	c #879599",
    "+.	c #091319",
    "@.	c #407C9E",
    "#.	c #5089A8",
    "$.	c #6094B0",
    "%.	c #6E9FB9",
    "&.	c #6E8D9D",
    "*.	c #C8C8C8",
    "=.	c #E0E0E0",
    "-.	c #EAEAEA",
    ";.	c #F8F8F8",
    ">.	c #C5C5C5",
    ",.	c #828282",
    "'.	c #888888",
    ").	c #767676",
    "!.	c #798285",
    "~.	c #C9DFE5",
    "{.	c #D9EFF5",
    "].	c #D8EEF5",
    "^.	c #D5ECF3",
    "/.	c #1B3F54",
    "(.	c #3D7A9D",
    "_.	c #4D87A6",
    ":.	c #5D92AF",
    "<.	c #6B9DB7",
    "[.	c #8D9396",
    "}.	c #D5D5D5",
    "|.	c #E9E9E9",
    "1.	c #F1F1F1",
    "2.	c #858585",
    "3.	c #696969",
    "4.	c #5E5E5E",
    "5.	c #6D777A",
    "6.	c #B2C9D0",
    "7.	c #D0E9F0",
    "8.	c #D4ECF3",
    "9.	c #D4EBF2",
    "0.	c #D1E9F1",
    "a.	c #748286",
    "b.	c #225876",
    "c.	c #39779B",
    "d.	c #4984A4",
    "e.	c #598FAD",
    "f.	c #527587",
    "g.	c #C6C6C6",
    "h.	c #D2D2D2",
    "i.	c #E6E6E6",
    "j.	c #DEDEDE",
    "k.	c #3A3A3A",
    "l.	c #555C60",
    "m.	c #98B0BA",
    "n.	c #C2DEE8",
    "o.	c #C7E2EB",
    "p.	c #CBE5ED",
    "q.	c #CEE7EF",
    "r.	c #A3B9C0",
    "s.	c #24678E",
    "t.	c #347498",
    "u.	c #4580A1",
    "v.	c #548CAA",
    "w.	c #637A87",
    "x.	c #C2C2C2",
    "y.	c #CECECE",
    "z.	c #D8D8D8",
    "A.	c #828B8E",
    "B.	c #AECFDD",
    "C.	c #C1DDE8",
    "D.	c #C4E0EA",
    "E.	c #C9E3EC",
    "F.	c #C8E3EC",
    "G.	c #C2DEE9",
    "H.	c #02080C",
    "I.	c #1E638B",
    "J.	c #2F7095",
    "K.	c #3E7B9E",
    "L.	c #6A7479",
    "M.	c #BCBCBC",
    "N.	c #D0D0D0",
    "O.	c #93B1BE",
    "P.	c #AFD0DD",
    "Q.	c #B5D4E1",
    "R.	c #BAD8E3",
    "S.	c #BDDBE6",
    "T.	c #C0DDE7",
    "U.	c #C1DEE8",
    "V.	c #90A2A8",
    "W.	c #8EA0A7",
    "X.	c #BBD9E4",
    "Y.	c #121517",
    "Z.	c #03111A",
    "`.	c #15587F",
    " +	c #28526A",
    ".+	c #4E616C",
    "++	c #727272",
    "@+	c #A7A7A7",
    "#+	c #B3B3B3",
    "$+	c #BDBDBD",
    "%+	c #CDCDCD",
    "&+	c #D3D3D3",
    "*+	c #D6D6D6",
    "=+	c #8E9BA1",
    "-+	c #ADCEDC",
    ";+	c #B2D2DF",
    ">+	c #B5D5E1",
    ",+	c #8B9EA5",
    "'+	c #888C8E",
    ")+	c #ACACAC",
    "!+	c #7D8E94",
    "~+	c #121516",
    "{+	c #4E4E4E",
    "]+	c #717171",
    "^+	c #818181",
    "/+	c #9C9C9C",
    "(+	c #A8A8A8",
    "_+	c #B2B2B2",
    ":+	c #BABABA",
    "<+	c #C1C1C1",
    "[+	c #C9C9C9",
    "}+	c #B0B0B0",
    "|+	c #819FAD",
    "1+	c #A4C7D7",
    "2+	c #8297A0",
    "3+	c #898D8F",
    "4+	c #A9A9A9",
    "5+	c #8B8B8B",
    "6+	c #8EA9B4",
    "7+	c #474747",
    "8+	c #666666",
    "9+	c #757575",
    "0+	c #848484",
    "a+	c #909090",
    "b+	c #9B9B9B",
    "c+	c #A5A5A5",
    "d+	c #ADADAD",
    "e+	c #B7B7B7",
    "f+	c #A1A1A1",
    "g+	c #7C7C7C",
    "h+	c #ABABAB",
    "i+	c #999999",
    "j+	c #535556",
    "k+	c #A5C9D8",
    "l+	c #82A0AD",
    "m+	c #2B2B2B",
    "n+	c #595959",
    "o+	c #686868",
    "p+	c #8C8C8C",
    "q+	c #969696",
    "r+	c #9D9D9D",
    "s+	c #A0A0A0",
    "t+	c #919191",
    "u+	c #787878",
    "v+	c #4D5D65",
    "w+	c #9AC0D2",
    "x+	c #556A75",
    "y+	c #0C0C0C",
    "z+	c #4B4B4B",
    "A+	c #5A5A5A",
    "B+	c #676767",
    "C+	c #949494",
    "D+	c #959595",
    "E+	c #8D8D8D",
    "F+	c #868686",
    "G+	c #5B5B5B",
    "H+	c #484A4C",
    "I+	c #7DA1B2",
    "J+	c #8EB7CB",
    "K+	c #8CB6CA",
    "L+	c #181F22",
    "M+	c #272727",
    "N+	c #494949",
    "O+	c #565656",
    "P+	c #616161",
    "Q+	c #6A6A6A",
    "R+	c #808080",
    "S+	c #7D7D7D",
    "T+	c #414141",
    "U+	c #4F6774",
    "V+	c #83AFC4",
    "W+	c #82AEC4",
    "X+	c #516C7A",
    "Y+	c #050505",
    "Z+	c #343434",
    "`+	c #434343",
    " @	c #444444",
    ".@	c #7E7E7E",
    "+@	c #585858",
    "@@	c #454545",
    "#@	c #484848",
    "$@	c #323232",
    "%@	c #394F5A",
    "&@	c #74A4BC",
    "*@	c #75A4BC",
    "=@	c #6D9AB0",
    "-@	c #0A0A0A",
    ";@	c #242424",
    ">@	c #303030",
    ",@	c #505050",
    "'@	c #213B49",
    ")@	c #27485B",
    "!@	c #6A6E70",
    "~@	c #7A7A7A",
    "{@	c #334047",
    "]@	c #497085",
    "^@	c #6598B4",
    "/@	c #6699B4",
    "(@	c #6090A9",
    "_@	c #172227",
    ":@	c #090909",
    "<@	c #2A2A2A",
    "[@	c #112D3D",
    "}@	c #145B85",
    "|@	c #256488",
    "1@	c #285168",
    "2@	c #49697B",
    "3@	c #476577",
    "4@	c #37596C",
    "5@	c #427591",
    "6@	c #538BAA",
    "7@	c #568DAB",
    "8@	c #5285A1",
    "9@	c #141F25",
    "0@	c #03324E",
    "a@	c #07527E",
    "b@	c #0F5883",
    "c@	c #195F88",
    "d@	c #22668D",
    "e@	c #2A6C92",
    "f@	c #317196",
    "g@	c #37769A",
    "h@	c #3C7A9D",
    "i@	c #407D9F",
    "j@	c #447FA1",
    "k@	c #2D5266",
    "l@	c #010D15",
    "m@	c #032D47",
    "n@	c #084568",
    "o@	c #125A84",
    "p@	c #196089",
    "q@	c #20658D",
    "r@	c #266A90",
    "s@	c #255A78",
    "t@	c #1D4155",
    "u@	c #0A141A",
    "v@	c #02090D",
    "                    . + @ .                     ",
    "              # $ % & * = - ; > ,               ",
    "          ' ) ! ~ { ] ^ / ( _ : < [ }           ",
    "        | 1 2 3 4 5 6 7 8 9 0 a b c d e         ",
    "      f g h i j k l m n o p q r s t u v e       ",
    "    w x y h i j z A B C D E E F G H I J K }     ",
    "    L M N O P Q R S T D U V W X Y Z ` `  ...    ",
    "  +.@.#.$.%.&.*.=.-.C ;.>.,.'.).!.~.{.{.].^.,   ",
    "  /.(._.:.<.[.}.=.|.1.2.3.4.5.6.7.< 8.^.9.0.a.  ",
    "  b.c.d.e.f.g.h.B i.j.k.l.m.n.o.p.q.: : q.0 r.  ",
    ". s.t.u.v.w.x.y.z.=.i.A.B.7 / C.D.o.E.E.F.9 G.. ",
    "H.I.J.K._.L.M.g.N.z.j.>.O.P.Q.R.S.T.U.V.W.8 X.Y.",
    "Z.`. +.+++@+#+$+g.%+&+*+=+& -+;+>+,+'+)+!+7 ^ ~+",
    ". {+]+^+X /+(+_+:+<+g.[+}+|+1+2+3+#+4+5+6+-+] . ",
    "  7+8+9+0+a+b+c+d+#+e+:+:+f+g+_+h+Y i+j+k+l l+  ",
    "  m+n+o+).,.p+q+r+Y @+4+4+(+c+s+i+t+u+v+w+k x+  ",
    "  y+z+A+B+++g+2.p+t+C+q+q+D+t+E+F+G+H+I+J+K+L+  ",
    "    M+N+O+P+Q+n+4.3.^+,.,.R+S+n+T+N+U+V+W+X+    ",
    "    Y+Z+`+ @ @).a+i+.@9+P++@@@#@$@%@&@*@=@'     ",
    "      -@;@>@,@'@)@!@_+e+b+~@{+{@]@^@/@(@_@      ",
    "        :@<@[@}@I.|@1@2@3@4@5@#.6@7@8@9@        ",
    "          . 0@a@b@c@d@e@f@g@h@i@j@k@w           ",
    "              l@m@n@o@p@q@r@s@t@u@              ",
    "                    . v@v@.                     " ]

_coin_logo_pixmap = QtGui.QPixmap(_coin_logo_24x24_xpm)
