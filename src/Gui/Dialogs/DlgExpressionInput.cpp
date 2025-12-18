/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
 *   Copyright (c) 2025 Pieter Hijma <info@pieterhijma.net>                *
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

#include <QApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QTreeWidget>
#include <QStyledItemDelegate>

#include <fmt/format.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/ExpressionParser.h>
#include <App/VarSet.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <regex>

#include "Dialogs/DlgExpressionInput.h"
#include "ui_DlgExpressionInput.h"
#include "Application.h"
#include "Command.h"
#include "Tools.h"
#include "ExpressionBinding.h"
#include "BitmapFactory.h"
#include "ViewProviderDocumentObject.h"

using namespace App;
using namespace Gui::Dialog;

FC_LOG_LEVEL_INIT("DlgExpressionInput", true, true)

DlgExpressionInput::DlgExpressionInput(
    const App::ObjectIdentifier& _path,
    std::shared_ptr<const Expression> _expression,
    const Base::Unit& _impliedUnit,
    QWidget* parent
)
    : QDialog(parent)
    , ui(new Ui::DlgExpressionInput)
    , expression(_expression ? _expression->copy() : nullptr)
    , path(_path)
    , discarded(false)
    , impliedUnit(_impliedUnit)
    , varSetsVisible(false)
    , comboBoxGroup(this)
{
    assert(path.getDocumentObject());

    // Setup UI
    ui->setupUi(this);
    okBtn = ui->buttonBox->button(QDialogButtonBox::Ok);
    discardBtn = ui->buttonBox->button(QDialogButtonBox::Reset);
    discardBtn->setToolTip(tr("Revert to last calculated value (as constant)"));

    initializeVarSets();

    // Connect signal(s)
    connect(ui->expression, &ExpressionTextEdit::textChanged, this, &DlgExpressionInput::textChanged);
    connect(discardBtn, &QPushButton::clicked, this, &DlgExpressionInput::setDiscarded);

    if (expression) {
        ui->expression->setPlainText(QString::fromStdString(expression->toString()));
    }
    else {
        QVariant text = parent->property("text");
        if (text.canConvert<QString>()) {
            ui->expression->setPlainText(text.toString());
        }
    }

    // Set document object on text edit to create auto completer
    DocumentObject* docObj = path.getDocumentObject();
    ui->expression->setDocumentObject(docObj);

    // There are some platforms where setting no system background causes a black
    // rectangle to appear. To avoid this the 'NoSystemBackground' parameter can be
    // set to false. Then a normal non-modal dialog will be shown instead (#0002440).
    bool noBackground = App::GetApplication()
                            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Expression")
                            ->GetBool("NoSystemBackground", false);

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
        ui->expression->setMinimumWidth(300);
        ui->expression->setMinimumHeight(80);
        ui->msg->setWordWrap(true);
        ui->msg->setMaximumHeight(200);
        ui->msg->setMinimumWidth(280);
        ui->verticalLayout->setContentsMargins(9, 9, 9, 9);
        this->adjustSize();
        // It is strange that (at least on Linux) DlgExpressionInput will shrink
        // to be narrower than ui->expression after calling adjustSize() above.
        // Why?
        if (this->width() < ui->expression->width() + 18) {
            this->resize(ui->expression->width() + 18, this->height());
        }
    }
    ui->expression->setFocus();
}

DlgExpressionInput::~DlgExpressionInput()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    disconnect(
        ui->checkBoxVarSets,
        &QCheckBox::checkStateChanged,
        this,
        &DlgExpressionInput::onCheckVarSets
    );
#else
    disconnect(ui->checkBoxVarSets, &QCheckBox::stateChanged, this, &DlgExpressionInput::onCheckVarSets);
#endif
    disconnect(
        ui->comboBoxVarSet,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        &DlgExpressionInput::onVarSetSelected
    );
    disconnect(
        &comboBoxGroup,
        &EditFinishedComboBox::currentTextChanged,
        this,
        &DlgExpressionInput::onTextChangedGroup
    );
    disconnect(ui->lineEditPropNew, &QLineEdit::textChanged, this, &DlgExpressionInput::namePropChanged);

    delete ui;
}

