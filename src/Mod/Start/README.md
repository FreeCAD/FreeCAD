## Start Workbench

This Workbench is intended to eventually replace (and be renamed to) Start. Its main reason for existing is to
eliminate FreeCAD's dependency on QtWebEngine by redesigning Start to eliminate the HTML component.

The long-term plan for this workbench is to migrate from Qt Widgets to QtQuick/QML with a C++ backend providing access
to things like FreeCAD's Recent Files list. This switch will happen sometime after we no longer have to support building
on Ubuntu 20.04 LTS, which still uses Qt 5.12. The cMake integration of QML and C++ together in a single project is
greatly improved in Qt 5.15 and later.

In the meantime the workbench is written in C++ so that the models can be reused later, and only the UI itself will
have to change.

### Structure

The main UI file for the Start screen is in `Gui/StartView.cpp` -- that class is a `QScrollArea` that gets embedded
into a new FreeCAD `MDIView`. Inside the scroll area are three regions:

1. **New File**. A set of `QPushButtons` with embedded `QLayouts` displaying an image and two lines of text. Currently
laid out manually in a `QGridLayout`, but eventually it would be nice to dynamically calculate that layout to allow the
buttons to exist on a single line when there is enough space for them.
2. **Recent Files**. One of two "File Card" regions, this shows a list of recent files. It uses the
Model-View-Controller architecture for flexibility and re-usability. In that architecture, the data being displayed is
called the "model", and the actual mechanism for doing the display is called the "view". Qt further differentiates
between the overall view and the display of the individual items in the view. The items are rendered by a "delegate"
when they are too complex to be displayed by the simple view (e.g. if you want images and text in a particular layout).
    * The "model" in this case is `RecentFilesModel`, a simple read-only interface to
FreeCAD's preferences system, where it gets the list of recent files. That class is in`App/RecentFilesModel.*`. It is
implemented using a set of User Roles, one for each piece of data about a file. Not all data is available for all files.
For example, when given `const QModelIndex &index`, you can call `index.data(DisplayedFilesModelRoles::author)` to get
the "author" metadata item of an `FCStd` file. See the `DisplayedFilesModelRoles` enumeration for possible values. These
roles are also exposed to QML via their names.
    * The View is a class derived from `QListView`, `Gui/FileCardView.*`, whose only function beyond the standard
`QListView` is to implement the "height for width" functionality, so the widget can properly resize based on the number
of file cards and the screen width, laying them out in a grid.
    * The file cards are rendered using the `FileCardDelegate` class in `Gui/FileCardDelegate.*`. That class uses
a simple `QVBoxLayout` to paint the icon, filename, and file size.
3. **Examples**. Another "File Card" widget, using the same classes as Recent Files, but with a different model. In this
case the model is `ExamplesModel` in `App/ExamplesModel.*`. It fetches a read-only list of files from the FreeCAD
`resources/examples` directory and displays them.

### UI Design

This Workbench does the minimum amount of design customization, preferring to allow Stylesheet authors control over the
display via the normal QSS mechanisms in Qt. There are three FreeCAD Parameters that control the spacing between the
widgets and the size of the icons, all in the `BaseApp/Preferences/Mod/Start/` preferences group:
* `FileCardSpacing` (default: 20). The space between and around the individual File Cards.
* `FileThumbnailIconsSize` (default: 128). The size of the file thumbnail on the File Cards.
* `NewFileIconSize` (default: 48). The size of the icons on each of the "new file" buttons.

At present none of these are directly exposed to the user, and the new Start workbench does not have a preferences
panel. The parameters are intended to be used by Preference Pack authors to assist in customization of FreeCAD themes.
It is likely that this will be expanded once feedback from theme designers is received.
