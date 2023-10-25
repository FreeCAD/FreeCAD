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
#include <boost/algorithm/string.hpp>

#include <App/Application.h>
#include <App/Document.h>
#include <App/ExpressionParser.h>
#include <App/Range.h>
#include <Gui/CommandT.h>

#include "DlgBindSheet.h"
#include "ui_DlgBindSheet.h"


using namespace App;
using namespace Spreadsheet;
using namespace SpreadsheetGui;

DlgBindSheet::DlgBindSheet(Sheet* sheet, const std::vector<Range>& ranges, QWidget* parent)
    : QDialog(parent)
    , sheet(sheet)
    , range(ranges.front())
    , ui(new Ui::DlgBindSheet)
{
    ui->setupUi(this);
    // remove the automatic help button in dialog title since we don't use it
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    std::string toStart, toEnd;
    ExpressionPtr pStart, pEnd;
    App::ObjectIdentifier bindingTarget;
    PropertySheet::BindingType type = sheet->getCellBinding(range, &pStart, &pEnd, &bindingTarget);
    if (type == PropertySheet::BindingNone) {
        if (ranges.size() > 1) {
            toStart = ranges.back().from().toString();
            toEnd = ranges.back().to().toString();
        }
        else {
            CellAddress target(range.to().row() ? 0 : range.to().row() + 1, range.from().col());
            toStart = target.toString();
            target.setRow(target.row() + range.to().row() - range.from().row());
            target.setCol(target.col() + range.to().col() - range.from().col());
            toEnd = target.toString();
        }
        ui->btnDiscard->setDisabled(true);
    }
    else {
        ui->lineEditFromStart->setReadOnly(true);
        ui->lineEditFromEnd->setReadOnly(true);
        ui->checkBoxHREF->setChecked(type == PropertySheet::BindingHiddenRef);
        assert(pStart && pEnd);
        if (!pStart->hasComponent() && pStart->isDerivedFrom(StringExpression::getClassTypeId())) {
            toStart = static_cast<StringExpression*>(pStart.get())->getText();
        }
        else {
            toStart = "=";
            toStart += pStart->toString();
        }
        if (!pEnd->hasComponent() && pEnd->isDerivedFrom(StringExpression::getClassTypeId())) {
            toEnd = static_cast<StringExpression*>(pEnd.get())->getText();
        }
        else {
            toEnd = "=";
            toEnd += pEnd->toString();
        }
    }

    ui->lineEditFromStart->setText(QString::fromLatin1(range.from().toString().c_str()));
    ui->lineEditFromEnd->setText(QString::fromLatin1(range.to().toString().c_str()));

    ui->lineEditToStart->setDocumentObject(sheet, false);
    ui->lineEditToStart->setPrefix('=');
    ui->lineEditToEnd->setDocumentObject(sheet, false);
    ui->lineEditToEnd->setPrefix('=');

    ui->lineEditToStart->setText(QLatin1String(toStart.c_str()));
    ui->lineEditToEnd->setText(QLatin1String(toEnd.c_str()));

    ui->comboBox->addItem(
        QString::fromLatin1(". (%1)").arg(QString::fromUtf8(sheet->Label.getValue())),
        QByteArray(""));

    App::DocumentObject* target = bindingTarget.getDocumentObject();
    for (auto obj : sheet->getDocument()->getObjectsOfType<Sheet>()) {
        if (obj == sheet) {
            continue;
        }
        QString label;
        if (obj->Label.getStrValue() != obj->getNameInDocument()) {
            label =
                QString::fromLatin1("%1 (%2)").arg(QString::fromLatin1(obj->getNameInDocument()),
                                                   QString::fromUtf8(obj->Label.getValue()));
        }
        else {
            label = QLatin1String(obj->getNameInDocument());
        }
        ui->comboBox->addItem(label, QByteArray(obj->getNameInDocument()));
        if (obj == target) {
            ui->comboBox->setCurrentIndex(ui->comboBox->count() - 1);
        }
    }
    for (auto doc : GetApplication().getDocuments()) {
        if (doc == sheet->getDocument()) {
            continue;
        }
        for (auto obj : sheet->getDocument()->getObjectsOfType<Sheet>()) {
            if (obj == sheet) {
                continue;
            }
            std::string fullname = obj->getFullName();
            QString label;
            if (obj->Label.getStrValue() != obj->getNameInDocument()) {
                label =
                    QString::fromLatin1("%1 (%2)").arg(QString::fromLatin1(fullname.c_str()),
                                                       QString::fromUtf8(obj->Label.getValue()));
            }
            else {
                label = QLatin1String(fullname.c_str());
            }
            ui->comboBox->addItem(label, QByteArray(fullname.c_str()));
            if (obj == target) {
                ui->comboBox->setCurrentIndex(ui->comboBox->count() - 1);
            }
        }
    }

    connect(ui->btnDiscard, &QPushButton::clicked, this, &DlgBindSheet::onDiscard);
}

DlgBindSheet::~DlgBindSheet()
{
    delete ui;
}