static void getVarSetsDocument(std::vector<App::VarSet*>& varSets, App::Document* doc)
{
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
    if (typePath == App::PropertyString::getClassTypeId()
        || typePath.isDerivedFrom(App::PropertyFloat::getClassTypeId())
        || typePath.isDerivedFrom(App::PropertyInteger::getClassTypeId())) {
        return typePath;
    }

    // If we cannot determine the type by means of the path, for example when
    // dealing with a sketcher constraint list or with the x, y, or z of a
    // Placement, the type of the unit allows us to create a property in the
    // varset.  Since unit properties are derived from App::PropertyFloat, it
    // allows us to create a property and set the value.

    std::string unitTypeString = impliedUnit.getTypeString();
    if (unitTypeString.empty()) {
        // no type was provided
        return Base::Type::BadType;
    }

    std::string typeString = "App::Property" + unitTypeString;
    // may return badType
    return Base::Type::fromName(typeString.c_str());
}

bool DlgExpressionInput::typeOkForVarSet()
{
    std::string unitType = impliedUnit.getTypeString();
    return !determineTypeVarSet().isBad();
}

void DlgExpressionInput::initializeErrorFrame()
{
    ui->errorFrame->setVisible(false);
    const int size = style()->pixelMetric(QStyle::PM_LargeIconSize);
    QIcon icon = Gui::BitmapFactory().iconFromTheme("overlay_error");
    if (icon.isNull()) {
        icon = style()->standardIcon(QStyle::SP_MessageBoxCritical);
    }
    ui->errorIconLabel->setPixmap(icon.pixmap(QSize(size, size)));
}

void DlgExpressionInput::initializeVarSets()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    connect(ui->checkBoxVarSets, &QCheckBox::checkStateChanged, this, &DlgExpressionInput::onCheckVarSets);
#else
    connect(ui->checkBoxVarSets, &QCheckBox::stateChanged, this, &DlgExpressionInput::onCheckVarSets);
