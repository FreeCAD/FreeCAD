# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""This is the tutorial of the BIM workbench"""

import os

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")


html = """<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0//EN" "http://www.w3.org/TR/REC-html40/strict.dtd">
<html><head><meta name="qrichtext" content="1" /><style type="text/css">
p, li { white-space: pre-wrap; }</style></head><body>inserthere</body></html>"""

URL = "https://www.freecadweb.org/wiki/BIM_ingame_tutorial"
TESTINTERVAL = 1000  # interval between tests


class BIM_Tutorial:
    def GetResources(self):
        return {
            "Pixmap": "BIM_Tutorial",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Tutorial", "BIM Tutorial"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Tutorial", "Starts or continues the BIM in-game tutorial"
            ),
        }

    def Activated(self):
        from PySide import QtCore, QtGui

        # find existing tutorial
        m = FreeCADGui.getMainWindow()
        self.dock = m.findChild(QtGui.QDockWidget, "BIMTutorial")

        if not self.dock:
            # set the tutorial dialog up
            self.form = FreeCADGui.PySideUic.loadUi(":/ui/dialogTutorial.ui")
            self.form.setObjectName("BIMTutorial")
            self.form.progressBar.setValue(0)
            self.form.labelGoal1.setText("")
            self.form.labelGoal2.setText("")
            self.form.labelIcon1.setText("")
            self.form.labelIcon2.setText("")
            self.form.buttonPrevious.setEnabled(False)
            self.form.buttonNext.setEnabled(False)
            self.form.buttonPrevious.clicked.connect(self.previous)
            self.form.buttonNext.clicked.connect(self.next)
            self.form.labelTasks.hide()
            self.form.textEdit.setOpenExternalLinks(True)

            self.dock = QtGui.QDockWidget()
            self.dock.setWidget(self.form)

            m.addDockWidget(QtCore.Qt.RightDockWidgetArea, self.dock)

            # load icons
            self.pixyes = QtGui.QPixmap(":/icons/button_valid.svg").scaled(16, 16)
            self.pixno = QtGui.QPixmap(":/icons/button_right.svg").scaled(16, 16)
            self.pixempty = QtGui.QPixmap()

            # fire the loading after displaying the widget
            from draftutils import todo

            # self.load()
            # todo.ToDo.delay(self.load,None)
            QtCore.QTimer.singleShot(1000, self.load)

    def load(self, arg=None):
        import codecs
        import re
        import sys

        if sys.version_info.major < 3:
            import urllib2
        else:
            import urllib.request as urllib2

        # initial loading

        if not hasattr(self, "form") or not self.form or not hasattr(self, "dock"):
            return

        # load tutorial from wiki
        offlineloc = os.path.join(
            FreeCAD.getUserAppDataDir(), "BIM", "Tutorial", "Tutorial.html"
        )
        try:
            u = urllib2.urlopen(URL)
            html = u.read()
            if sys.version_info.major >= 3:
                html = html.decode("utf8")
            html = html.replace("\n", " ")
            html = html.replace('href="/', 'href="https://wiki.freecad.org/')
            html = re.sub(
                '<div id="toc".*?</ul> </div>', "", html
            )  # remove table of contents
            u.close()
        except:
            # unable to load tutorial. Look for offline version
            if os.path.exists(offlineloc):
                f = open(offlineloc)
                html = f.read()
                f.close()
            else:
                FreeCAD.Console.PrintError(
                    translate(
                        "BIM",
                        "Unable to access the tutorial. Verify the internet connection (This is needed only once).",
                    )
                    + "\n"
                )
                return
        else:
            if not os.path.exists(os.path.dirname(offlineloc)):
                os.makedirs(os.path.dirname(offlineloc))
            f = codecs.open(offlineloc, "w", "utf-8")
            f.write(html)
            f.close()

        # setup title and progress bar
        self.steps = len(re.findall(r"infotext", html)) - 1

        # setup description texts and goals
        self.descriptions = [""] + re.findall(
            "<p><br /> </p><p><br /> </p> (.*?)<p><b>Tutorial step", html
        )
        self.goal1 = re.findall(r'goal1">(.*?)</div', html)
        self.goal2 = re.findall(r'goal2">(.*?)</div', html)
        self.test1 = re.findall(r'test1".*?>(.*?)</div', html)
        self.test2 = re.findall(r'test2".*?>(.*?)</div', html)

        # fix mediawiki encodes
        self.test1 = [t.replace("&lt;", "<").replace("&gt;", ">") for t in self.test1]
        self.test2 = [t.replace("&lt;", "<").replace("&gt;", ">") for t in self.test2]

        # download images (QTextEdit cannot load online images)
        self.form.textEdit.setHtml(
            html.replace("inserthere", translate("BIM", "Downloading images…"))
        )
        nd = []
        for descr in self.descriptions:
            imagepaths = re.findall(r'<img.*?src="(.*?)"', descr)
            if imagepaths:
                store = os.path.join(FreeCAD.getUserAppDataDir(), "BIM", "Tutorial")
                if not os.path.exists(store):
                    os.makedirs(store)
                for path in imagepaths:
                    # name = re.findall(r"[\\w.-]+\\.(?i)(?:jpg|png|gif|bmp)",path)
                    # name = re.findall(r"(?i)[\\w.-]+\\.(?:jpg|png|gif|bmp)", path)
                    try:
                        name = os.path.splitext(os.path.basename(path))[0]
                    except:
                        print("unparsable image path:", path)
                    else:
                        storename = os.path.join(store, name)
                        if not os.path.exists(storename):
                            if path.startswith("/images"):
                                # relative path
                                fullpath = "https://www.freecadweb.org/wiki" + path
                            else:
                                fullpath = path
                            u = urllib2.urlopen(fullpath)
                            imagedata = u.read()
                            f = open(storename, "wb")
                            f.write(imagedata)
                            f.close()
                            u.close()
                        # descr = descr.replace(path,"file://"+storename.replace("\\","/"))
                        # fix for windows - seems to work everywhere else too...
                        descr = descr.replace(
                            path, "file:///" + storename.replace("\\", "/")
                        )
            nd.append(descr)
        self.descriptions = nd

        # check where we are
        self.step = PARAMS.GetInt("CurrentTutorialStep", 0)
        if self.step:
            self.update()
        else:
            self.next()

    def next(self):
        self.step += 1
        self.update()

    def previous(self):
        self.step -= 1
        self.update()

    def update(self):
        from PySide import QtCore

        if not hasattr(self, "form") or not self.form or not hasattr(self, "dock"):
            return

        # stay within bounds!
        if self.step > self.steps:
            self.step = self.steps
        elif self.step < 1:
            self.step = 1

        t = self.descriptions[self.step]

        # print(t)

        # set contents
        self.form.textEdit.setHtml(html.replace("inserthere", t))
        self.form.labelGoal1.setText(self.goal1[self.step])
        if self.goal1[self.step]:
            self.form.labelIcon1.setPixmap(self.pixno)
        else:
            self.form.labelIcon1.setPixmap(self.pixempty)
        self.form.labelGoal2.setText(self.goal2[self.step])
        if self.goal2[self.step]:
            self.form.labelIcon2.setPixmap(self.pixno)
        else:
            self.form.labelIcon2.setPixmap(self.pixempty)
        if self.goal1[self.step] or self.goal2[self.step]:
            self.form.labelTasks.show()
        else:
            self.form.labelTasks.hide()
        self.dock.setWindowTitle(
            translate("BIM", "BIM Tutorial - step")
            + " "
            + str(self.step)
            + " / "
            + str(self.steps)
        )
        self.form.progressBar.setValue(int((float(self.step) / self.steps) * 100))

        # save the current step
        PARAMS.SetInt("CurrentTutorialStep", self.step)

        if self.steps > self.step:
            self.form.buttonNext.setEnabled(True)
        else:
            self.form.buttonNext.setEnabled(False)
        if self.step == 1:
            self.form.buttonPrevious.setEnabled(False)
        else:
            self.form.buttonPrevious.setEnabled(True)

        # launch test watcher
        self.done1 = False
        self.done2 = False
        if self.test1[self.step] or self.test2[self.step]:
            QtCore.QTimer.singleShot(TESTINTERVAL, self.checkGoals)

    def checkGoals(self):
        from PySide import QtCore

        if not hasattr(self, "form"):
            return

        if self.goal1[self.step]:
            if self.test1[self.step]:
                if not self.done1:
                    try:
                        result = eval(self.test1[self.step])
                    except:
                        print("BIM Tutorial: unable to eval: " + self.test1[self.step])
                        result = False
                        self.done1 = True
                    if result:
                        self.form.labelIcon1.setPixmap(self.pixyes)
                        self.done1 = True

        if self.goal2[self.step]:
            if self.test2[self.step]:
                if not self.done2:
                    try:
                        result = eval(self.test2[self.step])
                    except:
                        print("BIM Tutorial: unable to eval: " + self.test2[self.step])
                        result = False
                        self.done2 = True
                    if result:
                        self.form.labelIcon2.setPixmap(self.pixyes)
                        self.done2 = True

        if (self.test1[self.step] or self.test2[self.step]) and (
            (not self.done1) or (not self.done2)
        ):
            QtCore.QTimer.singleShot(TESTINTERVAL, self.checkGoals)


FreeCADGui.addCommand("BIM_Tutorial", BIM_Tutorial())
