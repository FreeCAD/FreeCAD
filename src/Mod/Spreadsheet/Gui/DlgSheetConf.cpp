/****************************************************************************
 *   Copyright (c) 2019 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"

#include <QMessageBox>

#include <App/AutoTransaction.h>
#include <App/Document.h>
#include <App/ExpressionParser.h>
#include <App/Range.h>
#include <Base/Tools.h>
#include <Gui/CommandT.h>

#include "DlgSheetConf.h"
#include "ui_DlgSheetConf.h"


using namespace App;
using namespace Spreadsheet;
using namespace SpreadsheetGui;

DlgSheetConf::DlgSheetConf(Sheet* sheet, Range range, QWidget* parent)
    : QDialog(parent)
    , sheet(sheet)
    , ui(new Ui::DlgSheetConf)
{
    ui->setupUi(this);

    if (range.colCount() == 1) {
        auto to = range.to();
        to.setCol(CellAddress::MAX_COLUMNS - 1);
        range = Range(range.from(), to);
    }

    ui->lineEditStart->setText(QString::fromLatin1(range.from().toString().c_str()));
    ui->lineEditEnd->setText(QString::fromLatin1(range.to().toString().c_str()));

    ui->lineEditProp->setDocumentObject(sheet, false);

    connect(ui->btnDiscard, &QPushButton::clicked, this, &DlgSheetConf::onDiscard);

    CellAddress from, to;
    std::string rangeConf;
    ObjectIdentifier path;
    auto prop = prepare(from, to, rangeConf, path, true);
    if (prop) {
        ui->lineEditProp->setText(QString::fromUtf8(path.toString().c_str()));
        if (auto group = prop->getGroup()) {
            ui->lineEditGroup->setText(QString::fromUtf8(group));
        }
    }

    ui->lineEditStart->setText(QString::fromLatin1(from.toString().c_str()));
    ui->lineEditEnd->setText(QString::fromLatin1(to.toString().c_str()));
}

DlgSheetConf::~DlgSheetConf()
{
    delete ui;
}

App::Property* DlgSheetConf::prepare(CellAddress& from,
                                     CellAddress& to,
                                     std::string& rangeConf,
                                     ObjectIdentifier& path,
                                     bool init)
{
    from = sheet->getCellAddress(ui->lineEditStart->text().trimmed().toLatin1().constData());
    to = sheet->getCellAddress(ui->lineEditEnd->text().trimmed().toLatin1().constData());

    if (from.col() >= to.col()) {
        FC_THROWM(Base::RuntimeError, "Invalid cell range");
    }

    // Setup row as parameters, and column as configurations
    to.setRow(from.row());

    CellAddress confFrom(from.row() + 1, from.col());
    rangeConf = confFrom.toString();
    // rangeConf is supposed to hold the range of string cells, each
    // holding the name of a configuration. The '|' below indicates a
    // growing but continuous column, so that we can auto include new
    // configurations. We'll bind the string list to a
    // PropertyEnumeration for dynamical switching of the
    // configuration.
    rangeConf += ":|";

    if (!init) {
        std::string exprTxt(ui->lineEditProp->text().trimmed().toUtf8().constData());
        ExpressionPtr expr;
        try {
            expr.reset(App::Expression::parse(sheet, exprTxt));
        }
        catch (Base::Exception& e) {
            e.ReportException();
            FC_THROWM(Base::RuntimeError, "Failed to parse expression for property");
        }
        if (expr->hasComponent()
            || !expr->isDerivedFrom(App::VariableExpression::getClassTypeId())) {
            FC_THROWM(Base::RuntimeError, "Invalid property expression: " << expr->toString());
        }

        path = static_cast<App::VariableExpression*>(expr.get())->getPath();
        auto obj = path.getDocumentObject();
        if (!obj) {
            FC_THROWM(Base::RuntimeError, "Invalid object referenced in: " << expr->toString());
        }

        int pseudoType;
        auto prop = path.getProperty(&pseudoType);
        if (pseudoType
            || (prop
                && (!prop->isDerivedFrom(App::PropertyEnumeration::getClassTypeId())
                    || !prop->testStatus(App::Property::PropDynamic)))) {
            FC_THROWM(Base::RuntimeError, "Invalid property referenced in: " << expr->toString());
        }
        return prop;
    }

    Cell* cell = sheet->getCell(from);
    if (cell && cell->getExpression()) {
        auto expr = cell->getExpression();
        if (expr->isDerivedFrom(FunctionExpression::getClassTypeId())) {
            auto fexpr = Base::freecad_dynamic_cast<FunctionExpression>(cell->getExpression());
            if (fexpr
                && (fexpr->getFunction() == FunctionExpression::HREF
                    || fexpr->getFunction() == FunctionExpression::HIDDENREF)
                && fexpr->getArgs().size() == 1) {
                expr = fexpr->getArgs().front();
            }
        }
        auto vexpr = Base::freecad_dynamic_cast<VariableExpression>(expr);
        if (vexpr) {
            auto prop =
                Base::freecad_dynamic_cast<PropertyEnumeration>(vexpr->getPath().getProperty());
            if (prop) {
                auto obj = Base::freecad_dynamic_cast<DocumentObject>(prop->getContainer());
                if (obj && prop->hasName()) {
                    path = ObjectIdentifier(sheet);
                    path.setDocumentObjectName(obj, true);
                    path << ObjectIdentifier::SimpleComponent(prop->getName());
                    return prop;
                }
            }
        }
    }
    return nullptr;
}

void DlgSheetConf::accept()
{
    bool commandActive = false;
    try {
        std::string rangeConf;
        CellAddress from, to;
        ObjectIdentifier path;
        App::Property* prop = prepare(from, to, rangeConf, path, false);

        Range range(from, to);

        // check rangeConf, make sure it is a sequence of string only
        Range r(sheet->getRange(rangeConf.c_str()));
        do {
            auto cell = sheet->getCell(*r);
            if (cell && cell->getExpression()) {
                ExpressionPtr expr(cell->getExpression()->eval());
                if (expr->isDerivedFrom(StringExpression::getClassTypeId())) {
                    continue;
                }
            }
            FC_THROWM(Base::RuntimeError,
                      "Expects cell " << r.address() << " evaluates to string.\n"
                                      << rangeConf
                                      << " is supposed to contain a list of configuration names");
        } while (r.next());

        std::string exprTxt(ui->lineEditProp->text().trimmed().toUtf8().constData());
        App::ExpressionPtr expr(App::Expression::parse(sheet, exprTxt));
        if (expr->hasComponent()
            || !expr->isDerivedFrom(App::VariableExpression::getClassTypeId())) {
            FC_THROWM(Base::RuntimeError, "Invalid property expression: " << expr->toString());
        }

        AutoTransaction guard("Setup conf table");
        commandActive = true;

        // unbind any previous binding
        int count = range.rowCount() * range.colCount();
        for (int i = 0; i < count; ++i) {
            auto r = range;
            auto binding = sheet->getCellBinding(r);
            if (!binding) {
                break;
            }
            Gui::cmdAppObjectArgs(sheet,
                                  "setExpression('.cells.%s.%s.%s', None)",
                                  binding == PropertySheet::BindingNormal ? "Bind"
                                                                          : "BindHiddenRef",
                                  r.from().toString(),
                                  r.to().toString());
        }

        auto obj = path.getDocumentObject();
        if (!obj) {
            FC_THROWM(Base::RuntimeError, "Object not found");
        }

        // Add a dynamic PropertyEnumeration for user to switch the configuration
        std::string propName = path.getPropertyName();
        QString groupName = ui->lineEditGroup->text().trimmed();
        if (!prop) {
            prop = obj->addDynamicProperty("App::PropertyEnumeration",
                                           propName.c_str(),
                                           groupName.toUtf8().constData());
        }
        else if (groupName.size()) {
            obj->changeDynamicProperty(prop, groupName.toUtf8().constData(), nullptr);
        }
        prop->setStatus(App::Property::CopyOnChange, true);

        // Bind the enumeration items to the column of configuration names
        Gui::cmdAppObjectArgs(obj,
                              "setExpression('%s.Enum', '%s.cells[<<%s>>]')",
                              propName,
                              sheet->getFullName(),
                              rangeConf);

        Gui::cmdAppObjectArgs(obj, "recompute()");

        // Bind the first cell to string value of the PropertyEnumeration. We
        // could have just bind the entire row as below, but binding the first
        // cell separately using a simpler expression can make it easy for us
        // to extract the name of the PropertyEnumeration for editing or unsetup.
        Gui::cmdAppObjectArgs(sheet,
                              "set('%s', '=hiddenref(%s.String)')",
                              from.toString(CellAddress::Cell::ShowRowColumn),
                              prop->getFullName());

        // Adjust the range to skip the first cell
        range = Range(from.row(), from.col() + 1, to.row(), to.col());

        // Formulate expression to calculate the row binding using
        // PropertyEnumeration
        Gui::cmdAppObjectArgs(
            sheet,
            "setExpression('.cells.Bind.%s.%s', "
            "'tuple(.cells, <<%s>> + str(hiddenref(%s)+%d), <<%s>> + str(hiddenref(%s)+%d))')",
            range.from().toString(CellAddress::Cell::ShowRowColumn),
            range.to().toString(CellAddress::Cell::ShowRowColumn),
            range.from().toString(CellAddress::Cell::ShowColumn),
            prop->getFullName(),
            from.row() + 2,
            range.to().toString(CellAddress::Cell::ShowColumn),
            prop->getFullName(),
            from.row() + 2);

        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        Gui::Command::commitCommand();
        QDialog::accept();
    }
    catch (Base::Exception& e) {
        e.ReportException();
        QMessageBox::critical(this, tr("Setup configuration table"), QString::fromUtf8(e.what()));
        if (commandActive) {
            Gui::Command::abortCommand();
        }
    }
}

void DlgSheetConf::onDiscard()
{
    bool commandActive = false;
    try {
        std::string rangeConf;
        CellAddress from, to;
        ObjectIdentifier path;
        auto prop = prepare(from, to, rangeConf, path, true);

        Range range(from, to);

        AutoTransaction guard("Unsetup conf table");
        commandActive = true;

        // unbind any previous binding
        int count = range.rowCount() * range.colCount();
        for (int i = 0; i < count; ++i) {
            auto r = range;
            auto binding = sheet->getCellBinding(r);
            if (!binding) {
                break;
            }
            Gui::cmdAppObjectArgs(sheet,
                                  "setExpression('.cells.%s.%s.%s', None)",
                                  binding == PropertySheet::BindingNormal ? "Bind"
                                                                          : "BindHiddenRef",
                                  r.from().toString(),
                                  r.to().toString());
        }

        Gui::cmdAppObjectArgs(sheet,
                              "clear('%s')",
                              from.toString(CellAddress::Cell::ShowRowColumn));

        if (prop && prop->getName()) {
            auto obj = path.getDocumentObject();
            if (!obj) {
                FC_THROWM(Base::RuntimeError, "Object not found");
            }
            Gui::cmdAppObjectArgs(obj, "setExpression('%s.Enum', None)", prop->getName());
            if (prop->testStatus(Property::PropDynamic)) {
                Gui::cmdAppObjectArgs(obj, "removeProperty('%s')", prop->getName());
            }
        }

        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        Gui::Command::commitCommand();
        QDialog::accept();
    }
    catch (Base::Exception& e) {
        e.ReportException();
        QMessageBox::critical(this, tr("Unsetup configuration table"), QString::fromUtf8(e.what()));
        if (commandActive) {
            Gui::Command::abortCommand();
        }
    }
}

#include "moc_DlgSheetConf.cpp"