void DlgBindSheet::accept()
{
    bool commandActive = false;
    try {
        const char* ref = ui->comboBox->itemData(ui->comboBox->currentIndex())
                              .toByteArray()
                              .constData();  // clazy:exclude=returning-data-from-temporary
        auto obj = sheet;
        if (ref[0]) {
            const char* sep = strchr(ref, '#');
            if (sep) {
                std::string docname(ref, sep);
                auto doc = GetApplication().getDocument(docname.c_str());
                if (!doc) {
                    FC_THROWM(Base::RuntimeError, "Cannot find document " << docname);
                }
                obj = Base::freecad_dynamic_cast<Sheet>(doc->getObject(sep + 1));
            }
            else {
                obj = Base::freecad_dynamic_cast<Sheet>(sheet->getDocument()->getObject(ref));
            }
            if (!obj) {
                FC_THROWM(Base::RuntimeError, "Cannot find Spreadsheet '" << ref << "'");
            }
        }

        auto checkAddress = [](std::string& addr, CellAddress& cell, bool quote) {
            std::string copy(addr);
            boost::to_upper(copy);
            cell = App::stringToAddress(copy.c_str(), true);
            if (!cell.isValid()) {
                std::string msg("Invalid cell: ");
                msg += addr;
                throw Base::ValueError(msg.c_str());
            }
            if (quote) {
                addr = std::string("<<") + copy + ">>";
            }
            else {
                addr = copy;
            }
        };

        CellAddress fromCellStart, fromCellEnd, toCellStart, toCellEnd;
        std::string fromStart(ui->lineEditFromStart->text().trimmed().toLatin1().constData());
        std::string fromEnd(ui->lineEditFromEnd->text().trimmed().toLatin1().constData());
        checkAddress(fromStart, fromCellStart, false);
        checkAddress(fromEnd, fromCellEnd, false);

        std::string toStart(ui->lineEditToStart->text().trimmed().toLatin1().constData());
        if (boost::starts_with(toStart, "=")) {
            toStart.erase(toStart.begin());
        }
        else {
            checkAddress(toStart, toCellStart, true);
        }

        std::string toEnd(ui->lineEditToEnd->text().trimmed().toLatin1().constData());
        if (boost::starts_with(toEnd, "=")) {
            toEnd.erase(toEnd.begin());
        }
        else {
            checkAddress(toEnd, toCellEnd, true);
            if (toCellStart.isValid()) {
                App::Range fromRange(fromCellStart, fromCellEnd, true);
                App::Range toRange(toCellStart, toCellEnd, true);
                if (fromRange.size() != toRange.size()) {
                    auto res = QMessageBox::warning(this,
                                                    tr("Bind cells"),
                                                    tr("Source and target cell count mismatch. "
                                                       "Partial binding may still work.\n\n"
                                                       "Do you want to continue?"),
                                                    QMessageBox::Yes | QMessageBox::No);
                    if (res == QMessageBox::No) {
                        return;
                    }
                }
            }
        }

        Gui::Command::openCommand("Bind cells");
        commandActive = true;

        if (ui->checkBoxHREF->isChecked()) {
            Gui::cmdAppObjectArgs(sheet,
                                  "setExpression('.cells.Bind.%s.%s', None)",
                                  fromStart,
                                  fromEnd);
            Gui::cmdAppObjectArgs(
                sheet,
                "setExpression('.cells.BindHiddenRef.%s.%s', 'hiddenref(tuple(%s.cells, %s, %s))')",
                fromStart,
                fromEnd,
                ref,
                toStart,
                toEnd);
        }
        else {
            Gui::cmdAppObjectArgs(sheet,
                                  "setExpression('.cells.BindHiddenRef.%s.%s', None)",
                                  fromStart,
                                  fromEnd);
            Gui::cmdAppObjectArgs(sheet,
                                  "setExpression('.cells.Bind.%s.%s', 'tuple(%s.cells, %s, %s)')",
                                  fromStart,
                                  fromEnd,
                                  ref,
                                  toStart,
                                  toEnd);
        }
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        Gui::Command::commitCommand();
        QDialog::accept();
    }
    catch (Base::Exception& e) {
        e.ReportException();
        QMessageBox::critical(this,
                              tr("Bind Spreadsheet Cells"),
                              tr("Error:\n") + QString::fromUtf8(e.what()));
        if (commandActive) {
            Gui::Command::abortCommand();
        }
    }
}

void DlgBindSheet::onDiscard()
{
    try {
        std::string fromStart(ui->lineEditFromStart->text().trimmed().toLatin1().constData());
        std::string fromEnd(ui->lineEditFromEnd->text().trimmed().toLatin1().constData());
        Gui::Command::openCommand("Unbind cells");
        Gui::cmdAppObjectArgs(sheet,
                              "setExpression('.cells.Bind.%s.%s', None)",
                              fromStart,
                              fromEnd);
        Gui::cmdAppObjectArgs(sheet,
                              "setExpression('.cells.BindHiddenRef.%s.%s', None)",
                              fromStart,
                              fromEnd);
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        Gui::Command::commitCommand();
        reject();
    }
    catch (Base::Exception& e) {
        e.ReportException();
        QMessageBox::critical(this, tr("Unbind cells"), QString::fromUtf8(e.what()));
        Gui::Command::abortCommand();
    }
}

#include "moc_DlgBindSheet.cpp"
