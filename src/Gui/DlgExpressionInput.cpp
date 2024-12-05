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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QTreeWidget>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/ExpressionParser.h>
#include <App/VarSet.h>
#include <Base/Tools.h>

#include "DlgExpressionInput.h"
#include "ui_DlgExpressionInput.h"
#include "Application.h"
#include "Command.h"
#include "Tools.h"
#include "ExpressionBinding.h"
#include "BitmapFactory.h"
#include "ViewProviderDocumentObject.h"

using namespace App;
using namespace Gui::Dialog;

bool DlgExpressionInput::varSetsVisible = false;

DlgExpressionInput::DlgExpressionInput(const App::ObjectIdentifier & _path,
                                       std::shared_ptr<const Expression> _expression,
                                       const Base::Unit & _impliedUnit, QWidget *parent)
  : QDialog(parent)
  , ui(new Ui::DlgExpressionInput)
  , expression(_expression ? _expression->copy() : nullptr)
  , path(_path)
  , discarded(false)
  , impliedUnit(_impliedUnit)
  , minimumWidth(10)
{
    assert(path.getDocumentObject());

    // Setup UI
    ui->setupUi(this);

    initializeVarSets();

    // Connect signal(s)
    connect(ui->expression, &ExpressionLineEdit::textChanged,
        this, &DlgExpressionInput::textChanged);
    connect(ui->discardBtn, &QPushButton::clicked,
        this, &DlgExpressionInput::setDiscarded);

    if (expression) {
        ui->expression->setText(QString::fromStdString(expression->toString()));
    }
    else {
        QVariant text = parent->property("text");
        if (text.canConvert<QString>()) {
            ui->expression->setText(text.toString());
        }
    }

    // Set document object on line edit to create auto completer
    DocumentObject * docObj = path.getDocumentObject();
    ui->expression->setDocumentObject(docObj);

    // There are some platforms where setting no system background causes a black
    // rectangle to appear. To avoid this the 'NoSystemBackground' parameter can be
    // set to false. Then a normal non-modal dialog will be shown instead (#0002440).
    bool noBackground = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Expression")->GetBool("NoSystemBackground", false);

    if (noBackground) {
#if defined(Q_OS_MACOS)
        setWindowFlags(Qt::Widget | Qt::Popup | Qt::FramelessWindowHint);
#else
        setWindowFlags(Qt::SubWindow | Qt::Widget | Qt::Popup | Qt::FramelessWindowHint);
#endif
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
    }
    else {
        ui->expression->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        ui->horizontalSpacer_3->changeSize(0, 2);
        ui->verticalLayout->setContentsMargins(9, 9, 9, 9);
        this->adjustSize();
        // It is strange that (at least on Linux) DlgExpressionInput will shrink
        // to be narrower than ui->expression after calling adjustSize() above.
        // Why?
        if(this->width() < ui->expression->width() + 18)
            this->resize(ui->expression->width()+18,this->height());
    }
    ui->expression->setFocus();
}

DlgExpressionInput::~DlgExpressionInput()
{
    disconnect(ui->checkBoxVarSets, &QCheckBox::stateChanged,
               this, &DlgExpressionInput::onCheckVarSets);
    disconnect(ui->comboBoxVarSet, qOverload<int>(&QComboBox::currentIndexChanged),
               this, &DlgExpressionInput::onVarSetSelected);
    disconnect(ui->lineEditGroup, &QLineEdit::textChanged,
               this, &DlgExpressionInput::onTextChangedGroup);
    disconnect(ui->lineEditPropNew, &QLineEdit::textChanged,
               this, &DlgExpressionInput::namePropChanged);

    delete ui;
}

static void getVarSetsDocument(std::vector<App::VarSet*>& varSets, App::Document* doc) {
    for (auto obj : doc->getObjects()) {
        auto varSet = dynamic_cast<App::VarSet*>(obj);
        if (varSet) {
            varSets.push_back(varSet);
        }
    }
}

