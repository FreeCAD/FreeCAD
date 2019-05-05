/***************************************************************************
 *   Copyright (c) Eivind Kvedalen (eivind@kvedalen.name) 2015             *
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
# include <QtCore>
# include <QApplication>
# include <QLocale>
# include <QMessageBox>
# include <QTextDocument>
#endif

#include <App/Document.h>
#include <Gui/Application.h>
#include "SheetModel.h"
#include <Mod/Spreadsheet/App/Utils.h>
#include "../App/Sheet.h"
#include <Gui/Command.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <boost/bind.hpp>

using namespace SpreadsheetGui;
using namespace Spreadsheet;
using namespace App;

SheetModel::SheetModel(Sheet *_sheet, QObject *parent)
    : QAbstractTableModel(parent)
    , sheet(_sheet)
{
    cellUpdatedConnection = sheet->cellUpdated.connect(bind(&SheetModel::cellUpdated, this, _1));

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Spreadsheet");
    aliasBgColor = QColor(Base::Tools::fromStdString(hGrp->GetASCII("AliasedCellBackgroundColor", "#feff9e")));
    textFgColor = QColor(Base::Tools::fromStdString(hGrp->GetASCII("TextColor", "#000000")));
    positiveFgColor = QColor(Base::Tools::fromStdString(hGrp->GetASCII("PositiveNumberColor", "#000000")));
    negativeFgColor = QColor(Base::Tools::fromStdString(hGrp->GetASCII("NegativeNumberColor", "#000000")));
}

SheetModel::~SheetModel()
{
    cellUpdatedConnection.disconnect();
}

int SheetModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 16384;
}

int SheetModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 26 * 26 + 26;
}

static void appendUnit(int l, bool isNumerator, std::string unit, std::vector<std::string> & v)
{
    if (l == 0)
        return;
    if ((l < 0) ^ isNumerator ) {
        std::ostringstream s;

        s << unit;
        if (abs(l) > 1)
            s << "^" << abs(l);

        v.push_back(s.str());
    }
}

static std::string getUnitString(const Base::Unit & unit)
{
    std::vector<std::string> numerator;
    std::vector<std::string> denominator;
    const Base::UnitSignature & sig = unit.getSignature();

    // Nominator
    appendUnit(sig.Length, true, "mm", numerator);
    appendUnit(sig.Mass, true, "kg", numerator);
    appendUnit(sig.Time, true, "s", numerator);
    appendUnit(sig.ElectricCurrent, true, "A", numerator);
    appendUnit(sig.ThermodynamicTemperature, true, "K", numerator);
    appendUnit(sig.AmountOfSubstance, true, "mol", numerator);
    appendUnit(sig.LuminousIntensity, true, "cd", numerator);
    appendUnit(sig.Angle, true, "deg", numerator);

    // Denominator
    appendUnit(sig.Length, false, "mm", denominator);
    appendUnit(sig.Mass, false, "kg", denominator);
    appendUnit(sig.Time, false, "s", denominator);
    appendUnit(sig.ElectricCurrent, false, "A", denominator);
    appendUnit(sig.ThermodynamicTemperature, false, "K", denominator);
    appendUnit(sig.AmountOfSubstance, false, "mol", denominator);
    appendUnit(sig.LuminousIntensity, false, "cd", denominator);
    appendUnit(sig.Angle, false, "deg", denominator);

    std::string unitStr;

    if (numerator.size() > 0) {
        for (std::size_t i = 0; i < numerator.size(); ++i) {
            if (i > 0)
                unitStr += "*";
            unitStr += numerator[i];
        }
    }

    if (denominator.size() > 0) {
        if (numerator.size() == 0)
            unitStr = "1";
        unitStr += "/";

        if (denominator.size() > 1)
            unitStr += "(";
        for (std::size_t i = 0; i < denominator.size(); ++i) {
            if (i > 0)
                unitStr += "*";
            unitStr += denominator[i];
        }
        if (denominator.size() > 1)
            unitStr += ")";
    }

    return unitStr;
}

QVariant SheetModel::data(const QModelIndex &index, int role) const
{
    static const Cell * emptyCell = new Cell(CellAddress(0, 0), 0);
    int row = index.row();
    int col = index.column();
    const Cell * cell = sheet->getCell(CellAddress(row, col));

    if (cell == 0)
        cell = emptyCell;

//#define DEBUG_DEPS
#ifdef DEBUG_DEPS
    if (role == Qt::ToolTipRole) {
        QString v;

        std::set<std::string> deps = sheet->dependsOn(CellAddress(row, col));
        std::set<std::string> provides;

        sheet->providesTo(CellAddress(row, col), provides);

        if (deps.size() > 0) {
            v += QString::fromUtf8("Depends on:");
            for (std::set<std::string>::const_iterator i = deps.begin(); i != deps.end(); ++i)
                v += QString::fromUtf8("\n\t") + Tools::fromStdString(*i);
            v += QString::fromUtf8("\n");
        }
        if (provides.size() > 0) {
            v += QString::fromUtf8("Used by:");
            for (std::set<std::string>::const_iterator i = provides.begin(); i != provides.end(); ++i)
                v += QString::fromUtf8("\n\t") + Tools::fromStdString(*i);
            v += QString::fromUtf8("\n");
        }
        return QVariant(v);
    }
#else
    if (!cell->hasException() && role == Qt::ToolTipRole) {
        std::string alias;
        if (cell->getAlias(alias))
            return QVariant(Base::Tools::fromStdString(alias));
    }
#endif

    if (cell->hasException()) {
        switch (role) {
        case Qt::ToolTipRole: {
#if QT_VERSION >= 0x050000
            QString txt(Base::Tools::fromStdString(cell->getException()).toHtmlEscaped());
#else
            QString txt(Qt::escape(Base::Tools::fromStdString(cell->getException())));
#endif
            return QVariant(QString::fromLatin1("<pre>%1</pre>").arg(txt));
        }
        case Qt::DisplayRole: {
#ifdef DEBUG_DEPS
            return QVariant::fromValue(QString::fromUtf8("#ERR: %1").arg(Tools::fromStdString(cell->getException())));
#else
            std::string str;
            if(cell->getStringContent(str))
                return QVariant::fromValue(QString::fromUtf8(str.c_str()));
            return QVariant::fromValue(QString::fromUtf8("#ERR"));
#endif
        }
        case Qt::TextColorRole:
            return QVariant::fromValue(QColor(255.0, 0, 0));
        case Qt::TextAlignmentRole:
            return QVariant(Qt::AlignVCenter | Qt::AlignLeft);
        default:
            break;
        }
    }

    // Get edit value by querying the sheet
    if (role == Qt::EditRole) {
        auto contents = cell->getEditData(true);
        if(contents.empty())
            return QVariant();
        else if(contents.size()==1)
            return QVariant(QString::fromUtf8(contents[0].c_str()));
        else {
            QStringList list;
            for(auto &content : contents)
                list << QString::fromUtf8(content.c_str());
            return QVariant(list);
        }
    }
    if(cell == emptyCell)
        return QVariant();

    // Get display value as computed property
    std::string address = CellAddress(row, col).toString();
    Property * prop = sheet->getPropertyByName(address.c_str());

    if (role == Qt::BackgroundRole) {
        Color color;

        if (cell->getBackground(color))
            return QVariant::fromValue(QColor(255.0 * color.r, 255.0 * color.g, 255.0 * color.b, 255.0 * color.a));
        else {
            std::string alias;
            if (cell->getAlias(alias)) {
                return QVariant::fromValue(aliasBgColor);
            }
            else
                return QVariant();
        }
    }

    int qtAlignment = 0;

    int alignment;
    cell->getAlignment(alignment);

    if (alignment & Cell::ALIGNMENT_LEFT)
        qtAlignment |= Qt::AlignLeft;
    if (alignment & Cell::ALIGNMENT_HCENTER)
        qtAlignment |= Qt::AlignHCenter;
    if (alignment & Cell::ALIGNMENT_RIGHT)
        qtAlignment |= Qt::AlignRight;
    if (alignment & Cell::ALIGNMENT_TOP)
        qtAlignment |= Qt::AlignTop;
    if (alignment & Cell::ALIGNMENT_VCENTER)
        qtAlignment |= Qt::AlignVCenter;
    if (alignment & Cell::ALIGNMENT_BOTTOM)
        qtAlignment |= Qt::AlignBottom;

    std::set<std::string> style;
    if (role == Qt::FontRole && cell->getStyle(style)) {
        QFont f;

        for (std::set<std::string>::const_iterator i = style.begin(); i != style.end(); ++i) {
            if (*i == "bold")
                f.setBold(true);
            else if (*i == "italic")
                f.setItalic(true);
            else if (*i == "underline")
                f.setUnderline(true);
        }

        return QVariant::fromValue(f);
    }

    if (!prop) {
        switch (role) {
        case  Qt::TextColorRole: {
            return QColor(0, 0, 255.0);
        }
        case Qt::TextAlignmentRole: {
            qtAlignment = Qt::AlignHCenter | Qt::AlignVCenter;
            return QVariant::fromValue(qtAlignment);
        }
        case Qt::DisplayRole:
            if(cell->getExpression())
                return QVariant(QLatin1String("#PENDING"));
            else
                return QVariant();
        default:
            return QVariant();
        }
    } else if (prop->isDerivedFrom(App::PropertyString::getClassTypeId())) {
        /* String */
        const App::PropertyString * stringProp = static_cast<const App::PropertyString*>(prop);

        switch (role) {
        case Qt::TextColorRole: {
            Color color;

            if (cell->getForeground(color))
                return QVariant::fromValue(QColor(255.0 * color.r, 255.0 * color.g, 255.0 * color.b, 255.0 * color.a));
            else
                return QVariant(QColor(textFgColor));
        }
        case Qt::DisplayRole:
            return QVariant(QString::fromUtf8(stringProp->getValue()));
        case Qt::TextAlignmentRole: {
            if (alignment & Cell::ALIGNMENT_HIMPLIED) {
                qtAlignment &= ~(Qt::AlignLeft | Qt::AlignHCenter | Qt::AlignRight);
                qtAlignment |= Qt::AlignLeft;
            }
            if (alignment & Cell::ALIGNMENT_VIMPLIED) {
                qtAlignment &= ~(Qt::AlignTop | Qt::AlignVCenter | Qt::AlignBottom);
                qtAlignment |= Qt::AlignVCenter;
            }
            return QVariant::fromValue(qtAlignment);
        }
        default:
            return QVariant();
        }
    }
    else if (prop->isDerivedFrom(App::PropertyQuantity::getClassTypeId())) {
        /* Number */
        const App::PropertyQuantity * floatProp = static_cast<const App::PropertyQuantity*>(prop);

        switch (role) {
        case  Qt::TextColorRole: {
            Color color;

            if (cell->getForeground(color))
                return QVariant::fromValue(QColor(255.0 * color.r, 255.0 * color.g, 255.0 * color.b, 255.0 * color.a));
            else {
                if (floatProp->getValue() < 0)
                    return QVariant::fromValue(QColor(negativeFgColor));
                else
                    return QVariant::fromValue(QColor(positiveFgColor));
            }
        }
        case Qt::TextAlignmentRole: {
            if (alignment & Cell::ALIGNMENT_HIMPLIED) {
                qtAlignment &= ~(Qt::AlignLeft | Qt::AlignHCenter | Qt::AlignRight);
                qtAlignment |= Qt::AlignRight;
            }
            if (alignment & Cell::ALIGNMENT_VIMPLIED) {
                qtAlignment &= ~(Qt::AlignTop | Qt::AlignVCenter | Qt::AlignBottom);
                qtAlignment |= Qt::AlignVCenter;
            }
            return QVariant::fromValue(qtAlignment);
        }
        case Qt::DisplayRole: {
            QString v;
            const Base::Unit & computedUnit = floatProp->getUnit();
            DisplayUnit displayUnit;

            // Display locale specific decimal separator (#0003875,#0003876)
            if (cell->getDisplayUnit(displayUnit)) {
                if (computedUnit.isEmpty() || computedUnit == displayUnit.unit) {
                    QString number = QLocale::system().toString(floatProp->getValue() / displayUnit.scaler,'f',Base::UnitsApi::getDecimals());
                    //QString number = QString::number(floatProp->getValue() / displayUnit.scaler);
                    v = number + Base::Tools::fromStdString(" " + displayUnit.stringRep);
                }
                else {
                    v = QString::fromUtf8("#ERR: unit");
                }
            }
            else {
                QString number = QLocale::system().toString(floatProp->getValue(),'f',Base::UnitsApi::getDecimals());
                //QString number = QString::number(floatProp->getValue());
                if (!computedUnit.isEmpty())
                    v = number + Base::Tools::fromStdString(" " + getUnitString(computedUnit));
                else
                    v = number;
            }

            return QVariant(v);
        }
        default:
            return QVariant();
        }
    }
    else if (prop->isDerivedFrom(App::PropertyFloat::getClassTypeId()) 
                || prop->isDerivedFrom(App::PropertyInteger::getClassTypeId())) 
    {
        /* Number */
        double d;
        if(prop->isDerivedFrom(App::PropertyFloat::getClassTypeId()))
            d = static_cast<const App::PropertyFloat*>(prop)->getValue();
        else
            d = static_cast<const App::PropertyInteger*>(prop)->getValue();

        switch (role) {
        case  Qt::TextColorRole: {
            Color color;

            if (cell->getForeground(color))
                return QVariant::fromValue(QColor(255.0 * color.r, 255.0 * color.g, 255.0 * color.b, 255.0 * color.a));
            else {
                if (d < 0)
                    return QVariant::fromValue(QColor(negativeFgColor));
                else
                    return QVariant::fromValue(QColor(positiveFgColor));
            }
        }
        case Qt::TextAlignmentRole: {
            if (alignment & Cell::ALIGNMENT_HIMPLIED) {
                qtAlignment &= ~(Qt::AlignLeft | Qt::AlignHCenter | Qt::AlignRight);
                qtAlignment |= Qt::AlignRight;
            }
            if (alignment & Cell::ALIGNMENT_VIMPLIED) {
                qtAlignment &= ~(Qt::AlignTop | Qt::AlignVCenter | Qt::AlignBottom);
                qtAlignment |= Qt::AlignVCenter;
            }
            return QVariant::fromValue(qtAlignment);
        }
        case Qt::DisplayRole: {
            QString v;
            DisplayUnit displayUnit;

            // Display locale specific decimal separator (#0003875,#0003876)
            if (cell->getDisplayUnit(displayUnit)) {
                QString number = QLocale::system().toString(d / displayUnit.scaler,'f',Base::UnitsApi::getDecimals());
                //QString number = QString::number(d / displayUnit.scaler);
                v = number + Base::Tools::fromStdString(" " + displayUnit.stringRep);
            }
            else {
                v = QLocale::system().toString(d,'f',Base::UnitsApi::getDecimals());
                //v = QString::number(d);
            }
            return QVariant(v);
        }
        default:
            return QVariant();
        }
    }
    else if (prop->isDerivedFrom(App::PropertyPythonObject::getClassTypeId())) {
        auto pyProp = static_cast<const App::PropertyPythonObject*>(prop);

        switch (role) {
        case  Qt::TextColorRole: {
            Color color;

            if (cell->getForeground(color))
                return QVariant::fromValue(QColor(255.0 * color.r, 255.0 * color.g, 255.0 * color.b, 255.0 * color.a));
            else 
                return QVariant(QColor(textFgColor));
        }
        case Qt::TextAlignmentRole: {
            if (alignment & Cell::ALIGNMENT_HIMPLIED) {
                qtAlignment &= ~(Qt::AlignLeft | Qt::AlignHCenter | Qt::AlignRight);
                qtAlignment |= Qt::AlignHCenter;
            }
            if (alignment & Cell::ALIGNMENT_VIMPLIED) {
                qtAlignment &= ~(Qt::AlignTop | Qt::AlignVCenter | Qt::AlignBottom);
                qtAlignment |= Qt::AlignVCenter;
            }
            return QVariant::fromValue(qtAlignment);
        }
        case Qt::DisplayRole: {
            if(cell->getEditMode()) {
                auto res = cell->getEditData(true);
                if(res.size())
                    return QVariant(QString::fromUtf8(res.front().c_str()));
            }
            Base::PyGILStateLocker lock;
            std::string value;
            try {
                value = pyProp->getValue().as_string();
            } catch (Py::Exception &) {
                Base::PyException e;
                value = "#ERR: ";
                value += e.what();
            } catch (Base::Exception &e) {
                value = "#ERR: ";
                value += e.what();
            } catch (...) {
                value = "#ERR: unknown exception";
            }
            return QVariant(QString::fromUtf8(value.c_str()));
        }
        default:
            return QVariant();
        }
    }

    return QVariant();
}