#endif
    connect(
        ui->comboBoxVarSet,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        &DlgExpressionInput::onVarSetSelected
    );
    connect(
        &comboBoxGroup,
        &EditFinishedComboBox::currentTextChanged,
        this,
        &DlgExpressionInput::onTextChangedGroup
    );
    connect(ui->lineEditPropNew, &QLineEdit::textChanged, this, &DlgExpressionInput::namePropChanged);

    comboBoxGroup.setObjectName(QStringLiteral("comboBoxGroup"));
    comboBoxGroup.setInsertPolicy(QComboBox::InsertAtTop);
    comboBoxGroup.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    DlgAddProperty::setWidgetForLabel("labelGroup", &comboBoxGroup, ui->formLayout);
    setTabOrder(ui->comboBoxVarSet, &comboBoxGroup);
    setTabOrder(&comboBoxGroup, ui->lineEditPropNew);

    std::vector<App::VarSet*> varSets = getAllVarSets();
    if (!varSets.empty() && typeOkForVarSet()) {
        ui->checkBoxVarSets->setVisible(true);
        ui->checkBoxVarSets->setCheckState(Qt::Unchecked);
        ui->groupBoxVarSets->setVisible(false);
    }
    else {
        // The dialog is shown without any VarSet options.
        ui->checkBoxVarSets->setVisible(false);
        ui->groupBoxVarSets->setVisible(false);
    }
    initializeErrorFrame();
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
    if (!defined) {
        return;
    }

    auto toQString = [](const Base::Quantity& v) {
        return QString::fromStdString(v.getUserString());
    };

    if (value.getValue() < minimum || value.getValue() > maximum) {
        Base::Quantity minVal(minimum, value.getUnit());
        Base::Quantity maxVal(maximum, value.getUnit());

        const QString fmt
            = QCoreApplication::translate("Exceptions", "Value out of range (%1 out of [%2, %3])");
        const QString msg = fmt.arg(toQString(value), toQString(minVal), toQString(maxVal));
        THROWM(Base::ValueError, msg.toStdString());
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

bool DlgExpressionInput::checkCyclicDependencyVarSet(const QString& text)
{
    std::shared_ptr<Expression> expr(
        ExpressionParser::parse(path.getDocumentObject(), text.toUtf8().constData())
    );

    if (expr) {
        DocumentObject* obj = path.getDocumentObject();
        auto ids = expr->getIdentifiers();

        for (const auto& id : ids) {
            if (id.first.getDocumentObject() == obj) {
                // This string is not translated.  It is based on a string that
                // originates from the expression validator in App that is also
                // not translated.
                ui->msg->setText(
                    QString::fromStdString(id.first.toString() + " reference causes a cyclic dependency")
                );
                return true;
            }
        }
    }

    return false;
}

void DlgExpressionInput::checkExpression(const QString& text)
{
    // now handle expression
    std::shared_ptr<Expression> expr(
        ExpressionParser::parse(path.getDocumentObject(), text.toUtf8().constData())
    );

    if (expr) {
        std::string error = path.getDocumentObject()->ExpressionEngine.validateExpression(path, expr);

        if (!error.empty()) {
            throw Base::RuntimeError(error.c_str());
        }

        std::unique_ptr<Expression> result(expr->eval());

        expression = expr;
        okBtn->setEnabled(true);
        ui->msg->clear();

        // set default palette as we may have read text right now
        ui->msg->setPalette(okBtn->palette());

        auto* n = freecad_cast<NumberExpression*>(result.get());
        if (n) {
            Base::Quantity value = n->getQuantity();
            if (!value.isValid()) {
                THROWMT(Base::ValueError, QT_TRANSLATE_NOOP("Exceptions", "Not a number"));
            }

            QString msg = QString::fromStdString(value.getUserString());
            if (impliedUnit != Base::Unit::One) {
                if (!value.isDimensionless() && value.getUnit() != impliedUnit) {
                    THROWMT(
                        Base::UnitsMismatchError,
                        QT_TRANSLATE_NOOP("Exceptions", "Unit mismatch between result and required unit")
                    );
                }

                value.setUnit(impliedUnit);
            }
            else if (!value.isDimensionless()) {
                msg += tr(" (Warning: unit discarded)");

                QPalette p(ui->msg->palette());
                p.setColor(QPalette::WindowText, Qt::red);
                ui->msg->setPalette(p);
            }

            numberRange.throwIfOutOfRange(value);
            message = msg.toStdString();
        }
        else {
            message = result->toString();
        }
        setMsgText();
    }
}

static const bool NoCheckExpr = false;

void DlgExpressionInput::textChanged()
{
    const QString& text = ui->expression->toPlainText();

    if (text.isEmpty()) {
        okBtn->setDisabled(true);
        discardBtn->setDefault(true);
        return;
    }

    okBtn->setDefault(true);

    try {
        checkExpression(text);
        if (varSetsVisible) {
            // If varsets are visible, check whether the varset info also
            // agrees that the button should be enabled.
            // No need to check the expression in that function.
            updateVarSetInfo(NoCheckExpr);
        }
    }
    catch (Base::Exception& e) {
        message = e.what();
        setMsgText();
        QPalette p(ui->msg->palette());
        p.setColor(QPalette::WindowText, Qt::red);
        ui->msg->setPalette(p);
        okBtn->setDisabled(true);
    }
}

void DlgExpressionInput::setDiscarded()
{
    discarded = true;
    reject();
}

void DlgExpressionInput::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
}

void DlgExpressionInput::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);

    // The 'FramelessWindowHint' is also set when the background is transparent.
    if (windowFlags() & Qt::FramelessWindowHint) {
        // we need to reject the dialog when clicked on the background. As the background is
        // transparent this is the expected behaviour for the user
        bool on = ui->expression->completerActive();
        if (!on) {
            this->reject();
        }
    }
}

void DlgExpressionInput::show()
{
    QDialog::show();
    this->activateWindow();
    ui->expression->selectAll();
}

class Binding: public Gui::ExpressionBinding
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

static constexpr const char* InvalidIdentifierMessage = QT_TR_NOOP(
    "must contain only alphanumeric characters, underscore, and must not start with a digit"
);

bool DlgExpressionInput::isPropertyNameValid(
    const QString& nameProp,
    const App::DocumentObject* obj,
    QString& message
) const
{
    auto withPrefix = [&](const QString& detail) {
        return tr("Invalid property name: %1").arg(detail);
    };

    if (!obj) {
        message = tr("Unknown object");
        return false;
    }

    std::string name = nameProp.toStdString();
    if (name.empty()) {
        message = withPrefix(tr("the name cannot be empty"));
        return false;
    }

    if (name != Base::Tools::getIdentifier(name)) {
        message = withPrefix(tr(InvalidIdentifierMessage));
        return false;
    }

    if (ExpressionParser::isTokenAUnit(name)) {
        message = withPrefix(tr("%1 is a unit").arg(nameProp));
        return false;
    }

    if (ExpressionParser::isTokenAConstant(name)) {
        message = withPrefix(tr("%1 is a constant").arg(nameProp));
        return false;
    }

    auto prop = obj->getPropertyByName(name.c_str());
    if (prop && prop->getContainer() == obj) {
        message = withPrefix(tr("%1 already exists").arg(nameProp));
        return false;
    }

    return true;
}

