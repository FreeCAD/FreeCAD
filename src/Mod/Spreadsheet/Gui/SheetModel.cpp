#include "PreCompiled.h"

#ifndef _PreComp_
#include <QtCore>
# include <QApplication>
# include <QMessageBox>
#endif

#include "SheetModel.h"
#include <App/Expression.h>
#include "../App/Sheet.h"
#include <Gui/Command.h>
#include <strstream>

using namespace SpreadsheetGui;
using namespace App;

SheetModel::SheetModel(Spreadsheet::Sheet *_sheet, QObject *parent)
    : QAbstractTableModel(parent)
    , sheet(_sheet)
{
    cellUpdatedConnection = sheet->cellUpdated.connect(bind(&SheetModel::cellUpdated, this, _1, _2));
}

SheetModel::~SheetModel()
{
}

int SheetModel::rowCount(const QModelIndex &parent) const
{
    return 16384;
}

int SheetModel::columnCount(const QModelIndex &parent) const
{
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

    // Nominator
    appendUnit(unit.getLengthDimension(), true, "mm", numerator);
    appendUnit(unit.getMassDimension(), true, "kg", numerator);
    appendUnit(unit.getTimeDimension(), true, "s", numerator);
    appendUnit(unit.getElectricCurrentDimension(), true, "A", numerator);
    appendUnit(unit.getThermodynamicTemperatureDimension(), true, "K", numerator);
    appendUnit(unit.getAmountOfSubstanceDimension(), true, "mol", numerator);
    appendUnit(unit.getLuminoseIntensityDimension(), true, "cd", numerator);
    appendUnit(unit.getAngleDimension(), true, "deg", numerator);

    // Denominator
    appendUnit(unit.getLengthDimension(), false, "mm", denominator);
    appendUnit(unit.getMassDimension(), false, "kg", denominator);
    appendUnit(unit.getTimeDimension(), false, "s", denominator);
    appendUnit(unit.getElectricCurrentDimension(), false, "A", denominator);
    appendUnit(unit.getThermodynamicTemperatureDimension(), false, "K", denominator);
    appendUnit(unit.getAmountOfSubstanceDimension(), false, "mol", denominator);
    appendUnit(unit.getLuminoseIntensityDimension(), false, "cd", denominator);
    appendUnit(unit.getAngleDimension(), false, "deg", denominator);

    std::string unitStr;

    if (numerator.size() > 0) {
        for (int i = 0; i < numerator.size(); ++i) {
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
        for (int i = 0; i < denominator.size(); ++i) {
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
    int row = index.row();
    int col = index.column();

    // Get edit value by querying the sheet
    if (role == Qt::EditRole || role == Qt::StatusTipRole) {
        const std::string str = sheet->getCellString(row, col);

        return QVariant(QString::fromUtf8(str.c_str()));
    }

    // Get display value as computed property
    std::string address = Spreadsheet::Sheet::toAddress(row, col);
    Property * prop = sheet->getPropertyByName(address.c_str());
    int this_row = row;
    int this_col = col;

    Color color;
    if (role == Qt::TextColorRole && sheet->getForeground(this_row, this_col, color))
        return QVariant::fromValue(QColor(255.0 * color.r, 255.0 * color.g, 255.0 * color.b, 255.0 * color.a));

    if (role == Qt::BackgroundRole && sheet->getBackground(this_row, this_col, color))
        return QVariant::fromValue(QColor(255.0 * color.r, 255.0 * color.g, 255.0 * color.b, 255.0 * color.a));

    int alignment;
    if (role == Qt::TextAlignmentRole && sheet->getAlignment(this_row, this_col, alignment)) {
        int qtAlignment = 0;

        if (alignment & 0x01)
            qtAlignment |= Qt::AlignLeft;
        if (alignment & 0x02)
            qtAlignment |= Qt::AlignHCenter;
        if (alignment & 0x04)
            qtAlignment |= Qt::AlignRight;
        if (alignment & 0x10)
            qtAlignment |= Qt::AlignTop;
        if (alignment & 0x20)
            qtAlignment |= Qt::AlignVCenter;
        if (alignment & 0x40)
            qtAlignment |= Qt::AlignBottom;

        return QVariant::fromValue(qtAlignment);
    }

    std::set<std::string> style;
    if (role == Qt::FontRole && sheet->getStyle(this_row, this_col, style)) {
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

    if (prop->isDerivedFrom(App::PropertyString::getClassTypeId())) {
        /* String */
        const App::PropertyString * stringProp = static_cast<const App::PropertyString*>(prop);

        switch (role) {
        case Qt::TextColorRole:
            return QVariant::fromValue(QColor(Qt::black));
        case Qt::DisplayRole:
            return QVariant(QString::fromUtf8(stringProp->getValue()));
        default:
            return QVariant();
        }
    }
    else if (prop->isDerivedFrom(App::PropertyFloat::getClassTypeId())) {
        /* Number */
        const App::PropertyFloat * floatProp = static_cast<const App::PropertyFloat*>(prop);

        switch (role) {
        case  Qt::TextColorRole:
            if (floatProp->getValue() < 0)
                return QVariant::fromValue(QColor(Qt::red));
            else
                return QVariant::fromValue(QColor(Qt::blue));
        case Qt::TextAlignmentRole:
            return (int)(Qt::AlignRight | Qt::AlignVCenter);
        case Qt::DisplayRole: {
            QString v;

            const Base::Unit & computedUnit = sheet->getComputedUnit(row, col);
            Spreadsheet::Sheet::DisplayUnit displayUnit;

            if (sheet->getUnit(row, col, displayUnit)) {
                if (computedUnit.isEmpty() || computedUnit == displayUnit.unit)
                    v = QString::number(floatProp->getValue() / displayUnit.scaler) + QString::fromStdString(" " + displayUnit.stringRep);
                else
                    v = QString::fromUtf8("ERR:unit");
            }
            else {
                if (!computedUnit.isEmpty())
                    v = QString::number(floatProp->getValue()) + QString::fromStdString(" " + getUnitString(computedUnit));
                else
                    v = QString::number(floatProp->getValue());
            }

//#define DEBUG_DEPS
#ifdef DEBUG_DEPS
            const Expression * e = sheet->getCell(row, col);
            std::set<std::string> deps;

            e->getDeps(deps);

            if (deps.size() > 0) {
                v += QString::fromUtf8("[");
                for (std::set<std::string>::const_iterator i = deps.begin(); i != deps.end(); ++i)
                    v += QString::fromUtf8(" ") + QString::fromStdString(*i);
                v += QString::fromUtf8(" ]");
            }
#endif
            return QVariant(v);
        }
        default:
            return QVariant();
        }
    }
}

QVariant SheetModel::headerData(int section, Qt::Orientation orientation, int role) const
 {
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
     }
     return QVariant();
}

bool SheetModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (role == Qt::DisplayRole) {
        // Nothing to do, it will get updated by the sheet in the application logic
    }
    else if (role == Qt::EditRole) {
        int row = index.row();
        int col = index.column();

        try {
            std::string address = Spreadsheet::Sheet::toAddress(row, col);
            QString str = value.toString();

            if (sheet->getCellString(row, col) != str.toStdString()) {
                str.replace(QString::fromUtf8("'"), QString::fromUtf8("\\'"));
                Gui::Command::openCommand("Edit cell");
                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.set('%s', '%s')", sheet->getNameInDocument(),
                                        address.c_str(), str.toUtf8().constData());
                Gui::Command::commitCommand();
            }
        }
        catch (const Base::Exception& e) {
            QMessageBox::critical(qApp->activeWindow(), QObject::tr("Cell contents"), QString::fromUtf8(e.what()));
            Gui::Command::abortCommand();
        }
    }
    return true;
}

Qt::ItemFlags SheetModel::flags(const QModelIndex & /*index*/) const
{
    return Qt::ItemIsSelectable |  Qt::ItemIsEditable | Qt::ItemIsEnabled;
}

void SheetModel::cellUpdated(int row, int col)
{
    QModelIndex i = index(row, col);

    dataChanged(i, i);
}

#include "moc_SheetModel.cpp"