QVariant SheetModel::headerData(int section, Qt::Orientation orientation, int role) const
 {
    if (role == Qt::SizeHintRole) {
        if (orientation == Qt::Horizontal)
            return QVariant(QSize(sheet->getColumnWidth(section), PropertyRowHeights::defaultHeight));
        else
            return QVariant(QSize(PropertyColumnWidths::defaultHeaderWidth, sheet->getRowHeight(section)));
    }
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            static QString labels = QString::fromUtf8("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
            if (section < 26) {
                return QVariant(labels[section]);
            }
            else {
                section -= 26;
                return QVariant(QString(labels[section / 26]) + QString(labels[section % 26]));
            }
        }
        else {
            return QString::number(section + 1);
        }
    } else if (role == Qt::TextColorRole) {
        if((orientation == Qt::Horizontal && sheet->hiddenColumns.getValues().count(section)) ||
           (orientation == Qt::Vertical && sheet->hiddenRows.getValues().count(section)))
           return QColor(0,0,255);
    }
    return QVariant();
}

bool SheetModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (role == Qt::DisplayRole) {
        // Nothing to do, it will get updated by the sheet in the application logic
    }
    else if (role == Qt::EditRole) {
        CellAddress address(index.row(), index.column());

        try {
            QString str = value.toString();
            Gui::Command::openCommand("Edit cell");
            // Because of possible complication of recursively escaped
            // characters, let's take a shortcut and bypass the command
            // interface for now.
#if 0
            std::string strAddress = address.toString();
            str.replace(QString::fromUtf8("\\"), QString::fromUtf8("\\\\"));
            str.replace(QString::fromUtf8("'"), QString::fromUtf8("\\'"));
            FCMD_OBJ_CMD(sheet,"set('" << strAddress << "','" << 
                    str.toUtf8().constData() << "')");
#else
            sheet->editCell(address, str.toUtf8().constData());
#endif
            Gui::Command::commitCommand();
            Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        }
        catch (const Base::Exception& e) {
            e.ReportException();
            Gui::Command::abortCommand();
            return false;
        }
    }
    return true;
}

Qt::ItemFlags SheetModel::flags(const QModelIndex & /*index*/) const
{
    return Qt::ItemIsSelectable |  Qt::ItemIsEditable | Qt::ItemIsEnabled;
}

void SheetModel::cellUpdated(CellAddress address)
{
    QModelIndex i = index(address.row(), address.col());

    dataChanged(i, i);
}

#include "moc_SheetModel.cpp"
