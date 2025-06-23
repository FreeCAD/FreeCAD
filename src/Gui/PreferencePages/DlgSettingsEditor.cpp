/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#include "PreCompiled.h"
#ifndef _PreComp_
#include <QFontDatabase>
#endif

#include <Base/Color.h>
#include <Gui/PythonEditor.h>
#include <Gui/Tools.h>

#include "DlgSettingsEditor.h"
#include "ui_DlgSettingsEditor.h"


using namespace Gui;
using namespace Gui::Dialog;

namespace Gui
{
namespace Dialog
{
struct DlgSettingsEditorP
{
    QVector<QPair<QString, unsigned int>> colormap;  // Color map
};
}  // namespace Dialog
}  // namespace Gui

namespace
{

/** Get the system-preferred fixed-width font. */
QFont getMonospaceFont()
{
    return QFontDatabase::systemFont(QFontDatabase::FixedFont);
}
}  // namespace

/* TRANSLATOR Gui::Dialog::DlgSettingsEditor */

/**
 *  Constructs a DlgSettingsEditor which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgSettingsEditor::DlgSettingsEditor(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsEditor)
{
    ui->setupUi(this);
    ui->EnableFolding->hide();  // Switch off until we have an editor with folding

    setupConnections();

    ui->textEdit1->setTabStopDistance(40.0);

    d = new DlgSettingsEditorP();
    QColor col;
    col = qApp->palette().windowText().color();
    unsigned int lText = Base::Color::asPackedRGB<QColor>(col);
    d->colormap.push_back(
        QPair<QString, unsigned int>(QString::fromLatin1(QT_TR_NOOP("Text")), lText));

    unsigned int lBookmarks = Base::Color::asPackedRGB<QColor>(QColor(Qt::cyan));
    d->colormap.push_back(
        QPair<QString, unsigned int>(QString::fromLatin1(QT_TR_NOOP("Bookmark")), lBookmarks));

    unsigned int lBreakpnts = Base::Color::asPackedRGB<QColor>(QColor(Qt::red));
    d->colormap.push_back(
        QPair<QString, unsigned int>(QString::fromLatin1(QT_TR_NOOP("Breakpoint")), lBreakpnts));

    unsigned int lKeywords = Base::Color::asPackedRGB<QColor>(QColor(Qt::blue));
    d->colormap.push_back(
        QPair<QString, unsigned int>(QString::fromLatin1(QT_TR_NOOP("Keyword")), lKeywords));

    unsigned int lComments = Base::Color::asPackedRGB<QColor>(QColor(0, 170, 0));
    d->colormap.push_back(
        QPair<QString, unsigned int>(QString::fromLatin1(QT_TR_NOOP("Comment")), lComments));

    unsigned int lBlockCom = Base::Color::asPackedRGB<QColor>(QColor(160, 160, 164));
    d->colormap.push_back(
        QPair<QString, unsigned int>(QString::fromLatin1(QT_TR_NOOP("Block comment")), lBlockCom));

    unsigned int lNumbers = Base::Color::asPackedRGB<QColor>(QColor(Qt::blue));
    d->colormap.push_back(
        QPair<QString, unsigned int>(QString::fromLatin1(QT_TR_NOOP("Number")), lNumbers));

    unsigned int lStrings = Base::Color::asPackedRGB<QColor>(QColor(Qt::red));
    d->colormap.push_back(
        QPair<QString, unsigned int>(QString::fromLatin1(QT_TR_NOOP("String")), lStrings));

    unsigned int lCharacter = Base::Color::asPackedRGB<QColor>(QColor(Qt::red));
    d->colormap.push_back(
        QPair<QString, unsigned int>(QString::fromLatin1(QT_TR_NOOP("Character")), lCharacter));

    unsigned int lClass = Base::Color::asPackedRGB<QColor>(QColor(255, 170, 0));
    d->colormap.push_back(
        QPair<QString, unsigned int>(QString::fromLatin1(QT_TR_NOOP("Class name")), lClass));

    unsigned int lDefine = Base::Color::asPackedRGB<QColor>(QColor(255, 170, 0));
    d->colormap.push_back(
        QPair<QString, unsigned int>(QString::fromLatin1(QT_TR_NOOP("Define name")), lDefine));

    unsigned int lOperat = Base::Color::asPackedRGB<QColor>(QColor(160, 160, 164));
    d->colormap.push_back(
        QPair<QString, unsigned int>(QString::fromLatin1(QT_TR_NOOP("Operator")), lOperat));

    unsigned int lPyOutput = Base::Color::asPackedRGB<QColor>(QColor(170, 170, 127));
    d->colormap.push_back(
        QPair<QString, unsigned int>(QString::fromLatin1(QT_TR_NOOP("Python output")), lPyOutput));

    unsigned int lPyError = Base::Color::asPackedRGB<QColor>(QColor(Qt::red));
    d->colormap.push_back(
        QPair<QString, unsigned int>(QString::fromLatin1(QT_TR_NOOP("Python error")), lPyError));

    unsigned int lCLine = Base::Color::asPackedRGB<QColor>(QColor(224, 224, 224));
    d->colormap.push_back(
        QPair<QString, unsigned int>(QString::fromLatin1(QT_TR_NOOP("Current line highlight")),
                                     lCLine));

    QStringList labels;
    labels << tr("Items");
    ui->displayItems->setHeaderLabels(labels);
    ui->displayItems->header()->hide();
    for (const auto& [textType, textColor] : d->colormap) {
        auto item = new QTreeWidgetItem(ui->displayItems);
        item->setText(0, tr(textType.toLatin1()));
    }
    pythonSyntax = new PythonSyntaxHighlighter(ui->textEdit1);
    pythonSyntax->setDocument(ui->textEdit1->document());
}

/** Destroys the object and frees any allocated resources */
DlgSettingsEditor::~DlgSettingsEditor()
{
    // no need to delete child widgets, Qt does it all for us
    delete pythonSyntax;
    delete d;
}

