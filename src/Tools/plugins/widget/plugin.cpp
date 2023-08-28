/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <werner.wm.mayer@gmx.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include <QtDesigner/QDesignerContainerExtension>
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerPropertySheetExtension>
#include <QtDesigner/QExtensionFactory>
#include <QtDesigner/QExtensionManager>

#include <QIcon>
#include <QtGui>
#include <QtPlugin>

#include "customwidgets.h"
#include "plugin.h"


/* XPM */
static const char* urllabel_pixmap[] = {"22 22 3 1",
                                        "# c #000000",
                                        "x c #ffffff",
                                        ". c None",
                                        "......................",
                                        ".......##.............",
                                        "......#xx#............",
                                        "......#xx#............",
                                        "......#xx#............",
                                        "......#xx#............",
                                        "......#xx###..........",
                                        "......#xx#xx###.......",
                                        "......#xx#xx#xx##.....",
                                        "...##.#xx#xx#xx#x#....",
                                        "..#xx##xx#xx#xx#x#....",
                                        "..#xxx#xxxxxxxxxx#....",
                                        "...#xxxxxxxxxxxxx#....",
                                        "....#xxxxxxxxxxxx#....",
                                        "....#xxxxxxxxxxxx#....",
                                        ".....#xxxxxxxxxx#.....",
                                        ".....#xxxxxxxxxx#.....",
                                        "......#xxxxxxxx#......",
                                        "......#xxxxxxxx#......",
                                        "......##########......",
                                        "......##########......",
                                        "......##########......"};

class UrlLabelPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    UrlLabelPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::UrlLabel(parent);
    }
    QString group() const
    {
        return QLatin1String("Display Widgets");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(urllabel_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/Widgets.h");
    }
    QString toolTip() const
    {
        return QLatin1String("Url label");
    }
    QString whatsThis() const
    {
        return QLatin1String("A widget to display a url in a text label.");
    }
    bool isContainer() const
    {
        return false;
    }
    //    QString codeTemplate() const;
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::UrlLabel\" name=\"urlLabel\">\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::UrlLabel");
    }
};

class LocationWidgetPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    LocationWidgetPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::LocationWidget(parent);
    }
    QString group() const
    {
        return QLatin1String("Display Widgets");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(urllabel_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/InputVector.h");
    }
    QString toolTip() const
    {
        return QLatin1String("Location");
    }
    QString whatsThis() const
    {
        return QLatin1String("A widget to define a location.");
    }
    bool isContainer() const
    {
        return false;
    }
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::LocationWidget\" name=\"locationWidget\">\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::LocationWidget");
    }
};

static const char* filechooser_pixmap[] = {"22 22 8 1",
                                           "  c Gray100",
                                           ". c Gray97",
                                           "X c #4f504f",
                                           "o c #00007f",
                                           "O c Gray0",
                                           "+ c none",
                                           "@ c Gray0",
                                           "# c Gray0",
                                           "++++++++++++++++++++++",
                                           "++++++++++++++++++++++",
                                           "++++++++++++++++++++++",
                                           "++++++++++++++++++++++",
                                           "+OOOOOOOOOOOOOOOOOOOO+",
                                           "OOXXXXXXXXXXXXXXXXXXOO",
                                           "OXX.          OO OO  O",
                                           "OX.      oo     O    O",
                                           "OX.      oo     O   .O",
                                           "OX  ooo  oooo   O    O",
                                           "OX    oo oo oo  O    O",
                                           "OX  oooo oo oo  O    O",
                                           "OX oo oo oo oo  O    O",
                                           "OX oo oo oo oo  O    O",
                                           "OX  oooo oooo   O    O",
                                           "OX            OO OO  O",
                                           "OO..................OO",
                                           "+OOOOOOOOOOOOOOOOOOOO+",
                                           "++++++++++++++++++++++",
                                           "++++++++++++++++++++++",
                                           "++++++++++++++++++++++",
                                           "++++++++++++++++++++++"};

class FileChooserPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    FileChooserPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::FileChooser(parent);
    }
    QString group() const
    {
        return QLatin1String("Input Widgets");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(filechooser_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/FileDialog.h");
    }
    QString toolTip() const
    {
        return QLatin1String("File Chooser");
    }
    QString whatsThis() const
    {
        return QLatin1String("A widget to choose a file or directory.");
    }
    bool isContainer() const
    {
        return false;
    }
    //    QString codeTemplate() const;
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::FileChooser\" name=\"fileChooser\">\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::FileChooser");
    }
};

class PrefFileChooserPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    PrefFileChooserPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::PrefFileChooser(parent);
    }
    QString group() const
    {
        return QLatin1String("Preference Widgets");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(filechooser_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/PrefWidgets.h");
    }
    QString toolTip() const
    {
        return QLatin1String("File Chooser");
    }
    QString whatsThis() const
    {
        return QLatin1String("A widget to choose a file or directory.");
    }
    bool isContainer() const
    {
        return false;
    }
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::PrefFileChooser\" name=\"fileChooser\">\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::PrefFileChooser");
    }
};

/* XPM */
static const char* lineedit_pixmap[] = {"22 22 6 1",
                                        "a c #000000",
                                        "# c #000080",
                                        "b c #008080",
                                        "c c #808080",
                                        "d c #c0c0c0",
                                        ". c #ffffff",
                                        "......................",
                                        "......................",
                                        "......................",
                                        "...#aaaaaaaaaaaaaa#...",
                                        ".baccccccccccccccccab.",
                                        ".acccddddddddddddddca.",
                                        "#ccd................d#",
                                        "acc.................da",
                                        "acd.......d....ca.ac.a",
                                        "acd......db......a...a",
                                        "acd.dbbb.dbbbd...a...a",
                                        "acd.ccdbddb.db...a...a",
                                        "acd.dbbbddb..b...a...a",
                                        "acd.bd.bddb..b...a...a",
                                        "acd.bbbbddbbbc...a...a",
                                        "acd..d.....dd..ca.acda",
                                        "#cd.................d#",
                                        ".ac................da.",
                                        ".badd............dda#.",
                                        "...#aaaaaaaaaaaaaa#...",
                                        "......................",
                                        "......................"};

class AccelLineEditPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    AccelLineEditPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::AccelLineEdit(parent);
    }
    QString group() const
    {
        return QLatin1String("Input Widgets");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(lineedit_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/Widgets.h");
    }
    QString toolTip() const
    {
        return QLatin1String("Accelerator Line Edit");
    }
    QString whatsThis() const
    {
        return QLatin1String("A widget to specify accelerator keys.");
    }
    bool isContainer() const
    {
        return false;
    }
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::AccelLineEdit\" name=\"accelEdit\">\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::AccelLineEdit");
    }
};

/* XPM */
static const char* actionselector_pixmap[] = {"22 22 6 1",
                                              "a c #000000",
                                              "# c #000080",
                                              "b c #008080",
                                              "c c #808080",
                                              "d c #c0c0c0",
                                              ". c #ffffff",
                                              "......................",
                                              "......................",
                                              "......................",
                                              "...#aaaaaaaaaaaaaa#...",
                                              ".baccccccccccccccccab.",
                                              ".acccddddddddddddddca.",
                                              "#ccd................d#",
                                              "acc.................da",
                                              "acd.......d....ca.ac.a",
                                              "acd......db......a...a",
                                              "acd.dbbb.dbbbd...a...a",
                                              "acd.ccdbddb.db...a...a",
                                              "acd.dbbbddb..b...a...a",
                                              "acd.bd.bddb..b...a...a",
                                              "acd.bbbbddbbbc...a...a",
                                              "acd..d.....dd..ca.acda",
                                              "#cd.................d#",
                                              ".ac................da.",
                                              ".badd............dda#.",
                                              "...#aaaaaaaaaaaaaa#...",
                                              "......................",
                                              "......................"};

class ActionSelectorPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    ActionSelectorPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::ActionSelector(parent);
    }
    QString group() const
    {
        return QLatin1String("Input Widgets");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(actionselector_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/Widgets.h");
    }
    QString toolTip() const
    {
        return QLatin1String("Action Selector");
    }
    QString whatsThis() const
    {
        return QLatin1String("A widget to select actions.");
    }
    bool isContainer() const
    {
        return false;
    }
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::ActionSelector\" name=\"actionSelector\">\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::ActionSelector");
    }
};