static std::vector<App::VarSet*> getAllVarSets()
{
    std::vector<App::Document*> docs = App::GetApplication().getDocuments();
    std::vector<App::VarSet*> varSets;

    for (auto doc : docs) {
        getVarSetsDocument(varSets, doc);
    }

    return varSets;
}

Base::Type DlgExpressionInput::getTypePath()
{
    return path.getProperty()->getTypeId();
}

Base::Type DlgExpressionInput::determineTypeVarSet()
{
    Base::Type typePath = getTypePath();

    // The type of the path is leading.  If it is one of the types below, we
    // can create a property in the varset.
    if (typePath == App::PropertyString::getClassTypeId() ||
        typePath.isDerivedFrom(App::PropertyFloat::getClassTypeId()) ||
        typePath.isDerivedFrom(App::PropertyInteger::getClassTypeId())) {
        return typePath;
    }

    // If we cannot determine the type by means of the path, for example when
    // dealing with a sketcher constraint list or with the x, y, or z of a
    // Placement, the type of the unit allows us to create a property in the
    // varset.  Since unit properties are derived from App::PropertyFloat, it
    // allows us to create a property and set the value.

    std::string unitTypeString = impliedUnit.getTypeString().toStdString();
    if (unitTypeString.empty()) {
        // no type was provided
        return Base::Type::badType();
    }

    std::string typeString = "App::Property" + unitTypeString;
    // may return badType
    return Base::Type::fromName(typeString.c_str());
}

bool DlgExpressionInput::typeOkForVarSet()
{
    std::string unitType = impliedUnit.getTypeString().toStdString();
    return determineTypeVarSet() != Base::Type::badType();
}

void DlgExpressionInput::initializeVarSets()
{
    ui->labelInfoActive->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    ui->labelInfoActive->setWordWrap(true);

    connect(ui->checkBoxVarSets, &QCheckBox::stateChanged,
            this, &DlgExpressionInput::onCheckVarSets);
    connect(ui->comboBoxVarSet, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &DlgExpressionInput::onVarSetSelected);
    connect(ui->lineEditGroup, &QLineEdit::textChanged,
            this, &DlgExpressionInput::onTextChangedGroup);
    connect(ui->lineEditPropNew, &QLineEdit::textChanged,
            this, &DlgExpressionInput::namePropChanged);

    std::vector<App::VarSet*> varSets = getAllVarSets();
    if (!varSets.empty() && typeOkForVarSet()) {
        ui->checkBoxVarSets->setVisible(true);
        ui->checkBoxVarSets->setCheckState(varSetsVisible ? Qt::Checked : Qt::Unchecked);
        ui->groupBoxVarSets->setVisible(varSetsVisible);
        if (varSetsVisible) {
            setupVarSets();
        }
    }
    else {
        // The dialog is shown without any VarSet options.
        varSetsVisible = false;
        ui->checkBoxVarSets->setVisible(false);
        ui->groupBoxVarSets->setVisible(false);
    }
}

void NumberRange::setRange(double min, double max)
{
    minimum = min;
    maximum = max;
    defined = true;
}

void NumberRange::clearRange()
{
    defined = false;
}

void NumberRange::throwIfOutOfRange(const Base::Quantity& value) const
{
    if (!defined)
        return;

    if (value.getValue() < minimum || value.getValue() > maximum) {
        Base::Quantity minVal(minimum, value.getUnit());
        Base::Quantity maxVal(maximum, value.getUnit());
        QString valStr = value.getUserString();
        QString minStr = minVal.getUserString();
        QString maxStr = maxVal.getUserString();
        QString error = QString::fromLatin1("Value out of range (%1 out of [%2, %3])").arg(valStr, minStr, maxStr);

        throw Base::ValueError(error.toStdString());
    }
}

void DlgExpressionInput::setRange(double minimum, double maximum)
{
    numberRange.setRange(minimum, maximum);
}

void DlgExpressionInput::clearRange()
{
    numberRange.clearRange();
}

QPoint DlgExpressionInput::expressionPosition() const
{
    return ui->expression->pos();
}