void DlgSettingsEditor::setupConnections()
{
    // clang-format off
    connect(ui->displayItems, &QTreeWidget::currentItemChanged,
            this, &DlgSettingsEditor::onDisplayItemsCurrentItemChanged);
    connect(ui->colorButton, &ColorButton::changed,
            this, &DlgSettingsEditor::onColorButtonChanged);
    connect(ui->fontFamily, &QComboBox::textActivated,
            this, &DlgSettingsEditor::onFontFamilyActivated);
    connect(ui->fontSize, &PrefSpinBox::textChanged,
            this, &DlgSettingsEditor::onFontSizeValueChanged);
    // clang-format on
}

/** Searches for the color value corresponding to \e name in current editor
 *   settings ColorMap and assigns it to the color button
 *  @see Gui::ColorButton
 */
void DlgSettingsEditor::onDisplayItemsCurrentItemChanged(QTreeWidgetItem* item)
{
    int index = ui->displayItems->indexOfTopLevelItem(item);
    unsigned int col = d->colormap[index].second;
    ui->colorButton->setColor(Base::Color::fromPackedRGB<QColor>(col));
}

/** Updates the color map if a color was changed */
void DlgSettingsEditor::onColorButtonChanged()
{
    QColor col = ui->colorButton->color();
    unsigned int lcol = Base::Color::asPackedRGB<QColor>(col);

    int index = ui->displayItems->indexOfTopLevelItem(ui->displayItems->currentItem());
    d->colormap[index].second = lcol;
    pythonSyntax->setColor(d->colormap[index].first, col);
}

void DlgSettingsEditor::setEditorTabWidth(int tabWidth)
{
    QFontMetrics metric(font());
    int fontSize = QtTools::horizontalAdvance(metric, QLatin1Char('0'));
    ui->textEdit1->setTabStopDistance(tabWidth * fontSize);
}

void DlgSettingsEditor::saveSettings()
{
    ui->EnableLineNumber->onSave();
    ui->EnableBlockCursor->onSave();
    ui->EnableFolding->onSave();
    ui->tabSize->onSave();
    ui->indentSize->onSave();
    ui->radioTabs->onSave();
    ui->radioSpaces->onSave();

    // Saves the color map
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("Editor");
    for (const auto& [textType, textColor] : d->colormap) {
        auto col = static_cast<unsigned long>(textColor);
        hGrp->SetUnsigned(textType.toLatin1(), col);
    }
    hGrp->SetInt("FontSize", ui->fontSize->value());
    hGrp->SetASCII("Font", ui->fontFamily->currentText().toLatin1());

    setEditorTabWidth(ui->tabSize->value());
}

