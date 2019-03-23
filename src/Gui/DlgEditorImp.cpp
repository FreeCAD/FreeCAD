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
# include <QComboBox>
# include <QFontDatabase>
# include <QHeaderView>
#endif

#include "DlgEditorImp.h"
#include "PrefWidgets.h"
#include "PythonEditor.h"

using namespace Gui;
using namespace Gui::Dialog;

namespace Gui {
namespace Dialog {
struct DlgSettingsEditorP
{
    QVector<QPair<QString, unsigned int> > colormap; // Color map
};
} // namespace Dialog
} // namespace Gui

/* TRANSLATOR Gui::Dialog::DlgSettingsEditorImp */

/**
 *  Constructs a DlgSettingsEditorImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgSettingsEditorImp::DlgSettingsEditorImp( QWidget* parent )
  : PreferencePage( parent )
{
    this->setupUi(this);
    this->EnableFolding->hide(); // Switch off until we have an editor with folding

    d = new DlgSettingsEditorP();
    QColor col;
    col = Qt::black; 
    unsigned int lText = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8);
    d->colormap.push_back(QPair<QString, unsigned int>
        (QString::fromLatin1(QT_TR_NOOP("Text")), lText));
    col = Qt::cyan; 
    unsigned int lBookmarks = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8);
    d->colormap.push_back(QPair<QString, unsigned int>
        (QString::fromLatin1(QT_TR_NOOP("Bookmark")), lBookmarks));
    col = Qt::red; 
    unsigned int lBreakpnts = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8);
    d->colormap.push_back(QPair<QString, unsigned int>
        (QString::fromLatin1(QT_TR_NOOP("Breakpoint")), lBreakpnts));
    col = Qt::blue; 
    unsigned int lKeywords = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8);
    d->colormap.push_back(QPair<QString, unsigned int>
        (QString::fromLatin1(QT_TR_NOOP("Keyword")), lKeywords));
    col.setRgb(0, 170, 0); 
    unsigned int lComments = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8);
    d->colormap.push_back(QPair<QString, unsigned int>
        (QString::fromLatin1(QT_TR_NOOP("Comment")), lComments));
    col.setRgb(160, 160, 164); 
    unsigned int lBlockCom = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8);
    d->colormap.push_back(QPair<QString, unsigned int>
        (QString::fromLatin1(QT_TR_NOOP("Block comment")), lBlockCom));
    col = Qt::blue; 
    unsigned int lNumbers = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8);
    d->colormap.push_back(QPair<QString, unsigned int>
        (QString::fromLatin1(QT_TR_NOOP("Number")), lNumbers));
    col = Qt::red; 
    unsigned int lStrings = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8);
    d->colormap.push_back(QPair<QString, unsigned int>
        (QString::fromLatin1(QT_TR_NOOP("String")), lStrings));
    col = Qt::red; 
    unsigned int lCharacter = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8);
    d->colormap.push_back(QPair<QString, unsigned int>
        (QString::fromLatin1(QT_TR_NOOP("Character")), lCharacter));
    col.setRgb(255, 170, 0); 
    unsigned int lClass = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8);
    d->colormap.push_back(QPair<QString, unsigned int>
        (QString::fromLatin1(QT_TR_NOOP("Class name")), lClass));
    col.setRgb(255, 170, 0); 
    unsigned int lDefine = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8);
    d->colormap.push_back(QPair<QString, unsigned int>
        (QString::fromLatin1(QT_TR_NOOP("Define name")), lDefine));
    col.setRgb(160, 160, 164); 
    unsigned int lOperat = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8);
    d->colormap.push_back(QPair<QString, unsigned int>
        (QString::fromLatin1(QT_TR_NOOP("Operator")), lOperat));
    col.setRgb(170, 170, 127); 
    unsigned int lPyOutput = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8);
    d->colormap.push_back(QPair<QString, unsigned int>
        (QString::fromLatin1(QT_TR_NOOP("Python output")), lPyOutput));
    col = Qt::red; 
    unsigned int lPyError = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8);
    d->colormap.push_back(QPair<QString, unsigned int>
        (QString::fromLatin1(QT_TR_NOOP("Python error")), lPyError));
    col.setRgb(224, 224, 224); 
    unsigned int lCLine = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8);
    d->colormap.push_back(QPair<QString, unsigned int>
        (QString::fromLatin1(QT_TR_NOOP("Current line highlight")), lCLine));

    QStringList labels; labels << tr("Items");
    this->displayItems->setHeaderLabels(labels);
    this->displayItems->header()->hide();
    for (QVector<QPair<QString, unsigned int> >::ConstIterator it = d->colormap.begin(); it != d->colormap.end(); ++it) {
        QTreeWidgetItem* item = new QTreeWidgetItem(this->displayItems);
        item->setText(0, tr((*it).first.toLatin1()));
    }
    pythonSyntax = new PythonSyntaxHighlighter(textEdit1);
    pythonSyntax->setDocument(textEdit1->document());
}

/** Destroys the object and frees any allocated resources */
DlgSettingsEditorImp::~DlgSettingsEditorImp()
{
    // no need to delete child widgets, Qt does it all for us
    delete pythonSyntax;
    delete d;
}

