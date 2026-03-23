#!/usr/bin/env pythonw

from AppKit import *
from Foundation import *
from PyObjCTools import NibClassBuilder, AppHelper
import sys, objc

objc.loadBundle('Sc21', globals(),
                bundle_path='/Library/Frameworks/Sc21.framework')

NibClassBuilder.extractClasses('MainMenu.nib')
            
class AppController (NibClassBuilder.AutoBaseClass):

    def awakeFromNib(self):
        self.filenametext.setStringValue_(u"None")

    def open_(self, sender):
        panel = NSOpenPanel.openPanel()
        panel.beginSheetForDirectory_file_types_modalForWindow_modalDelegate_didEndSelector_contextInfo_(None, None, [u'wrl'], NSApp().mainWindow(), self, "openPanelDidEnd:returnCode:contextInfo:", 0)
    def openPanelDidEnd_returnCode_contextInfo_(self, panel, code, ct):
        if code == NSOKButton:
            sg = self.coincontroller.sceneGraph()
            sg.readFromFile_(panel.filename())
            sg.viewAll()
            self.filenametext.setStringValue_(panel.filename())

    openPanelDidEnd_returnCode_contextInfo_ =  AppHelper.endSheetMethod(
        openPanelDidEnd_returnCode_contextInfo_)

if __name__ == "__main__":
    sys.exit(NSApplicationMain(sys.argv))
