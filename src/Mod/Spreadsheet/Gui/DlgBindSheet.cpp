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
#include <boost/algorithm/string/predicate.hpp>
#include <QMessageBox>
#include "DlgBindSheet.h"
#include <Base/Tools.h>
#include <App/Range.h>
#include <App/Document.h>
#include <App/Application.h>
#include <App/ExpressionParser.h>
#include <Gui/CommandT.h>
#include "ui_DlgBindSheet.h"

using namespace App;
using namespace Spreadsheet;
using namespace SpreadsheetGui;

DlgBindSheet::DlgBindSheet(Sheet *sheet, const std::vector<Range> &ranges, QWidget *parent)
    : QDialog(parent), sheet(sheet), range(ranges.front()), ui(new Ui::DlgBindSheet)
{
    ui->setupUi(this);
    // remove the automatic help button in dialog title since we don't use it
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    std::string toStart,toEnd;
    ExpressionPtr pStart, pEnd;
    PropertySheet::BindingType type = sheet->getCellBinding(range,&pStart,&pEnd);
    if(type == PropertySheet::BindingNone) {
        if(ranges.size()>1) {
            toStart = ranges.back().from().toString();
            toEnd = ranges.back().to().toString();
        } else {
            CellAddress target(range.to().row()?0:range.to().row()+1,range.from().col());
            toStart = target.toString();
            target.setRow(target.row() + range.to().row() - range.from().row());
            target.setCol(target.col() + range.to().col() - range.from().col());
            toEnd = target.toString();
        }
    } else {
        ui->lineEditFromStart->setReadOnly(true);
        ui->lineEditFromEnd->setReadOnly(true);
        ui->checkBoxHREF->setChecked(type==PropertySheet::BindingHiddenRef);
        assert(pStart && pEnd);
        if(!pStart->hasComponent() && pStart->isDerivedFrom(StringExpression::getClassTypeId()))
            toStart = static_cast<StringExpression*>(pStart.get())->getText();
        else {
            toStart = "=";
            toStart += pStart->toString();
        }
        if(!pEnd->hasComponent() && pEnd->isDerivedFrom(StringExpression::getClassTypeId()))
            toEnd = static_cast<StringExpression*>(pEnd.get())->getText();
        else {
            toEnd = "=";
            toEnd += pEnd->toString();
        }
    }

    ui->lineEditFromStart->setText(QString::fromLatin1(range.from().toString().c_str()));
    ui->lineEditFromEnd->setText(QString::fromLatin1(range.to().toString().c_str()));

    ui->lineEditToStart->setDocumentObject(sheet,false);
    ui->lineEditToStart->setPrefix('=');
    ui->lineEditToEnd->setDocumentObject(sheet,false);
    ui->lineEditToEnd->setPrefix('=');

    ui->lineEditToStart->setText(QLatin1String(toStart.c_str()));
    ui->lineEditToEnd->setText(QLatin1String(toEnd.c_str()));

    ui->comboBox->addItem(QString::fromLatin1(". (%1)").arg(
                QString::fromUtf8(sheet->Label.getValue())), QByteArray(""));

    for(auto obj : sheet->getDocument()->getObjectsOfType<Sheet>()) {
        if(obj == sheet)
            continue;
        QString label;
        if(obj->Label.getStrValue() != obj->getNameInDocument())
            label = QString::fromLatin1("%1 (%2)").arg(
                                QString::fromLatin1(obj->getNameInDocument()),
                                QString::fromUtf8(obj->Label.getValue()));
        else
            label = QLatin1String(obj->getNameInDocument());
        ui->comboBox->addItem(label, QByteArray(obj->getNameInDocument()));
    }
    for(auto doc : GetApplication().getDocuments()) {
        if(doc == sheet->getDocument())
            continue;
        for(auto obj : sheet->getDocument()->getObjectsOfType<Sheet>()) {
            if(obj == sheet)
                continue;
            std::string fullname = obj->getFullName();
            QString label;
            if(obj->Label.getStrValue() != obj->getNameInDocument())
                label = QString::fromLatin1("%1 (%2)").arg(
                                    QString::fromLatin1(fullname.c_str()),
                                    QString::fromUtf8(obj->Label.getValue()));
            else
                label = QLatin1String(fullname.c_str());
            ui->comboBox->addItem(label, QByteArray(fullname.c_str()));
        }
    }

    connect(ui->btnDiscard, SIGNAL(clicked()), this, SLOT(onDiscard()));
}