static const int DocRole = Qt::UserRole;
static const int VarSetNameRole = Qt::UserRole + 1;
static const int VarSetLabelRole = Qt::UserRole + 2;
static const int LevelRole = Qt::UserRole + 3;

static QString getValue(QComboBox* comboBox, int role)
{
    QVariant variant = comboBox->currentData(role);
    return variant.toString();
}

static void storePreferences(
    const std::string& nameDoc,
    const std::string& nameVarSet,
    const std::string& nameGroup
)
{
    auto paramExpressionEditor = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/ExpressionEditor"
    );
    paramExpressionEditor->SetASCII("LastDocument", nameDoc);
    paramExpressionEditor->SetASCII("LastVarSet", nameVarSet);
    paramExpressionEditor->SetASCII("LastGroup", nameGroup);
}

static const App::NumberExpression* toNumberExpr(const App::Expression* expr)
{
    return freecad_cast<const App::NumberExpression*>(expr);
}

static const App::StringExpression* toStringExpr(const App::Expression* expr)
{
    return freecad_cast<const App::StringExpression*>(expr);
}

static const App::OperatorExpression* toUnitNumberExpr(const App::Expression* expr)
{
    auto* opExpr = freecad_cast<const App::OperatorExpression*>(expr);
    if (opExpr && opExpr->getOperator() == App::OperatorExpression::Operator::UNIT
        && toNumberExpr(opExpr->getLeft())) {
        return opExpr;
    }
    return nullptr;
}

void DlgExpressionInput::createBindingVarSet(App::Property* propVarSet, App::DocumentObject* varSet)
{
    ObjectIdentifier varSetId(*propVarSet);

    // rewrite the identifiers of the expression to be relative to the VarSet
    std::map<App::ObjectIdentifier, bool> identifiers = expression->getIdentifiers();

    std::map<ObjectIdentifier, ObjectIdentifier> idsFromObjToVarSet;
    for (const auto& idPair : identifiers) {
        ObjectIdentifier exprId = idPair.first;
        ObjectIdentifier relativeId = exprId.relativeTo(varSetId);
        idsFromObjToVarSet[exprId] = relativeId;
    }

    Binding binding;
    binding.bind(*propVarSet);
    binding.setExpression(expression);
    binding.apply();

    varSet->renameObjectIdentifiers(idsFromObjToVarSet);
}

void DlgExpressionInput::acceptWithVarSet()
{
    // all checks have been performed in updateVarSetInfo and textChanged that
    // decide to enable the button

    // create a property in the VarSet
    QString nameVarSet = getValue(ui->comboBoxVarSet, VarSetNameRole);
    QString nameGroup = comboBoxGroup.currentText();
    QString nameProp = ui->lineEditPropNew->text();

    QString nameDoc = getValue(ui->comboBoxVarSet, DocRole);
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
    const Expression* expr = expression.get();
    if (const NumberExpression* ne = toNumberExpr(expr)) {
        // the value is a number: directly assign it to the property instead of
        // making it an expression in the variable set
        Gui::Command::doCommand(
            Gui::Command::Doc,
            "App.getDocument('%s').getObject('%s').%s = %f",
            obj->getDocument()->getName(),
            obj->getNameInDocument(),
            prop->getName(),
            ne->getValue()
        );
    }
    else if (const StringExpression* se = toStringExpr(expr)) {
        // the value is a string: directly assign it to the property.
        Gui::Command::doCommand(
            Gui::Command::Doc,
            "App.getDocument('%s').getObject('%s').%s = \"%s\"",
            obj->getDocument()->getName(),
            obj->getNameInDocument(),
            prop->getName(),
            se->getText().c_str()
        );
    }
    else if (const OperatorExpression* une = toUnitNumberExpr(expr)) {
        // the value is a unit number: directly assign it to the property.
        Gui::Command::doCommand(
            Gui::Command::Doc,
            "App.getDocument('%s').getObject('%s').%s = \"%s\"",
            obj->getDocument()->getName(),
            obj->getNameInDocument(),
            prop->getName(),
            une->toString().c_str()
        );
    }
    else {
        // the value is an expression: make an expression binding in the VarSet
        createBindingVarSet(prop, obj);
    }

    // Create a new expression that refers to the property in the VarSet
    // for the original property that is the target of this dialog.
    expression.reset(ExpressionParser::parse(path.getDocumentObject(), prop->getFullName().c_str()));

    storePreferences(nameDoc.toStdString(), nameVarSet.toStdString(), group);
}