void DlgExpressionInput::checkExpression(const QString& text)
{
        //now handle expression
        std::shared_ptr<Expression> expr(ExpressionParser::parse(path.getDocumentObject(), text.toUtf8().constData()));

        if (expr) {
            std::string error = path.getDocumentObject()->ExpressionEngine.validateExpression(path, expr);

            if (!error.empty())
                throw Base::RuntimeError(error.c_str());

            std::unique_ptr<Expression> result(expr->eval());

            expression = expr;
            ui->okBtn->setEnabled(true);
            ui->msg->clear();

            //set default palette as we may have read text right now
            ui->msg->setPalette(ui->okBtn->palette());

            auto * n = Base::freecad_dynamic_cast<NumberExpression>(result.get());
            if (n) {
                Base::Quantity value = n->getQuantity();
                QString msg = value.getUserString();

                if (!value.isValid()) {
                    throw Base::ValueError("Not a number");
                }
                else if (!impliedUnit.isEmpty()) {
                    if (!value.getUnit().isEmpty() && value.getUnit() != impliedUnit)
                        throw Base::UnitsMismatchError("Unit mismatch between result and required unit");

                    value.setUnit(impliedUnit);

                }
                else if (!value.getUnit().isEmpty()) {
                    msg += QString::fromUtf8(" (Warning: unit discarded)");

                    QPalette p(ui->msg->palette());
                    p.setColor(QPalette::WindowText, Qt::red);
                    ui->msg->setPalette(p);
                }

                numberRange.throwIfOutOfRange(value);

                ui->msg->setText(msg);
            }
            else {
                ui->msg->setText(QString::fromStdString(result->toString()));
            }

        }
}

static const bool NO_CHECK_EXPR = false;

void DlgExpressionInput::textChanged(const QString &text)
{
    if (text.isEmpty()) {
        ui->okBtn->setDisabled(true);
        ui->discardBtn->setDefault(true);
        return;
    }

    ui->okBtn->setDefault(true);

    try {
        //resize the input field according to text size
        QFontMetrics fm(ui->expression->font());
        int width = QtTools::horizontalAdvance(fm, text) + 15;
        if (width < minimumWidth)
            ui->expression->setMinimumWidth(minimumWidth);
        else
            ui->expression->setMinimumWidth(width);

        if(this->width() < ui->expression->minimumWidth())
            setMinimumWidth(ui->expression->minimumWidth());

        checkExpression(text);
        if (varSetsVisible) {
            // If varsets are visible, check whether the varset info also
            // agrees that the button should be enabled.
            // No need to check the expression in that function.
            updateVarSetInfo(NO_CHECK_EXPR);
        }
    }
    catch (Base::Exception & e) {
        ui->msg->setText(QString::fromUtf8(e.what()));
        QPalette p(ui->msg->palette());
        p.setColor(QPalette::WindowText, Qt::red);
        ui->msg->setPalette(p);
        ui->okBtn->setDisabled(true);
    }
}

void DlgExpressionInput::setDiscarded()
{
    discarded = true;
    reject();
}

void DlgExpressionInput::setExpressionInputSize(int width, int height)
{
    if (ui->expression->minimumHeight() < height)
        ui->expression->setMinimumHeight(height);

    if (ui->expression->minimumWidth() < width)
        ui->expression->setMinimumWidth(width);

    minimumWidth = width;
}

void DlgExpressionInput::mouseReleaseEvent(QMouseEvent* ev)
{
    Q_UNUSED(ev);
}

void DlgExpressionInput::mousePressEvent(QMouseEvent* ev)
{
    Q_UNUSED(ev);

    // The 'FramelessWindowHint' is also set when the background is transparent.
    if (windowFlags() & Qt::FramelessWindowHint) {
        //we need to reject the dialog when clicked on the background. As the background is transparent
        //this is the expected behaviour for the user
        bool on = ui->expression->completerActive();
        if (!on)
            this->reject();
    }
}

void DlgExpressionInput::show()
{
    QDialog::show();
    this->activateWindow();
    ui->expression->selectAll();
}

class Binding : public Gui::ExpressionBinding
{
    // helper class to compensate for the fact that
    // ExpressionBinding::setExpression is protected.
public:
    Binding() = default;