/** Searches for the color value corresponding to \e name in current editor
 *   settings ColorMap and assigns it to the color button
 *  @see Gui::ColorButton
 */
void DlgSettingsEditorImp::on_displayItems_currentItemChanged(QTreeWidgetItem *item)
{
    int index = displayItems->indexOfTopLevelItem(item);
    unsigned int col = d->colormap[index].second;
    colorButton->setColor(QColor((col >> 24) & 0xff, (col >> 16) & 0xff, (col >> 8) & 0xff));
}

/** Updates the color map if a color was changed */
void DlgSettingsEditorImp::on_colorButton_changed()
{
    QColor col = colorButton->color();
    unsigned int lcol = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8);

    int index = displayItems->indexOfTopLevelItem(displayItems->currentItem());
    d->colormap[index].second = lcol;
    pythonSyntax->setColor( d->colormap[index].first, col );
}

void DlgSettingsEditorImp::saveSettings()
{
    EnableLineNumber->onSave();
    EnableFolding->onSave();
    tabSize->onSave();
    indentSize->onSave();
    radioTabs->onSave();
    radioSpaces->onSave();

    // Saves the color map
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("Editor");
    for (QVector<QPair<QString, unsigned int> >::ConstIterator it = d->colormap.begin(); it != d->colormap.end(); ++it) {
        unsigned long col = static_cast<unsigned long>((*it).second);
        hGrp->SetUnsigned((*it).first.toLatin1(), col);
    }

    hGrp->SetInt( "FontSize", fontSize->value() );
    hGrp->SetASCII( "Font", fontFamily->currentText().toLatin1() );
}

void DlgSettingsEditorImp::loadSettings()
{
    EnableLineNumber->onRestore();
    EnableFolding->onRestore();
    tabSize->onRestore();
    indentSize->onRestore();
    radioTabs->onRestore();
    radioSpaces->onRestore();

    textEdit1->setPlainText(QString::fromLatin1(
        "# Short Python sample\n"
        "import sys\n"
        "def foo(begin, end):\n"
        "	i=begin\n"
        "	while (i<end):\n"
        "		print i\n"
        "		i=i+1\n"
        "		print \"Text\"\n"
        "\n"
        "foo(0, 20))\n"));

    // Restores the color map
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("Editor");
    for (QVector<QPair<QString, unsigned int> >::Iterator it = d->colormap.begin(); it != d->colormap.end(); ++it){
        unsigned long col = static_cast<unsigned long>((*it).second);
        col = hGrp->GetUnsigned((*it).first.toLatin1(), col);
        (*it).second = static_cast<unsigned int>(col);
        QColor color;
        color.setRgb((col >> 24) & 0xff, (col >> 16) & 0xff, (col >> 8) & 0xff);
        pythonSyntax->setColor( (*it).first, color );
    }

    // fill up font styles
    //
    fontSize->setValue(10);
    fontSize->setValue( hGrp->GetInt("FontSize", fontSize->value()) );

    QByteArray fontName = this->font().family().toLatin1();

    QFontDatabase fdb;
    QStringList familyNames = fdb.families( QFontDatabase::Any );
    fontFamily->addItems(familyNames);
    int index = familyNames.indexOf(QString::fromLatin1(hGrp->GetASCII("Font", fontName).c_str()));
    if (index < 0) index = 0;
    fontFamily->setCurrentIndex(index);
    on_fontFamily_activated(this->fontFamily->currentText());

    displayItems->setCurrentItem(displayItems->topLevelItem(0));
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsEditorImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        int index = 0;
        for (QVector<QPair<QString, unsigned int> >::ConstIterator it = d->colormap.begin(); it != d->colormap.end(); ++it)
            this->displayItems->topLevelItem(index++)->setText(0, tr((*it).first.toLatin1()));
        this->retranslateUi(this);
    } else {
        QWidget::changeEvent(e);
    }
}

void DlgSettingsEditorImp::on_fontFamily_activated(const QString& fontFamily)
{
    int fontSize = this->fontSize->value();
    QFont ft(fontFamily, fontSize);
    textEdit1->setFont(ft);
}

void DlgSettingsEditorImp::on_fontSize_valueChanged(const QString&)
{
    on_fontFamily_activated(this->fontFamily->currentText());
}

#include "moc_DlgEditorImp.cpp"