void DlgExpressionInput::accept()
{
    if (varSetsVisible) {
        if (needReportOnVarSet()) {
            return;
        }
        acceptWithVarSet();
    }
    QDialog::accept();
}

static App::Document* getPreselectedDocument()
{
    auto paramExpressionEditor = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/ExpressionEditor"
    );
    std::string lastDoc = paramExpressionEditor->GetASCII("LastDocument", "");

    if (lastDoc.empty()) {
        return App::GetApplication().getActiveDocument();
    }

    App::Document* doc = App::GetApplication().getDocument(lastDoc.c_str());
    if (doc == nullptr) {
        return App::GetApplication().getActiveDocument();
    }

    return doc;
}


int DlgExpressionInput::getVarSetIndex(const App::Document* doc) const
{
    auto paramExpressionEditor = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/ExpressionEditor"
    );
    std::string lastVarSet = paramExpressionEditor->GetASCII("LastVarSet", "VarSet");

    auto* model = qobject_cast<QStandardItemModel*>(ui->comboBoxVarSet->model());
    for (int i = 0; i < model->rowCount(); ++i) {
        QStandardItem* item = model->item(i);
        if (item->data(DocRole).toString() == QString::fromUtf8(doc->getName())
            && item->data(VarSetNameRole).toString() == QString::fromStdString(lastVarSet)) {
            return i;
        }
    }

    // Select the first varset of the first document (the document is item 0)
    return 1;
}

void DlgExpressionInput::preselectVarSet()
{
    const App::Document* doc = getPreselectedDocument();
    if (doc == nullptr) {
        FC_ERR("No active document found");
    }
    ui->comboBoxVarSet->setCurrentIndex(getVarSetIndex(doc));
}

// Custom delegate to add indentation
class IndentedItemDelegate: public QStyledItemDelegate
{
public:
    explicit IndentedItemDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent)
    {}

    void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);

        if (index.data(LevelRole) == 1) {
            int indentWidth = 20;
            option->rect.adjust(indentWidth, 0, 0, 0);
        }
    }
};

static void addVarSetsVarSetComboBox(
    std::vector<App::VarSet*>& varSets,
    QStandardItem* docItem,
    QStandardItemModel* model
)
{
    for (auto* varSet : varSets) {
        auto* vp = freecad_cast<Gui::ViewProviderDocumentObject*>(
            Gui::Application::Instance->getViewProvider(varSet)
        );
        if (vp == nullptr) {
            FC_ERR("No ViewProvider found for VarSet: " << varSet->getNameInDocument());
            continue;
        }

        // The item will be owned by the model, so no need to delete it manually.
        auto item = new QStandardItem();
        item->setIcon(vp->getIcon());
        item->setText(QString::fromUtf8(varSet->Label.getValue()));
        item->setData(QString::fromUtf8(varSet->Label.getValue()), VarSetLabelRole);
        item->setData(QString::fromUtf8(varSet->getNameInDocument()), VarSetNameRole);
        item->setData(docItem->data(DocRole), DocRole);
        item->setData(1, LevelRole);
        model->appendRow(item);
    }
}

static void addDocVarSetComboBox(App::Document* doc, QPixmap& docIcon, QStandardItemModel* model)
{
    if (doc->testStatus(App::Document::TempDoc)) {
        // Do not add temporary documents to the VarSet combo box
        return;
    }

    std::vector<App::VarSet*> varSets;
    getVarSetsDocument(varSets, doc);
    if (varSets.empty()) {
        return;
    }

    // The item will be owned by the model, so no need to delete it manually.
    auto* item = new QStandardItem();
    item->setIcon(docIcon);
    item->setText(QString::fromUtf8(doc->Label.getValue()));
    item->setData(QByteArray(doc->getName()), DocRole);
    item->setFlags(Qt::ItemIsEnabled);  // Make sure this item cannot be selected
    item->setData(0, LevelRole);
    model->appendRow(item);

    addVarSetsVarSetComboBox(varSets, item, model);
}

