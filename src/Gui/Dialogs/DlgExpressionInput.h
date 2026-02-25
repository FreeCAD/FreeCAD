// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
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
 *   write to the Free Software Foundation, Inc., 51 Franklin Street,      *
 *   Fifth Floor, Boston, MA  02110-1301, USA                              *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <QDialog>
#include <QTreeWidget>
#include <QStandardItemModel>
#include <App/ObjectIdentifier.h>
#include <Base/Type.h>
#include <Base/Unit.h>
#include <memory>

#include "Dialogs/DlgAddProperty.h"

namespace Ui
{
class DlgExpressionInput;
}

namespace Base
{
class Quantity;
}

namespace App
{
class Path;
class Expression;
class DocumentObject;
}  // namespace App

namespace Gui::Dialog
{

class GuiExport NumberRange
{
public:
    void setRange(double minimum, double maximum);
    void clearRange();
    void throwIfOutOfRange(const Base::Quantity&) const;

private:
    double minimum {};
    double maximum {};
    bool defined {false};
};

class GuiExport DlgExpressionInput: public QDialog
{
    Q_OBJECT

public:
    explicit DlgExpressionInput(
        const App::ObjectIdentifier& _path,
        std::shared_ptr<const App::Expression> _expression,
        const Base::Unit& _impliedUnit,
        QWidget* parent = nullptr
    );
    ~DlgExpressionInput() override;

    void setRange(double minimum, double maximum);
    void clearRange();
    std::shared_ptr<App::Expression> getExpression() const
    {
        return expression;
    }

    bool discardedFormula() const
    {
        return discarded;
    }

    QPoint expressionPosition() const;

public Q_SLOTS:
    void show();
    void accept() override;

protected:
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    Base::Type getTypePath();
    Base::Type determineTypeVarSet();
    bool typeOkForVarSet();
    void initializeErrorFrame();
    void initializeVarSets();
    bool checkCyclicDependencyVarSet(const QString& text);
    void checkExpression(const QString& text);
    int getVarSetIndex(const App::Document* doc) const;
    void preselectGroup();
    void preselectVarSet();
    QStandardItemModel* createVarSetModel();
    void setupVarSets();
    std::string getType();
    void reportVarSetInfo(const QString& message);
    bool reportName();
    bool reportGroup(const QString& nameGroup);
    void updateVarSetInfo(bool checkExpr = true);
    void createBindingVarSet(App::Property* propVarSet, App::DocumentObject* varSet);
    void acceptWithVarSet();
    bool isPropertyNameValid(
        const QString& nameProp,
        const App::DocumentObject* obj,
        QString& message
    ) const;
    bool isGroupNameValid(const QString& nameGroup, QString& message) const;
    void setMsgText();

private Q_SLOTS:
    void textChanged();
    void setDiscarded();
    void onCheckVarSets(int state);
    void onVarSetSelected(int index);
    void onTextChangedGroup(const QString&);
    void namePropChanged(const QString&);
    bool needReportOnVarSet();

private:
    ::Ui::DlgExpressionInput* ui;
    std::shared_ptr<App::Expression> expression;
    App::ObjectIdentifier path;
    bool discarded;
    const Base::Unit impliedUnit;
    NumberRange numberRange;

    std::string message;

    bool varSetsVisible;
    QPushButton* okBtn = nullptr;
    QPushButton* discardBtn = nullptr;

    EditFinishedComboBox comboBoxGroup;
};

}  // namespace Gui::Dialog