/* XPM */
static const char* inputfield_pixmap[] = {"22 22 6 1",
                                          "a c #000000",
                                          "# c #000080",
                                          "b c #008080",
                                          "c c #808080",
                                          "d c #c0c0c0",
                                          ". c #ffffff",
                                          "......................",
                                          "......................",
                                          "......................",
                                          "...#aaaaaaaaaaaaaa#...",
                                          ".baccccccccccccccccab.",
                                          ".acccddddddddddddddca.",
                                          "#ccd................d#",
                                          "acc.................da",
                                          "acd.......d....ca.ac.a",
                                          "acd......db......a...a",
                                          "acd.dbbb.dbbbd...a...a",
                                          "acd.ccdbddb.db...a...a",
                                          "acd.dbbbddb..b...a...a",
                                          "acd.bd.bddb..b...a...a",
                                          "acd.bbbbddbbbc...a...a",
                                          "acd..d.....dd..ca.acda",
                                          "#cd.................d#",
                                          ".ac................da.",
                                          ".badd............dda#.",
                                          "...#aaaaaaaaaaaaaa#...",
                                          "......................",
                                          "......................"};

class InputFieldPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    InputFieldPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::InputField(parent);
    }
    QString group() const
    {
        return QLatin1String("Input Widgets");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(inputfield_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/InputField.h");
    }
    QString toolTip() const
    {
        return QLatin1String("Input Field");
    }
    QString whatsThis() const
    {
        return QLatin1String("A widget to work with quantities.");
    }
    bool isContainer() const
    {
        return false;
    }
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::InputField\" name=\"inputField\">\n"
               "  <property name=\"unit\" stdset=\"0\">\n"
               "   <string notr=\"true\"></string>\n"
               "  </property>\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::InputField");
    }
};

/* XPM */
static const char* quantityspinbox_pixmap[] = {"22 22 6 1",
                                               "a c #000000",
                                               "# c #000080",
                                               "b c #008080",
                                               "c c #808080",
                                               "d c #c0c0c0",
                                               ". c #ffffff",
                                               "...#aaaaaaaaaaaaaa#...",
                                               ".baccccccccccccccccab.",
                                               ".acccddddddddddddddca.",
                                               "#ccd................d#",
                                               "acc.............dcd.da",
                                               "acd.............dbd..a",
                                               "acd............dcbbd.a",
                                               "acd.d..dd..d...dbbbc.a",
                                               "acddb.dbbdcbb.dbbb#bda",
                                               "acd.b.d.cc..b.bb###bda",
                                               "acd.b...bd.cb.dddccdda",
                                               "acd.b...b..db...dddd.a",
                                               "acd.b..cd...bdddccbbda",
                                               "acd.b.dbbccdb.ccbbbbda",
                                               "acddd.ddd.dd..dbbb#cda",
                                               "acd............bb##cda",
                                               "acd............db#cd.a",
                                               "acd.............bbcdda",
                                               "#cd.............ddd.d#",
                                               ".ac................da.",
                                               ".badd............dda#.",
                                               "...#aaaaaaaaaaaaaa#..."};

class QuantitySpinBoxPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    QuantitySpinBoxPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::QuantitySpinBox(parent);
    }
    QString group() const
    {
        return QLatin1String("Input Widgets");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(quantityspinbox_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/QuantitySpinBox.h");
    }
    QString toolTip() const
    {
        return QLatin1String("Quantity spin box");
    }
    QString whatsThis() const
    {
        return QLatin1String("A widget to work with quantities.");
    }
    bool isContainer() const
    {
        return false;
    }
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::QuantitySpinBox\" name=\"quantitySpinBox\">\n"
               "  <property name=\"unit\" stdset=\"0\">\n"
               "   <string notr=\"true\"></string>\n"
               "  </property>\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::QuantitySpinBox");
    }
};

class PrefUnitSpinBoxPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    PrefUnitSpinBoxPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::PrefUnitSpinBox(parent);
    }
    QString group() const
    {
        return QLatin1String("Preference Widgets");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(quantityspinbox_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/PrefWidgets.h");
    }
    QString toolTip() const
    {
        return QLatin1String("Quantity Spin Box");
    }
    QString whatsThis() const
    {
        return QLatin1String("Quantity Spin box widget.");
    }
    bool isContainer() const
    {
        return false;
    }
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::PrefUnitSpinBox\" name=\"unitSpinBox\">\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::PrefUnitSpinBox");
    }
};

class PrefQuantitySpinBoxPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    PrefQuantitySpinBoxPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::PrefQuantitySpinBox(parent);
    }
    QString group() const
    {
        return QLatin1String("Preference Widgets");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(quantityspinbox_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/PrefWidgets.h");
    }
    QString toolTip() const
    {
        return QLatin1String("Preference Quantity Spin Box");
    }
    QString whatsThis() const
    {
        return QLatin1String("Preference Quantity Spin Box Widget.");
    }
    bool isContainer() const
    {
        return false;
    }
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::PrefQuantitySpinBox\" name=\"unitSpinBox\">\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::PrefQuantitySpinBox");
    }
};

/* XPM */
static const char* iconview_pixmap[] = {"22 22 10 1",
                                        "# c #000000",
                                        "h c #000080",
                                        "f c #0000ff",
                                        "d c #008000",
                                        "e c #008080",
                                        "a c #800000",
                                        "b c #808080",
                                        "c c #c0c0c0",
                                        "g c #ff0000",
                                        ". c #ffffff",
                                        "...################...",
                                        ".a#bbccccccccccccbb#a.",
                                        ".#bcc..............b#.",
                                        "#bb......c.....c....c#",
                                        "#bbbbc..cbbc...bbbc.c#",
                                        "#cccdd....bdb..ccdd..#",
                                        "#cbcb#c.cbcbd..bcb#c.#",
                                        "#cbbb#b..bbb#..cbb#c.#",
                                        "#c..c##...cb#c...c##.#",
                                        "#c...................#",
                                        "#ccbbc..c#bbc..cbbcc.#",
                                        "#c...................#",
                                        "#cbbbaa.cb..cc..c.bb.#",
                                        "#cbccca.c#ccb..cecf#.#",
                                        "#cbcgba..c#b...bfbfh.#",
                                        "#cacbba..bb#c..bbhb#.#",
                                        "#caaaaa.bc.bb..bb###.#",
                                        "#b..................c#",
                                        "#b.bbcc..cbbbb.cbbc.c#",
                                        ".#b................c#.",
                                        ".a#cc............cc##.",
                                        "...################..."};

class CommandIconViewPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    CommandIconViewPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::CommandIconView(parent);
    }
    QString group() const
    {
        return QLatin1String("View Widgets");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(iconview_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/Widgets.h");
    }
    QString toolTip() const
    {
        return QLatin1String("Command View");
    }
    QString whatsThis() const
    {
        return QLatin1String("Area with movable and labeled icons.");
    }
    bool isContainer() const
    {
        return false;
    }
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::CommandIconView\" name=\"iconView\">\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::CommandIconView");
    }
};

/* XPM */
static const char* spinbox_pixmap[] = {"22 22 6 1",
                                       "a c #000000",
                                       "# c #000080",
                                       "b c #008080",
                                       "c c #808080",
                                       "d c #c0c0c0",
                                       ". c #ffffff",
                                       "...#aaaaaaaaaaaaaa#...",
                                       ".baccccccccccccccccab.",
                                       ".acccddddddddddddddca.",
                                       "#ccd................d#",
                                       "acc.............dcd.da",
                                       "acd.............dbd..a",
                                       "acd............dcbbd.a",
                                       "acd.d..dd..d...dbbbc.a",
                                       "acddb.dbbdcbb.dbbb#bda",
                                       "acd.b.d.cc..b.bb###bda",
                                       "acd.b...bd.cb.dddccdda",
                                       "acd.b...b..db...dddd.a",
                                       "acd.b..cd...bdddccbbda",
                                       "acd.b.dbbccdb.ccbbbbda",
                                       "acddd.ddd.dd..dbbb#cda",
                                       "acd............bb##cda",
                                       "acd............db#cd.a",
                                       "acd.............bbcdda",
                                       "#cd.............ddd.d#",
                                       ".ac................da.",
                                       ".badd............dda#.",
                                       "...#aaaaaaaaaaaaaa#..."};

class UIntSpinBoxPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    UIntSpinBoxPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::UIntSpinBox(parent);
    }
    QString group() const
    {
        return QLatin1String("Input Widgets");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(spinbox_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/SpinBox.h");
    }
    QString toolTip() const
    {
        return QLatin1String("Unsigned Spin Box");
    }
    QString whatsThis() const
    {
        return QLatin1String("Spin box widget (spin button).");
    }
    bool isContainer() const
    {
        return false;
    }
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::UIntSpinBox\" name=\"uintSpinBox\">\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::UIntSpinBox");
    }
};

class IntSpinBoxPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    IntSpinBoxPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::IntSpinBox(parent);
    }
    QString group() const
    {
        return QLatin1String("Input Widgets");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(spinbox_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/SpinBox.h");
    }
    QString toolTip() const
    {
        return QLatin1String("Spin Box");
    }
    QString whatsThis() const
    {
        return QLatin1String("Spin box widget (spin button).");
    }
    bool isContainer() const
    {
        return false;
    }
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::IntSpinBox\" name=\"intSpinBox\">\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::IntSpinBox");
    }
};

class DoubleSpinBoxPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    DoubleSpinBoxPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::DoubleSpinBox(parent);
    }
    QString group() const
    {
        return QLatin1String("Input Widgets");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(spinbox_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/SpinBox.h");
    }
    QString toolTip() const
    {
        return QLatin1String("Double Spin Box");
    }
    QString whatsThis() const
    {
        return QLatin1String("Spin box widget (spin button).");
    }
    bool isContainer() const
    {
        return false;
    }
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::DoubleSpinBox\" name=\"doubleSpinBox\">\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::DoubleSpinBox");
    }
};

class PrefSpinBoxPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    PrefSpinBoxPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::PrefSpinBox(parent);
    }
    QString group() const
    {
        return QLatin1String("Preference Widgets");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(spinbox_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/PrefWidgets.h");
    }
    QString toolTip() const
    {
        return QLatin1String("Spin Box");
    }
    QString whatsThis() const
    {
        return QLatin1String("Spin box widget (spin button).");
    }
    bool isContainer() const
    {
        return false;
    }
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::PrefSpinBox\" name=\"spinBox\">\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::PrefSpinBox");
    }
};

/* XPM */
static const char* colorbutton_pixmap[] = {"21 21 7 1",
                                           "d c #000000",
                                           "b c #000080",
                                           "e c #0000ff",
                                           "a c #008080",
                                           "# c #808080",
                                           "c c #c0c0c0",
                                           ". c #ffffff",
                                           ".....................",
                                           ".#abbbbbbbbbbbbbba#c.",
                                           "c#c..............c##.",
                                           "#c................ca.",
                                           "#..................b.",
                                           "#...ddddddddddd....b.",
                                           "#...deeeeeeeeed....b.",
                                           "#...deeeeeeeeed....b.",
                                           "#...deeeeeeeeed....b.",
                                           "#...deeeeeeeeed....b.",
                                           "#...deeeeeeeeed....b.",
                                           "#...deeeeeeeeed....b.",
                                           "#...deeeeeeeeed....b.",
                                           "#...deeeeeeeeed....b.",
                                           "#...deeeeeeeeed....b.",
                                           "#...ddddddddddd....b.",
                                           "#..................b.",
                                           "#.................cb.",
                                           "#cccccccccccccccccca.",
                                           "c#cccccccccccccccc##.",
                                           ".cccccccccccccccccc.."};

class ColorButtonPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    ColorButtonPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::ColorButton(parent);
    }
    QString group() const
    {
        return QLatin1String("Buttons");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(colorbutton_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/Widgets.h");
    }
    QString toolTip() const
    {
        return QLatin1String("Color Button");
    }
    QString whatsThis() const
    {
        return QLatin1String("A button to choose a color.");
    }
    bool isContainer() const
    {
        return false;
    }
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::ColorButton\" name=\"colorButton\">\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::ColorButton");
    }
};

class PrefColorButtonPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    PrefColorButtonPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::PrefColorButton(parent);
    }
    QString group() const
    {
        return QLatin1String("Preference Widgets");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(colorbutton_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/PrefWidgets.h");
    }
    QString toolTip() const
    {
        return QLatin1String("Color Button");
    }
    QString whatsThis() const
    {
        return QLatin1String("A button to choose a color.");
    }
    bool isContainer() const
    {
        return false;
    }
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::PrefColorButton\" name=\"colorButton\">\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::PrefColorButton");
    }
};