QStandardItemModel* DlgExpressionInput::createVarSetModel()
{
    // Create the model
    auto* model = new QStandardItemModel(ui->comboBoxVarSet);
    model->setColumnCount(1);

    // Add items to the model
    QPixmap docIcon(Gui::BitmapFactory().pixmap("Document"));
    std::vector<App::Document*> docs = App::GetApplication().getDocuments();

    for (auto doc : docs) {
        addDocVarSetComboBox(doc, docIcon, model);
    }

    return model;
}

void DlgExpressionInput::setupVarSets()
{
    QStandardItemModel* model = createVarSetModel();
    {
        QSignalBlocker blocker(ui->comboBoxVarSet);
        ui->comboBoxVarSet->clear();
        auto* listView = new QListView(this);
        listView->setSelectionMode(QAbstractItemView::SingleSelection);
        listView->setModel(model);
        ui->comboBoxVarSet->setView(listView);
        ui->comboBoxVarSet->setModel(model);
        ui->comboBoxVarSet->setItemDelegate(new IndentedItemDelegate(ui->comboBoxVarSet));
    }
    preselectVarSet();

    okBtn->setEnabled(false);
}

std::string DlgExpressionInput::getType()
{
    return determineTypeVarSet().getName();
}

void DlgExpressionInput::onCheckVarSets(int state)
{
    varSetsVisible = state == Qt::Checked;
    ui->groupBoxVarSets->setVisible(varSetsVisible);
    if (varSetsVisible) {
        setupVarSets();
    }
    else {
        try {
            checkExpression(ui->expression->toPlainText());
        }
        catch (Base::Exception&) {
            okBtn->setEnabled(false);
        }
        adjustSize();
    }
}

void DlgExpressionInput::preselectGroup()
{
    auto paramExpressionEditor = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/ExpressionEditor"
    );
    std::string lastGroup = paramExpressionEditor->GetASCII("LastGroup", "");

    if (lastGroup.empty()) {
        return;
    }

    if (int index = comboBoxGroup.findText(QString::fromStdString(lastGroup)); index != -1) {
        comboBoxGroup.setCurrentIndex(index);
    }
}

void DlgExpressionInput::onVarSetSelected(int /*index*/)
{
    QString docName = getValue(ui->comboBoxVarSet, DocRole);
    QString varSetName = getValue(ui->comboBoxVarSet, VarSetNameRole);
    if (docName.isEmpty() || varSetName.isEmpty()) {
        FC_ERR("No document or variable set selected");
        return;
    }

    App::Document* doc = App::GetApplication().getDocument(docName.toUtf8());
    if (doc == nullptr) {
        FC_ERR("Document not found: " << docName.toStdString());
        return;
    }

    App::DocumentObject* varSet = doc->getObject(varSetName.toUtf8());
    if (varSet == nullptr) {
        FC_ERR("Variable set not found: " << varSetName.toStdString());
        return;
    }

    DlgAddProperty::populateGroup(comboBoxGroup, varSet);
    preselectGroup();
    updateVarSetInfo();
    ui->lineEditPropNew->setFocus();
}

void DlgExpressionInput::onTextChangedGroup(const QString&)
{
    updateVarSetInfo();
}

void DlgExpressionInput::namePropChanged(const QString&)
{
    updateVarSetInfo();
}

bool DlgExpressionInput::isGroupNameValid(const QString& nameGroup, QString& message) const
{
    auto withPrefix = [&](const QString& detail) {
        return tr("Invalid group name: %1").arg(detail);
    };

    if (nameGroup.isEmpty()) {
        message = withPrefix(tr("the name cannot be empty"));
        return false;
    }
    std::string name = nameGroup.toStdString();

    if (name != Base::Tools::getIdentifier(name)) {
        message = withPrefix(tr(InvalidIdentifierMessage));
        return false;
    }

    return true;
}