    void setExpression(std::shared_ptr<App::Expression> expr) override
    {
        ExpressionBinding::setExpression(expr);
    }
};

static bool isNamePropOk(const QString& nameProp, App::DocumentObject* obj,
                         std::stringstream& message)
{
    if (!obj) {
        message << "Unknown object";
        return false;
    }

    std::string name = nameProp.toStdString();
    if (name.empty()) {
        message << "Please provide a name for the property.";
        return false;
    }

    if (name != Base::Tools::getIdentifier(name)) {
        message << "Invalid property name (must only contain alphanumericals, underscore, "
                << "and must not start with digit";
        return false;
    }

    if (ExpressionParser::isTokenAUnit(name) || ExpressionParser::isTokenAConstant(name)) {
        message << name << " is a reserved word";
        return false;
    }

    auto prop = obj->getPropertyByName(name.c_str());
    if (prop && prop->getContainer() == obj) {
        message << name << " already exists";
        return false;
    }

    return true;
}

static const int ROLE_DOC = Qt::UserRole;
static const int ROLE_VARSET_NAME = Qt::UserRole + 1;
static const int ROLE_VARSET_LABEL = Qt::UserRole + 2;
static const int ROLE_GROUP = Qt::UserRole + 3;

static QString getValue(QTreeWidgetItem* item, int role)
{
    QVariant variant = item->data(0, role);
    return variant.toString();
}

void DlgExpressionInput::acceptWithVarSet()
{
    // all checks have been performed in updateVarSetInfo and textChanged that
    // decide to enable the button

    // create a property in the VarSet
    QTreeWidgetItem *selected = treeWidget->currentItem();
    QString nameVarSet = getValue(selected, ROLE_VARSET_NAME);
    QString nameGroup = ui->lineEditGroup->text();
    QString nameProp = ui->lineEditPropNew->text();

    QString nameDoc = getValue(selected, ROLE_DOC);
    App::Document* doc = App::GetApplication().getDocument(nameDoc.toUtf8());
    App::DocumentObject* obj = doc->getObject(nameVarSet.toUtf8());

    std::string name = nameProp.toStdString();
    std::string group = nameGroup.toStdString();
    std::string type = getType();
    auto prop = obj->addDynamicProperty(type.c_str(), name.c_str(), group.c_str());

    // Set the value of the property in the VarSet
    //
    // The value of the property is going to be the value that was originally
    // meant to be the value for the property that this dialog is targeting.
    Expression* exprSimplfied = expression->simplify();
    auto ne = dynamic_cast<NumberExpression*>(exprSimplfied);
    auto se = dynamic_cast<StringExpression*>(exprSimplfied);
    if (ne) {
        // the value is a number: directly assign it to the property instead of
        // making it an expression in the variable set
        Gui::Command::doCommand(Gui::Command::Doc, "App.getDocument('%s').getObject('%s').%s = %f",
                                obj->getDocument()->getName(),
                                obj->getNameInDocument(),
                                prop->getName(), ne->getValue());
    }
    else if (se) {
        // the value is a string: directly assign it to the property.
        Gui::Command::doCommand(Gui::Command::Doc, "App.getDocument('%s').getObject('%s').%s = \"%s\"",
                                obj->getDocument()->getName(),
                                obj->getNameInDocument(),
                                prop->getName(), se->getText().c_str());
    }
    else {
        // the value is an epxression: make an expression binding in the variable set.
        ObjectIdentifier objId(*prop);
        Binding binding;
        binding.bind(objId);
        binding.setExpression(expression);
        binding.apply();
    }

    // Create a new expression that refers to the property in the variable set
    // for the original property that is the target of this dialog.
    expression.reset(ExpressionParser::parse(path.getDocumentObject(),
                                             prop->getFullName().c_str()));
}

void DlgExpressionInput::accept() {
    if (varSetsVisible) {
        acceptWithVarSet();
    }
    QDialog::accept();
}