/* XPM */
static const char* slider_pixmap[] = {"22 22 5 1",
                                      "b c #000000",
                                      "c c #008080",
                                      "# c #808080",
                                      "a c #c0c0c0",
                                      ". c #ffffff",
                                      "......................",
                                      "......................",
                                      "......................",
                                      "......................",
                                      "......................",
                                      ".........#............",
                                      "........a##...........",
                                      "........a##...........",
                                      "........a##...........",
                                      "..bbbb..a#bbbbbbbbbb..",
                                      ".bbbbb..a#bbbbbbbbbbc.",
                                      ".bb###..a#b########c#.",
                                      ".bbb##..a#b########aa.",
                                      "..cc##..a#b########a..",
                                      "........a##...........",
                                      "........a##...........",
                                      "........a##...........",
                                      "......#####...........",
                                      ".......####...........",
                                      "......................",
                                      "......................",
                                      "......................"};

class PrefSliderPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    PrefSliderPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::PrefSlider(parent);
    }
    QString group() const
    {
        return QLatin1String("Preference Widgets");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(slider_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/PrefWidgets.h");
    }
    QString toolTip() const
    {
        return QLatin1String("Slider");
    }
    QString whatsThis() const
    {
        return QLatin1String("Vertical or horizontal slider.");
    }
    bool isContainer() const
    {
        return false;
    }
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::PrefSlider\" name=\"slider\">\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::PrefSlider");
    }
};

/* XPM */
static const char* radiobutton_pixmap[] = {"22 22 4 1",
                                           "b c #000000",
                                           "# c #808080",
                                           "a c #c0c0c0",
                                           ". c #ffffff",
                                           "......................",
                                           "......................",
                                           "......................",
                                           "......................",
                                           ".......########.......",
                                           "......#####aaa##......",
                                           ".....#b##a...aaa#.....",
                                           "....###aa.aa....a#....",
                                           "....###a.####a...a....",
                                           "....##a.####bba..a....",
                                           "....##.a###bbb#.......",
                                           "....#a.a##bbbb#.......",
                                           "....#a..bbbbbba.......",
                                           "....#aa.abbbb#...a....",
                                           "....##a..a##a....a....",
                                           ".....#a.........a.....",
                                           "......#a.......a......",
                                           ".......#aa...aa.......",
                                           "......................",
                                           "......................",
                                           "......................",
                                           "......................"};

class PrefRadioButtonPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    PrefRadioButtonPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::PrefRadioButton(parent);
    }
    QString group() const
    {
        return QLatin1String("Preference Widgets");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(radiobutton_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/PrefWidgets.h");
    }
    QString toolTip() const
    {
        return QLatin1String("Radio Button");
    }
    QString whatsThis() const
    {
        return QLatin1String("Radio button with a text or pixmap label.");
    }
    bool isContainer() const
    {
        return false;
    }
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::PrefRadioButton\" name=\"radioButton\">\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::PrefRadioButton");
    }
};

/* XPM */
static const char* checkbox_pixmap[] = {"22 22 4 1",
                                        "# c #000000",
                                        "a c #808080",
                                        "b c #c0c0c0",
                                        ". c #ffffff",
                                        "......................",
                                        "......................",
                                        "......................",
                                        "......................",
                                        "....###########aaa....",
                                        "....##aaaaaaaaaabb....",
                                        "....#aabbbbbbbbbbb....",
                                        "....#abbbbbbbbaa......",
                                        "....#abbbbbbba#a......",
                                        "....#ababbbba##a......",
                                        "....#ab#abba###a......",
                                        "....#ab##aa###ab......",
                                        "....#ab######abb......",
                                        "....#abb####abbb......",
                                        "....#abbb##abbbb......",
                                        "....aabbbbabbbb.......",
                                        "....abb......b........",
                                        "....abb...............",
                                        "......................",
                                        "......................",
                                        "......................",
                                        "......................"};

class PrefCheckBoxPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    PrefCheckBoxPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::PrefCheckBox(parent);
    }
    QString group() const
    {
        return QLatin1String("Preference Widgets");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(checkbox_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/PrefWidgets.h");
    }
    QString toolTip() const
    {
        return QLatin1String("Check Box");
    }
    QString whatsThis() const
    {
        return QLatin1String("Checkbox with a text label.");
    }
    bool isContainer() const
    {
        return false;
    }
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::PrefCheckBox\" name=\"checkBox\">\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::PrefCheckBox");
    }
};

/* XPM */
static const char* combobox_pixmap[] = {"22 22 8 1",
                                        "a c #000000",
                                        "# c #000080",
                                        "e c #008080",
                                        "f c #800000",
                                        "b c #808080",
                                        "c c #c0c0c0",
                                        "d c #ff0000",
                                        ". c #ffffff",
                                        ".#aaaaaaaaaaaaaaaaaa#.",
                                        "#bbccccccccccccccccdd#",
                                        "accee#########e.addfaa",
                                        "#c..............a.fa.#",
                                        "e#aaaaaaaaaaaaaaaaaa#e",
                                        "....#c...............#",
                                        "....ac...............a",
                                        "....ac.ccbbbbbbbbeb..a",
                                        "....ac.bbbeeeeeee##c.a",
                                        "....ac.bee########ac.a",
                                        "....ac..cccccccccccc.a",
                                        "....ac.ccccccccccbec.a",
                                        "....ac.cccccccccbbec.a",
                                        "....ac.bcbbbbbbbbbec.a",
                                        "....ac..cccccccccccc.a",
                                        "....ac.cbbeeeeeee#bc.a",
                                        "....ac.bee########ac.a",
                                        "....ab.b##aaaaaaaaacca",
                                        "....#bc.ccccccccccccc#",
                                        ".....ab............ca.",
                                        ".....eacc.........ca#.",
                                        ".......#aaaaaaaaaa#..."};

class PrefComboBoxPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    PrefComboBoxPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::PrefComboBox(parent);
    }
    QString group() const
    {
        return QLatin1String("Preference Widgets");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(combobox_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/PrefWidgets.h");
    }
    QString toolTip() const
    {
        return QLatin1String("Combo Box");
    }
    QString whatsThis() const
    {
        return QLatin1String("Combined button and popup list.");
    }
    bool isContainer() const
    {
        return false;
    }
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::PrefComboBox\" name=\"comboBox\">\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::PrefComboBox");
    }
};

class PrefLineEditPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    PrefLineEditPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::PrefLineEdit(parent);
    }
    QString group() const
    {
        return QLatin1String("Preference Widgets");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(lineedit_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/PrefWidgets.h");
    }
    QString toolTip() const
    {
        return QLatin1String("Line Edit");
    }
    QString whatsThis() const
    {
        return QLatin1String("One-line text editor.");
    }
    bool isContainer() const
    {
        return false;
    }
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::PrefLineEdit\" name=\"lineEdit\">\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::PrefLineEdit");
    }
};

class PrefDoubleSpinBoxPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    PrefDoubleSpinBoxPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::PrefDoubleSpinBox(parent);
    }
    QString group() const
    {
        return QLatin1String("Preference Widgets");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(spinbox_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/PrefWidgets.h");
    }
    QString toolTip() const
    {
        return QLatin1String("Double Spin Box");
    }
    QString whatsThis() const
    {
        return QLatin1String("Spin box widget that can work with doubles.");
    }
    bool isContainer() const
    {
        return false;
    }
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::PrefDoubleSpinBox\" name=\"doubleSpinBox\">\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::PrefDoubleSpinBox");
    }
};


/* XPM */
static const char* fontbox_pixmap[] = {"22 22 8 1",
                                       "a c #000000",
                                       "# c #000080",
                                       "e c #008080",
                                       "f c #800000",
                                       "b c #808080",
                                       "c c #c0c0c0",
                                       "d c #ff0000",
                                       ". c #ffffff",
                                       ".#aaaaaaaaaaaaaaaaaa#.",
                                       "#bbccccccccccccccccdd#",
                                       "accee#########e.addfaa",
                                       "#c..............a.fa.#",
                                       "e#aaaaaaaaaaaaaaaaaa#e",
                                       "....#c...............#",
                                       "....ac...............a",
                                       "....ac.ccbbbbbbbbeb..a",
                                       "....ac.bbbeeeeeee##c.a",
                                       "....ac.bee########ac.a",
                                       "....ac..cccccccccccc.a",
                                       "....ac.ccccccccccbec.a",
                                       "....ac.cccccccccbbec.a",
                                       "....ac.bcbbbbbbbbbec.a",
                                       "....ac..cccccccccccc.a",
                                       "....ac.cbbeeeeeee#bc.a",
                                       "....ac.bee########ac.a",
                                       "....ab.b##aaaaaaaaacca",
                                       "....#bc.ccccccccccccc#",
                                       ".....ab............ca.",
                                       ".....eacc.........ca#.",
                                       ".......#aaaaaaaaaa#..."};