DlgBindSheet::~DlgBindSheet()
{
    delete ui;
}

void DlgBindSheet::accept()
{
    bool commandActive = false;
    try {
        const char *ref = ui->comboBox->itemData(ui->comboBox->currentIndex()).toByteArray().constData();
        auto obj = sheet;
        if(ref[0]) {
            const char *sep = strchr(ref,'#');
            if(sep) {
                std::string docname(ref,sep);
                auto doc = GetApplication().getDocument(docname.c_str());
                if(!doc)
                    FC_THROWM(Base::RuntimeError, "Cannot find document " << docname);
                obj = Base::freecad_dynamic_cast<Sheet>(doc->getObject(sep+1));
            } else 
                obj = Base::freecad_dynamic_cast<Sheet>(sheet->getDocument()->getObject(ref));
            if(!obj)
                FC_THROWM(Base::RuntimeError, "Cannot find Spreadsheet '" << ref << "'");
        }

        std::string fromStart(ui->lineEditFromStart->text().trimmed().toLatin1().constData());
        std::string fromEnd(ui->lineEditFromEnd->text().trimmed().toLatin1().constData());

        std::string toStart(ui->lineEditToStart->text().trimmed().toLatin1().constData());
        if(boost::starts_with(toStart,"=")) 
            toStart.erase(toStart.begin());
        else
            toStart = std::string("<<") + toStart + ">>";

        std::string toEnd(ui->lineEditToEnd->text().trimmed().toLatin1().constData());
        if(boost::starts_with(toEnd,"=")) 
            toEnd.erase(toEnd.begin());
        else
            toEnd = std::string("<<") + toEnd + ">>";

        Gui::Command::openCommand("Bind cells");
        commandActive = true;

        if(ui->checkBoxHREF->isChecked())
            Gui::cmdAppObjectArgs(sheet,
                    "setExpression('.cells.BindHiddenRef.%s.%s', 'hiddenref(tuple(%s.cells, %s, %s))')",
                    fromStart, fromEnd, ref, toStart, toEnd);
        else
            Gui::cmdAppObjectArgs(sheet,
                    "setExpression('.cells.Bind.%s.%s', 'tuple(%s.cells, %s, %s)')",
                    fromStart, fromEnd, ref, toStart, toEnd);
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        Gui::Command::commitCommand();
        QDialog::accept();
    } catch(Base::Exception &e) {
        e.ReportException();
        QMessageBox::critical(this, tr("Bind cells"), QString::fromUtf8(e.what()));
        if(commandActive)
            Gui::Command::abortCommand();
    }
}

void DlgBindSheet::onDiscard() {
    try {
        std::string fromStart(ui->lineEditFromStart->text().trimmed().toLatin1().constData());
        std::string fromEnd(ui->lineEditFromEnd->text().trimmed().toLatin1().constData());
        Gui::Command::openCommand("Unbind cells");
        Gui::cmdAppObjectArgs(sheet, "setExpression('.cells.Bind.%s.%s', None)", fromStart, fromEnd);
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        Gui::Command::commitCommand();
        reject();
    } catch(Base::Exception &e) {
        e.ReportException();
        QMessageBox::critical(this, tr("Unbind cells"), QString::fromUtf8(e.what()));
        Gui::Command::abortCommand();
    }
}

#include "moc_DlgBindSheet.cpp"