static void addGroupsVarSetComboBox(App::VarSet* varSet, QTreeWidgetItem* varSetItem)
{
    std::vector<Property*> properties;
    std::set<std::string> namesGroup;
    varSet->getPropertyList(properties);
    for (auto prop : properties) {
        const char* nameGroup = prop->getGroup();
        if (!nameGroup || strcmp(nameGroup, "") == 0) {
            namesGroup.insert("Base");
        }
        else {
            namesGroup.insert(nameGroup);
        }
    }
    for (const auto& nameGroup : namesGroup) {
        // the item will be automatically destroyed when the varSetItem will be destroyed
        auto item = new QTreeWidgetItem(varSetItem);
        item->setText(0, QString::fromStdString(nameGroup));
        item->setData(0, ROLE_GROUP, QString::fromStdString(nameGroup));
        item->setData(0, ROLE_VARSET_NAME, varSetItem->data(0, ROLE_VARSET_NAME));
        item->setData(0, ROLE_VARSET_LABEL, varSetItem->data(0, ROLE_VARSET_LABEL));
        item->setData(0, ROLE_DOC, varSetItem->data(0, ROLE_DOC));
    }
}

static void addVarSetsVarSetComboBox(std::vector<App::VarSet*>& varSets, QTreeWidgetItem* docItem)
{
    for (auto varSet : varSets) {
        auto vp = Base::freecad_dynamic_cast<Gui::ViewProviderDocumentObject>(
                Gui::Application::Instance->getViewProvider(varSet));
        // the item will be automatically destroyed when the docItem will be destroyed
        auto item = new QTreeWidgetItem(docItem);
        item->setIcon(0, vp->getIcon());
        item->setText(0, QString::fromUtf8(varSet->Label.getValue()));
        item->setData(0, ROLE_VARSET_LABEL, QString::fromUtf8(varSet->Label.getValue()));
        item->setData(0, ROLE_VARSET_NAME, QString::fromUtf8(varSet->getNameInDocument()));
        item->setData(0, ROLE_DOC, docItem->data(0, ROLE_DOC));
        addGroupsVarSetComboBox(varSet, item);
    }
}

static void addDocVarSetComboBox(App::Document* doc, QPixmap& docIcon, QTreeWidgetItem* rootItem)
{
    if (!doc->testStatus(App::Document::TempDoc)) {
        std::vector<App::VarSet*> varSets;
        getVarSetsDocument(varSets, doc);
        if (!varSets.empty()) {
            // the item will be automatically destroyed when the rootItem will be destroyed
            auto item = new QTreeWidgetItem(rootItem);
            item->setIcon(0, docIcon);
            item->setText(0, QString::fromUtf8(doc->Label.getValue()));
            item->setData(0, ROLE_DOC, QByteArray(doc->getName()));
            item->setFlags(Qt::ItemIsEnabled);
            item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
            addVarSetsVarSetComboBox(varSets, item);
        }
    }
}

static QTreeWidget* createVarSetTreeWidget()
{
    // the caller of the function is responsible of managing the tree widget
    auto treeWidget = new QTreeWidget();
    treeWidget->setColumnCount(1);
    treeWidget->setHeaderHidden(true);
    // the rootItem will be destroyed when the treeWidget will be destroyed
    QTreeWidgetItem *rootItem = treeWidget->invisibleRootItem();

    QPixmap docIcon(Gui::BitmapFactory().pixmap("Document"));
    std::vector<App::Document*> docs = App::GetApplication().getDocuments();

    for (auto doc : docs) {
        addDocVarSetComboBox(doc, docIcon, rootItem);
    }
    treeWidget->expandAll();

    return treeWidget;
}

void DlgExpressionInput::setupVarSets()
{
    ui->comboBoxVarSet->clear();
    // createVarSetTreeWidget returns a dynamically allocated tree widget
    // the memory is managed by means of the unique pointer treeWidget.
    treeWidget.reset(createVarSetTreeWidget());
    ui->comboBoxVarSet->setModel(treeWidget->model());
    ui->comboBoxVarSet->setView(treeWidget.get());

    ui->okBtn->setEnabled(false);
}

std::string DlgExpressionInput::getType()
{
    return determineTypeVarSet().getName();
}

