# -*- coding: utf-8 -*-

import os
import sys
if sys.version_info.major < 3:
    import urllib2
else:
    import urllib.request as urllib2

from PySide import QtGui

# Qt translation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)

ssl_ctx = None
try:
    import ssl
except ImportError:
    pass
else:
    try:
        ssl_ctx = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
    except AttributeError:
        pass


def urlopen(url):
    """Opens an url with urllib2"""
    if ssl_ctx:
        u = urllib2.urlopen(url, context=ssl_ctx)
    else:
        u = urllib2.urlopen(url)
    return u