void DlgExpressionInput::reportVarSetInfo(const QString& message)
{
    if (!message.isEmpty()) {
        ui->errorFrame->setVisible(true);
        ui->errorTextLabel->setText(message);
        ui->errorTextLabel->updateGeometry();
    }
}

static void setErrorState(QWidget* widget, bool on)
{
    widget->setProperty("validationState", on ? QStringLiteral("error") : QVariant());

    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
}

bool DlgExpressionInput::reportGroup(const QString& nameGroup)
{
    QString message;
    if (!isGroupNameValid(nameGroup, message)) {
        setErrorState(&comboBoxGroup, true);
        reportVarSetInfo(message);
        return true;
    }

    return false;
}

bool DlgExpressionInput::reportName()
{
    QString nameProp = ui->lineEditPropNew->text();
    QString nameVarSet = getValue(ui->comboBoxVarSet, VarSetNameRole);
    QString nameDoc = getValue(ui->comboBoxVarSet, DocRole);
    App::Document* doc = App::GetApplication().getDocument(nameDoc.toUtf8());
    App::DocumentObject* obj = doc->getObject(nameVarSet.toUtf8());
    QString message;
    if (!isPropertyNameValid(nameProp, obj, message)) {
        setErrorState(ui->lineEditPropNew, true);
        reportVarSetInfo(message);
        return true;
    }

    return false;
}

void DlgExpressionInput::updateVarSetInfo(bool checkExpr)
{
    if (ui->lineEditPropNew->text().isEmpty()) {
        okBtn->setEnabled(false);
        return;
    }

    if (comboBoxGroup.currentText().isEmpty()) {
        okBtn->setEnabled(false);
        return;
    }

    if (checkCyclicDependencyVarSet(ui->expression->toPlainText())) {
        okBtn->setEnabled(false);
        return;
    }

    if (checkExpr) {
        // We have to check the text of the expression as well
        try {
            checkExpression(ui->expression->toPlainText());
        }
        catch (Base::Exception&) {
            okBtn->setEnabled(false);
        }
    }

    okBtn->setEnabled(true);
}

bool DlgExpressionInput::needReportOnVarSet()
{
    setErrorState(ui->lineEditPropNew, false);
    setErrorState(&comboBoxGroup, false);

    return reportGroup(comboBoxGroup.currentText()) || reportName();
}

void DlgExpressionInput::resizeEvent(QResizeEvent* event)
{
    // When the dialog is resized, message text may need to be re-wrapped
    if (!this->message.empty() && event->size() != event->oldSize()) {
        setMsgText();
    }
    QDialog::resizeEvent(event);
}

void DlgExpressionInput::setMsgText()
{
    if (!this->message.size()) {
        return;
    }

    const QFontMetrics msgFontMetrics {ui->msg->font()};

    // find words longer than length of msg widget
    // then insert newline to wrap it
    std::string wrappedMsg {};
    const int msgContentWidth = ui->msg->width() * 0.85;  // 0.85 is a magic number for some padding
    const int maxWordLength = msgContentWidth / msgFontMetrics.averageCharWidth();

    const auto wrappableWordPattern = std::regex {"\\S{" + std::to_string(maxWordLength) + "}"};
    auto it = std::sregex_iterator {this->message.cbegin(), this->message.cend(), wrappableWordPattern};
    const auto itEnd = std::sregex_iterator {};

    int lastPos = 0;
    for (; it != itEnd; ++it) {
        wrappedMsg += this->message.substr(lastPos, it->position() - lastPos);
        wrappedMsg += it->str() + "\n";
        lastPos = it->position() + it->length();
    }
    wrappedMsg += this->message.substr(lastPos);

    ui->msg->setText(QString::fromStdString(wrappedMsg));

    // elide text if it is going out of widget bounds
    // note: this is only 'rough elide', as this text is usually not very long;
    const int msgLinesLimit = 3;
    if (static_cast<int>(wrappedMsg.size())
        > msgContentWidth / msgFontMetrics.averageCharWidth() * msgLinesLimit) {
        const QString elidedMsg = msgFontMetrics.elidedText(
            QString::fromStdString(wrappedMsg),
            Qt::ElideRight,
            msgContentWidth * msgLinesLimit
        );
        ui->msg->setText(elidedMsg);
    }
}

#include "moc_DlgExpressionInput.cpp"