void DlgExpressionInput::onCheckVarSets(int state) {
    varSetsVisible = state == Qt::Checked;
    ui->groupBoxVarSets->setVisible(varSetsVisible);
    if (varSetsVisible) {
        setupVarSets();
    }
    else {
        ui->okBtn->setEnabled(true); // normal expression
    }
}

void DlgExpressionInput::onVarSetSelected(int)
{
    QTreeWidgetItem* selected = treeWidget->currentItem();

    if (selected) {
        // if group is known, fill it in
        QVariant variantGroup = selected->data(0, ROLE_GROUP);
        if (variantGroup.isValid()) {
            ui->lineEditGroup->setText(variantGroup.toString());
        }
        else {
            ui->lineEditGroup->clear();
        }
    }

    updateVarSetInfo();
}

void DlgExpressionInput::onTextChangedGroup(const QString&)
{
    updateVarSetInfo();
}

void DlgExpressionInput::namePropChanged(const QString&)
{
    updateVarSetInfo();
}

static bool isNameGroupOk(const QString& nameGroup,
                          std::stringstream& message)
{
    std::string name = nameGroup.toStdString();
    if (name.empty() || name != Base::Tools::getIdentifier(name)) {
        message << "Invalid group name (must only contain alphanumericals, underscore, "
                << "and must not start with digit";
        return false;
    }

    return true;
}

void DlgExpressionInput::reportVarSetInfo(const char* message)
{
    ui->labelInfoActive->setText(QString::fromUtf8(message));
}

bool DlgExpressionInput::reportGroup(QString& nameGroup)
{
    if (nameGroup.isEmpty()) {
        reportVarSetInfo("Please provide a group.");
        return true;
    }

    std::stringstream message;
    if (!isNameGroupOk(nameGroup, message)) {
        reportVarSetInfo(message.str().c_str());
        return true;
    }

    return false;
}

bool DlgExpressionInput::reportName(QTreeWidgetItem* item)
{
    QString nameProp = ui->lineEditPropNew->text();
    QString nameVarSet = getValue(item, ROLE_VARSET_NAME);
    QString nameDoc = getValue(item, ROLE_DOC);
    App::Document* doc = App::GetApplication().getDocument(nameDoc.toUtf8());
    App::DocumentObject* obj = doc->getObject(nameVarSet.toUtf8());
    std::stringstream message;
    if (!isNamePropOk(nameProp, obj, message)) {
        reportVarSetInfo(message.str().c_str());
        return true;
    }

    return false;
}

void DlgExpressionInput::updateVarSetInfo(bool checkExpr)
{
    QTreeWidgetItem* selected = treeWidget->currentItem();

    if (selected) {
        QString nameGroup = ui->lineEditGroup->text();
        if (reportGroup(nameGroup)) {
            // needed to report something about the group, so disable the button
            ui->okBtn->setEnabled(false);
            return;
        }

        if (reportName(selected)) {
            // needed to report something about the name, so disable the button
            ui->okBtn->setEnabled(false);
            return;
        }

        QString nameProp = ui->lineEditPropNew->text();
        QString labelVarSet = getValue(selected, ROLE_VARSET_LABEL);
        QString nameDoc = getValue(selected, ROLE_DOC);
        std::stringstream message;
        message << "Adding property " << nameProp.toStdString() << std::endl
                << "of type " << getType() << std::endl
                << "to variable set " << labelVarSet.toStdString() << std::endl
                << "in group " << nameGroup.toStdString() << std::endl
                << "in document " << nameDoc.toStdString() << ".";

        reportVarSetInfo(message.str().c_str());
        if (checkExpr) {
            // We have to check the text of the expression as well
            try {
                checkExpression(ui->expression->text());
                ui->okBtn->setEnabled(true);
            }
            catch (Base::Exception&) {
                ui->okBtn->setDisabled(true);
            }
        }
    }
    else {
        ui->okBtn->setEnabled(false);
        reportVarSetInfo("Please select a variable set.");
    }
}

#include "moc_DlgExpressionInput.cpp"
