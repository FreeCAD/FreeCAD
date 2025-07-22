# (c) 2024 Werner Mayer LGPL

__title__="FreeCAD Python documentation"
__author__ = "Werner Mayer"
__url__ = "https://www.freecad.org"
__doc__ = "Helper module to use pydoc"


import os, sys, pydoc, pkgutil

class FreeCADDoc(pydoc.HTMLDoc):
    def index(self, dir, shadowed=None):
        """ Generate an HTML index for a directory of modules."""
        modpkgs = []
        if shadowed is None: shadowed = {}
        for importer, name, ispkg in pkgutil.iter_modules([dir]):
            if name == 'Init':
                continue
            if name == 'InitGui':
                continue
            if name[-2:] == '_d':
                continue
            modpkgs.append((name, '', ispkg, name in shadowed))
            shadowed[name] = 1

        if len(modpkgs) == 0:
            return None

        modpkgs.sort()
        contents = self.multicolumn(modpkgs, self.modpkglink)
        try:
            return self.bigsection(dir, '#ffffff', '#ee77aa', contents)
        except Exception as e:
            return self.bigsection(dir, 'pkg-content', contents)

def bltinlink(name):
    return '<a href=\"%s.html\">%s</a>' % (name, name)

def getIndexOld():
    pydoc.html = FreeCADDoc()
    title = 'FreeCAD Python Modules Index'

    heading = pydoc.html.heading('<big><big><strong>Python: Index of Modules</strong></big></big>','#ffffff', '#7799ee')

    names = list(filter(lambda x: x != '__main__', sys.builtin_module_names))
    contents = pydoc.html.multicolumn(names, bltinlink)
    indices = ['<p>' + pydoc.html.bigsection('Built-in Modules', '#ffffff', '#ee77aa', contents)]

    names = ['FreeCAD', 'FreeCADGui']
    contents = pydoc.html.multicolumn(names, bltinlink)
    indices.append('<p>' + pydoc.html.bigsection('Built-in FreeCAD Modules', '#ffffff', '#ee77aa', contents))

    seen = {}
    for dir in sys.path:
        dir = os.path.realpath(dir)
        ret = pydoc.html.index(dir, seen)
        if ret != None:
            indices.append(ret)

    contents = heading + ' '.join(indices) + '''<p align=right>
<font color=\"#909090\" face=\"helvetica, arial\"><strong>
pydoc</strong> by Ka-Ping Yee &lt;ping@lfw.org&gt;</font>'''

    htmldocument = pydoc.html.page(title, contents)
    return htmldocument

def getIndexNew():
    pydoc.html = FreeCADDoc()
    title = 'FreeCAD Python Modules Index'

    heading = pydoc.html.heading(
        '<strong class="title">Index of Modules</strong>'
    )

    names = list(filter(lambda x: x != '__main__', sys.builtin_module_names))
    contents = pydoc.html.multicolumn(names, bltinlink)
    indices = ['<p>' + pydoc.html.bigsection('Built-in Modules', 'index', contents)]

    names = ['FreeCAD', 'FreeCADGui']
    contents = pydoc.html.multicolumn(names, bltinlink)
    indices.append('<p>' + pydoc.html.bigsection('Built-in FreeCAD Modules', 'index', contents))

    seen = {}
    for dir in sys.path:
        dir = os.path.realpath(dir)
        ret = pydoc.html.index(dir, seen)
        if ret != None:
            indices.append(ret)

    contents = heading + ' '.join(indices) + '''<p align=right>
<font color=\"#909090\" face=\"helvetica, arial\"><strong>
pydoc</strong> by Ka-Ping Yee &lt;ping@lfw.org&gt;</font>'''

    htmldocument = pydoc.html.page(title, contents)
    return htmldocument

def getIndex():
    try:
        return getIndexOld()
    except Exception as e:
        return getIndexNew()

def getPage(page):
    object, name = pydoc.resolve(page)
    page = pydoc.html.page(pydoc.describe(object), pydoc.html.document(object, name))
    return page