class PrefFontBoxPlugin: public QDesignerCustomWidgetInterface
{
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    PrefFontBoxPlugin()
    {}
    QWidget* createWidget(QWidget* parent)
    {
        return new Gui::PrefFontBox(parent);
    }
    QString group() const
    {
        return QLatin1String("Preference Widgets");
    }
    QIcon icon() const
    {
        return QIcon(QPixmap(fontbox_pixmap));
    }
    QString includeFile() const
    {
        return QLatin1String("Gui/PrefWidgets.h");
    }
    QString toolTip() const
    {
        return QLatin1String("Font Box");
    }
    QString whatsThis() const
    {
        return QLatin1String("Font box widget (spin button).");
    }
    bool isContainer() const
    {
        return false;
    }
    QString domXml() const
    {
        return "<ui language=\"c++\">\n"
               " <widget class=\"Gui::PrefFontBox\" name=\"fontBox\">\n"
               " </widget>\n"
               "</ui>";
    }
    QString name() const
    {
        return QLatin1String("Gui::PrefFontBox");
    }
};

/* XPM */
/*
static char *listbox_pixmap[]={
"22 22 6 1",
"# c #000000",
"c c #800000",
"d c #808000",
"a c #808080",
"b c #c0c0c0",
". c #ffffff",
".####################.",
"#aabbbbbbbbbbbbb#abb.#",
"#abcccccccccccdb#b#a.#",
"#b..............#....#",
"c####################c",
"#b...............#...#",
"#b...............#...#",
"#b.###########a..#...#",
"#b...............#.#.#",
"#b.cccccccccccd..#.#.#",
"#b...............#.#.#",
"#b.cccccccccccd..#.#.#",
"#b...............#.#.#",
"#b.###########a..#.#.#",
"#b...............#...#",
"#b.###########a..#...#",
"#b...............#...#",
"#a.###########a..#..b#",
"#a...............#..b#",
".#a..............#.b#.",
".c#bb............#b##.",
"...################..."};
*/
CustomWidgetPlugin::CustomWidgetPlugin(QObject* parent)
    : QObject(parent)
{}

QList<QDesignerCustomWidgetInterface*> CustomWidgetPlugin::customWidgets() const
{
    QList<QDesignerCustomWidgetInterface*> cw;
    cw.append(new UrlLabelPlugin);
    cw.append(new LocationWidgetPlugin);
    cw.append(new FileChooserPlugin);
    cw.append(new AccelLineEditPlugin);
    cw.append(new ActionSelectorPlugin);
    cw.append(new InputFieldPlugin);
    cw.append(new QuantitySpinBoxPlugin);
    cw.append(new CommandIconViewPlugin);
    cw.append(new UIntSpinBoxPlugin);
    cw.append(new IntSpinBoxPlugin);
    cw.append(new DoubleSpinBoxPlugin);
    cw.append(new ColorButtonPlugin);
    cw.append(new PrefFileChooserPlugin);
    cw.append(new PrefSpinBoxPlugin);
    cw.append(new PrefColorButtonPlugin);
    cw.append(new PrefSliderPlugin);
    cw.append(new PrefRadioButtonPlugin);
    cw.append(new PrefCheckBoxPlugin);
    cw.append(new PrefComboBoxPlugin);
    cw.append(new PrefLineEditPlugin);
    cw.append(new PrefDoubleSpinBoxPlugin);
    cw.append(new PrefFontBoxPlugin);
    cw.append(new PrefUnitSpinBoxPlugin);
    cw.append(new PrefQuantitySpinBoxPlugin);
    return cw;
}