void DlgSettingsEditor::loadSettings()
{
    ui->EnableLineNumber->onRestore();
    ui->EnableBlockCursor->onRestore();
    ui->EnableFolding->onRestore();
    ui->tabSize->onRestore();
    ui->indentSize->onRestore();
    ui->radioTabs->onRestore();
    ui->radioSpaces->onRestore();

    setEditorTabWidth(ui->tabSize->value());
    ui->textEdit1->setPlainText(QStringLiteral("# Short Python sample\n"
                                                    "import sys\n"
                                                    "\n"
                                                    "def foo(begin, end):\n"
                                                    "	i = begin\n"
                                                    "	while i < end:\n"
                                                    "		print(i)\n"
                                                    "		i = i + 1\n"
                                                    "		print(\"Text\")\n"
                                                    "	return None\n"
                                                    "\n"
                                                    "foo(0, 20)\n"));

    // Restores the color map
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("Editor");
    for (auto& [textType, textColor] : d->colormap) {
        auto col = static_cast<unsigned long>(textColor);
        col = hGrp->GetUnsigned(textType.toLatin1(), col);
        textColor = static_cast<unsigned int>(col);
        QColor color = Base::Color::fromPackedRGB<QColor>(col);
        pythonSyntax->setColor(textType, color);
    }

    // fill up font styles
    //
    ui->fontSize->setValue(10);
    ui->fontSize->setValue(hGrp->GetInt("FontSize", ui->fontSize->value()));

    QByteArray defaultMonospaceFont = getMonospaceFont().family().toLatin1();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QStringList familyNames = QFontDatabase().families(QFontDatabase::Any);
    QStringList fixedFamilyNames;
    for (const auto& name : familyNames) {
        if (QFontDatabase().isFixedPitch(name)) {
            // cursor.pcf was removed to cope with a problem with the Qt Font Manager
            // See https://github.com/FreeCAD/FreeCAD/issues/10514 for details
            if (name.compare(QLatin1String("8514oem"), Qt::CaseInsensitive) != 0
                && name.compare(QLatin1String("cursor.pcf"), Qt::CaseInsensitive) != 0)
              {
                fixedFamilyNames.append(name);
            }
        }
    }
#else
    QStringList familyNames = QFontDatabase::families(QFontDatabase::Any);
    QStringList fixedFamilyNames;
    for (const auto& name : familyNames) {
        if (QFontDatabase::isFixedPitch(name)) {
            // cursor.pcf was removed to cope with a problem with the Qt Font Manager
            // See https://github.com/FreeCAD/FreeCAD/issues/10514 for details
            if (name.compare(QLatin1String("8514oem"), Qt::CaseInsensitive) != 0
                && name.compare(QLatin1String("cursor.pcf"), Qt::CaseInsensitive) != 0)
              {
                fixedFamilyNames.append(name);
            }
        }
    }
#endif
    ui->fontFamily->addItems(fixedFamilyNames);
    int index = fixedFamilyNames.indexOf(
        QString::fromLatin1(hGrp->GetASCII("Font", defaultMonospaceFont).c_str()));
    if (index < 0) {
        index = 0;
    }
    ui->fontFamily->setCurrentIndex(index);
    onFontFamilyActivated(ui->fontFamily->currentText());
    ui->displayItems->setCurrentItem(ui->displayItems->topLevelItem(0));
}

void DlgSettingsEditor::resetSettingsToDefaults()
{
    ParameterGrp::handle hGrp;
    hGrp = WindowParameter::getDefaultParameter()->GetGroup("Editor");
    // reset the parameters in the "Editor" group
    for (const auto& [textType, textColor] : d->colormap) {
        hGrp->RemoveUnsigned(textType.toLatin1());
    }
    // reset "FontSize" parameter
    hGrp->RemoveInt("FontSize");
    // reset "Font" parameter
    hGrp->RemoveASCII("Font");

    // finally reset all the parameters associated to Gui::Pref* widgets
    PreferencePage::resetSettingsToDefaults();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsEditor::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        int index = 0;
        for (const auto& [textType, textColor] : d->colormap) {
            ui->displayItems->topLevelItem(index++)->setText(0, tr(textType.toLatin1()));
        }
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgSettingsEditor::onFontFamilyActivated(const QString& fontFamily)
{
    int fontSize = ui->fontSize->value();
    QFont ft(fontFamily, fontSize);
    ui->textEdit1->setFont(ft);
}

void DlgSettingsEditor::onFontSizeValueChanged(const QString&)
{
    onFontFamilyActivated(ui->fontFamily->currentText());
}

#include "moc_DlgSettingsEditor.cpp"
