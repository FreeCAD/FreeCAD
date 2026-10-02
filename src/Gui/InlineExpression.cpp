// SPDX-License-Identifier: LGPL-2.1-or-later

#include "InlineExpression.h"

#include <QObject>
#include <QRegularExpression>

#include <cmath>
#include <cctype>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <algorithm>

#include <App/Application.h>
#include <App/AutoTransaction.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Expression.h>
#include <App/ExpressionParser.h>
#include <App/ObjectIdentifier.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <App/VarSet.h>
#include <Base/Quantity.h>
#include <Base/Tools.h>

#include "Application.h"
#include "ViewProviderDocumentObject.h"

namespace
{
QString trimTrailingStatementDelimiter(QString text)
{
    text = text.trimmed();
    while (text.endsWith(QLatin1Char(';'))) {
        text.chop(1);
        text = text.trimmed();
    }
    return text;
}

bool toIntegerLiteral(double value, long long& integer)
{
    if (!std::isfinite(value)) {
        return false;
    }
    double intPart = 0.0;
    if (std::modf(value, &intPart) != 0.0) {
        return false;
    }
    if (intPart < static_cast<double>(std::numeric_limits<long long>::min())
        || intPart > static_cast<double>(std::numeric_limits<long long>::max())) {
        return false;
    }
    integer = static_cast<long long>(intPart);
    return true;
}

// Rollback must restore numeric values exactly, without the engine's epsilon comparison.
bool sameStoredValue(const App::any& before, const App::any& after)
{
    if (before.type() == typeid(double) && after.type() == typeid(double)) {
        return boost::any_cast<double>(before) == boost::any_cast<double>(after);
    }
    if (before.type() == typeid(Base::Quantity) && after.type() == typeid(Base::Quantity)) {
        const auto& previous = boost::any_cast<const Base::Quantity&>(before);
        const auto& current = boost::any_cast<const Base::Quantity&>(after);
        return previous.getValue() == current.getValue() && previous.getUnit() == current.getUnit();
    }
    return App::isAnyEqual(before, after);
}

void clearPropertyExpression(App::DocumentObject* owner, App::Property* prop)
{
    if (!owner || !prop) {
        return;
    }
    owner->ExpressionEngine.setValue(App::ObjectIdentifier(*prop), std::shared_ptr<App::Expression>());
}

bool assignNumberLiteral(App::DocumentObject* owner, App::Property* prop, double value, QString& message)
{
    if (auto* integerProp = freecad_cast<App::PropertyInteger*>(prop)) {
        long long integer = 0;
        if (!toIntegerLiteral(value, integer)) {
            message = QObject::tr("Integer variable requires an integer value.");
            return false;
        }
        integerProp->setValue(static_cast<long>(integer));
        clearPropertyExpression(owner, prop);
        return true;
    }
    if (auto* floatProp = freecad_cast<App::PropertyFloat*>(prop)) {
        floatProp->setValue(value);
        clearPropertyExpression(owner, prop);
        return true;
    }
    message = QObject::tr("Could not assign numeric value to variable property.");
    return false;
}

// Rewrite a copied expression before installing it, leaving other VarSet bindings alone.
class AssignmentExpressionVisitor: public App::ExpressionVisitor
{
public:
    AssignmentExpressionVisitor(const App::Expression& expression, const App::ObjectIdentifier& path)
        : path(path)
    {
        for (const auto& [identifier, hidden] : expression.getIdentifiers()) {
            identifiers.emplace(identifier.canonicalPath(), identifier.relativeTo(path));
        }
    }

    void visit(App::Expression& expression) override
    {
        renameObjectIdentifier(expression, identifiers, path);
    }

private:
    std::map<App::ObjectIdentifier, App::ObjectIdentifier> identifiers;
    const App::ObjectIdentifier& path;
};

bool isIdentifierStart(char c)
{
    const auto uc = static_cast<unsigned char>(c);
    return std::isalpha(uc) || c == '_';
}

bool isIdentifierChar(char c)
{
    const auto uc = static_cast<unsigned char>(c);
    return std::isalnum(uc) || c == '_' || c == '@';
}

size_t skipSpaces(const std::string& input, size_t index)
{
    while (index < input.size() && std::isspace(static_cast<unsigned char>(input[index]))) {
        ++index;
    }
    return index;
}

bool hasDotBefore(const std::string& input, size_t index)
{
    while (index > 0) {
        --index;
        if (!std::isspace(static_cast<unsigned char>(input[index]))) {
            return input[index] == '.';
        }
    }
    return false;
}

bool hasDotAfter(const std::string& input, size_t index)
{
    index = skipSpaces(input, index);
    return index < input.size() && input[index] == '.';
}

bool isFunctionCall(const std::string& input, size_t index)
{
    index = skipSpaces(input, index);
    return index < input.size() && input[index] == '(';
}

size_t endOfObjectReferenceString(const std::string& input, size_t start)
{
    size_t index = start + 2;
    while (index < input.size()) {
        if (input[index] == '\\') {
            index += (index + 1 < input.size()) ? 2 : 1;
            continue;
        }
        if (input[index] == '>' && index + 1 < input.size() && input[index + 1] == '>') {
            return index + 2;
        }
        ++index;
    }
    return input.size();
}

App::DocumentObject* findUniqueVarSetByLabel(App::Document* doc, const QString& label, QString& message)
{
    if (!doc) {
        return nullptr;
    }

    App::DocumentObject* varSet = nullptr;
    const std::string target = label.toStdString();
    for (auto* obj : doc->getObjects()) {
        if (!obj || obj->Label.getValue() != target
            || !obj->isDerivedFrom(App::VarSet::getClassTypeId())) {
            continue;
        }
        if (varSet) {
            message = QObject::tr("VarSet label is ambiguous: %1").arg(label);
            return nullptr;
        }
        varSet = obj;
    }

    return varSet;
}

App::DocumentObject* getOrCreateDefaultVarSet(
    App::Document* doc,
    QString& message,
    Gui::InlineExpression::AssignmentGuard* guard
)
{
    if (!doc) {
        message = QObject::tr("Unknown document");
        return nullptr;
    }

    App::DocumentObject* varSet = doc->getObject(Gui::InlineExpression::DefaultVarSetName);
    if (varSet && !varSet->isDerivedFrom(App::VarSet::getClassTypeId())) {
        message
            = QObject::tr("Object is not a VarSet: %1").arg(Gui::InlineExpression::DefaultVarSetName);
        return nullptr;
    }

    if (!varSet) {
        for (auto* obj : doc->getObjects()) {
            if (obj && obj->Label.getValue() == std::string(Gui::InlineExpression::DefaultVarSetName)
                && !obj->isDerivedFrom(App::VarSet::getClassTypeId())) {
                message = QObject::tr("Object labeled '%1' is not a VarSet")
                              .arg(Gui::InlineExpression::DefaultVarSetName);
                return nullptr;
            }
        }

        varSet = doc->addObject("App::VarSet", Gui::InlineExpression::DefaultVarSetName);
        if (!varSet) {
            message = QObject::tr("Could not create VarSet: %1")
                          .arg(Gui::InlineExpression::DefaultVarSetName);
            return nullptr;
        }

        if (guard) {
            guard->recordCreatedVarSet(varSet);
        }
        if (Gui::Application::Instance) {
            if (auto* vp = freecad_cast<Gui::ViewProviderDocumentObject*>(
                    Gui::Application::Instance->getViewProvider(varSet)
                )) {
                vp->setTreeRank(0);
            }
        }
    }

    return varSet;
}

bool hasPropertyInDefaultVarSet(App::DocumentObject* varSet, const std::string& name)
{
    if (!varSet || !varSet->isDerivedFrom(App::VarSet::getClassTypeId())) {
        return false;
    }

    App::Property* prop = varSet->getPropertyByName(name.c_str());
    return prop && prop->getContainer() == varSet;
}

}  // namespace

namespace Gui::InlineExpression
{

QString normalizeInput(QString text)
{
    return trimTrailingStatementDelimiter(text);
}

struct AssignmentGuard::State
{
    App::Document* doc;
    std::optional<App::AutoTransaction> transaction;
    std::string createdVarSet;

    struct Change
    {
        App::Document* doc;
        std::string object;
        App::ObjectIdentifier path;
        bool existed;
        App::any value;
        std::shared_ptr<App::Expression> expression;
    };
    std::vector<Change> changes;

    void remember(const App::ObjectIdentifier& path)
    {
        if (transaction || std::any_of(changes.begin(), changes.end(), [&](const Change& change) {
                return change.path == path;
            })) {
            return;
        }
        auto* owner = path.getDocumentObject();
        auto* prop = path.getProperty();
        Change change {owner->getDocument(), owner->getNameInDocument(), path, prop != nullptr, {}, {}};
        if (prop) {
            change.value = prop->getPathValue(path);
            if (auto expression = owner->getExpression(path).expression) {
                change.expression = expression->copy();
            }
        }
        changes.push_back(std::move(change));
    }
};

AssignmentGuard::AssignmentGuard(App::Document* doc, const App::ObjectIdentifier* target)
    : state(std::make_unique<State>())
{
    auto* targetOwner = target ? target->getDocumentObject() : nullptr;
    if (!doc || (target && !targetOwner)) {
        throw Base::RuntimeError("Unknown document");
    }
    state->doc = doc;
    auto* targetDoc = targetOwner ? targetOwner->getDocument() : nullptr;
    // Formula entry can assign into another document's VarSet. Join its existing tool
    // transaction, or book the target document under our ID, without closing either tool.
    if (doc->getBookedTransactionID() == 0 && targetDoc && targetDoc->getBookedTransactionID() != 0) {
        const auto id = targetDoc->getBookedTransactionID();
        doc->openTransaction(App::GetApplication().transactionDescription(id)->name, id);
    }
    if (doc->getBookedTransactionID() == 0) {
        state->transaction.emplace(doc, "Assign inline expression");
    }
    if (targetDoc && targetDoc != doc && targetDoc->getBookedTransactionID() == 0) {
        const auto id = doc->getBookedTransactionID();
        targetDoc->openTransaction(App::GetApplication().transactionDescription(id)->name, id);
    }
    if (target) {
        state->remember(*target);
    }
}

AssignmentGuard::~AssignmentGuard()
{
    if (!state) {
        return;
    }
    if (state->transaction) {
        state->transaction->close(App::TransactionCloseMode::Abort);
        return;
    }
    try {
        // Clear changed bindings first, so restoring old bindings cannot form a temporary cycle.
        for (const auto& change : state->changes) {
            auto* owner = change.doc->getObject(change.object.c_str());
            if (!owner || !change.path.getProperty()) {
                continue;
            }
            auto current = owner->getExpression(change.path).expression;
            if (current && (!change.expression || !current->isSame(*change.expression))) {
                owner->ExpressionEngine.setValue(change.path, nullptr);
            }
        }
        for (const auto& change : state->changes) {
            auto* owner = change.doc->getObject(change.object.c_str());
            if (!owner) {
                continue;
            }
            if (!change.existed) {
                owner->removeDynamicProperty(change.path.getPropertyName().c_str());
            }
            else if (change.path.getProperty()) {
                owner->ExpressionEngine.setValue(change.path, change.expression);
            }
        }
        // Restore parameters before the target, whose binding callbacks can update its value.
        for (auto it = state->changes.rbegin(); it != state->changes.rend(); ++it) {
            if (!it->existed || !it->doc->getObject(it->object.c_str())) {
                continue;
            }
            if (auto* prop = it->path.getProperty();
                prop && !sameStoredValue(it->value, prop->getPathValue(it->path))) {
                // A dimensionless quantity does not reset the unit in setPathValue().
                if (auto* quantity = freecad_cast<App::PropertyQuantity*>(prop)) {
                    quantity->setUnit(boost::any_cast<Base::Quantity>(it->value).getUnit());
                }
                prop->setPathValue(it->path, it->value);
            }
        }
        if (!state->createdVarSet.empty()) {
            state->doc->removeObject(state->createdVarSet.c_str());
        }
    }
    catch (const Base::Exception& error) {
        error.reportException();
    }
}

void AssignmentGuard::watch(App::DocumentObject* varSet, const QString& name)
{
    state->remember(App::ObjectIdentifier(varSet, name.toStdString()));
}

void AssignmentGuard::recordCreatedVarSet(App::DocumentObject* varSet)
{
    if (!state->transaction) {
        state->createdVarSet = varSet->getNameInDocument();
    }
}

void AssignmentGuard::commit()
{
    if (state && state->transaction) {
        state->transaction->close();
    }
    state.reset();
}

Assignment parseAssignment(const QString& text)
{
    Assignment assignment;
    QRegularExpressionMatch match;

    static const QRegularExpression labelAssignRegex(
        QStringLiteral(R"(^\s*<<(.*?)>>\s*\.\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*=(?!=)\s*(.+)$)")
    );
    match = labelAssignRegex.match(text);
    if (match.hasMatch()) {
        assignment.isAssignment = true;
        assignment.hasExplicitVarSet = true;
        assignment.isLabelVarSet = true;
        assignment.varSet = match.captured(1).trimmed();
        assignment.name = match.captured(2).trimmed();
        assignment.valueExpr = trimTrailingStatementDelimiter(match.captured(3));
        if (assignment.varSet.isEmpty() || assignment.name.isEmpty()
            || assignment.valueExpr.isEmpty()) {
            assignment = Assignment {};
        }
        return assignment;
    }

    static const QRegularExpression qualifiedAssignRegex(QStringLiteral(
        R"(^\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*\.\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*=(?!=)\s*(.+)$)"
    ));
    match = qualifiedAssignRegex.match(text);
    if (match.hasMatch()) {
        assignment.isAssignment = true;
        assignment.hasExplicitVarSet = true;
        assignment.varSet = match.captured(1).trimmed();
        assignment.name = match.captured(2).trimmed();
        assignment.valueExpr = trimTrailingStatementDelimiter(match.captured(3));
        if (assignment.varSet.isEmpty() || assignment.name.isEmpty()
            || assignment.valueExpr.isEmpty()) {
            assignment = Assignment {};
        }
        return assignment;
    }

    static const QRegularExpression assignRegex(
        QStringLiteral(R"(^\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*=(?!=)\s*(.+)$)")
    );
    match = assignRegex.match(text);
    if (!match.hasMatch()) {
        return assignment;
    }

    assignment.isAssignment = true;
    assignment.name = match.captured(1).trimmed();
    assignment.valueExpr = trimTrailingStatementDelimiter(match.captured(2));
    if (assignment.name.isEmpty() || assignment.valueExpr.isEmpty()) {
        assignment = Assignment {};
    }
    return assignment;
}

bool isValidName(const QString& name, QString& message)
{
    std::string nameStd = name.toStdString();
    if (nameStd != Base::Tools::getIdentifier(nameStd)) {
        message = QObject::tr("Invalid property name: %1")
                      .arg(
                          QObject::tr("must contain only alphanumeric characters, underscore, and must not start with a digit")
                      );
        return false;
    }
    if (App::ExpressionParser::isTokenAUnit(nameStd)) {
        message = QObject::tr("Invalid property name: %1 is a unit").arg(name);
        return false;
    }
    if (App::ExpressionParser::isTokenAConstant(nameStd)) {
        message = QObject::tr("Invalid property name: %1 is a constant").arg(name);
        return false;
    }
    return true;
}

App::DocumentObject* resolveExpressionOwner(App::DocumentObject* boundOwner, App::Document*& doc)
{
    if (boundOwner) {
        doc = boundOwner->getDocument();
        return boundOwner;
    }

    doc = App::GetApplication().getActiveDocument();
    if (!doc) {
        return nullptr;
    }

    App::DocumentObject* parameters = doc->getObject(DefaultVarSetName);
    if (parameters && parameters->isDerivedFrom(App::VarSet::getClassTypeId())) {
        return parameters;
    }

    // Parser fallback context for unbound widgets when no Parameters VarSet exists.
    for (auto* obj : doc->getObjects()) {
        if (obj) {
            return obj;
        }
    }

    return nullptr;
}

bool parseNumberExpression(
    const App::DocumentObject* owner,
    const QString& source,
    std::shared_ptr<App::Expression>& expr,
    Base::Quantity& quantity,
    QString& message
)
{
    try {
        expr = App::ExpressionParser::parse(owner, source.toUtf8().constData());
    }
    catch (const Base::Exception& e) {
        message = QString::fromUtf8(e.what());
        return false;
    }

    if (!expr) {
        message = QObject::tr("Invalid expression.");
        return false;
    }

    try {
        std::unique_ptr<App::Expression> result(expr->eval());
        auto* number = freecad_cast<App::NumberExpression*>(result.get());
        if (!number) {
            message = QObject::tr("Not a number");
            return false;
        }
        quantity = number->getQuantity();
        return true;
    }
    catch (const Base::Exception& e) {
        message = QString::fromUtf8(e.what());
        return false;
    }
}

App::DocumentObject* resolveVarSet(
    App::Document* doc,
    const Assignment& assignment,
    bool createDefault,
    QString& message,
    AssignmentGuard* guard
)
{
    if (!doc) {
        message = QObject::tr("Unknown document");
        return nullptr;
    }

    if (!assignment.hasExplicitVarSet) {
        return createDefault ? getOrCreateDefaultVarSet(doc, message, guard) : nullptr;
    }

    if (assignment.isLabelVarSet) {
        App::DocumentObject* varSet = findUniqueVarSetByLabel(doc, assignment.varSet, message);
        if (!varSet) {
            if (message.isEmpty()) {
                message = QObject::tr("VarSet not found by label: %1").arg(assignment.varSet);
            }
            return nullptr;
        }
        return varSet;
    }

    App::DocumentObject* obj = doc->getObject(assignment.varSet.toUtf8().constData());
    if (obj) {
        if (!obj->isDerivedFrom(App::VarSet::getClassTypeId())) {
            message = QObject::tr("Object is not a VarSet: %1").arg(assignment.varSet);
            return nullptr;
        }
        return obj;
    }

    App::DocumentObject* byLabel = findUniqueVarSetByLabel(doc, assignment.varSet, message);
    if (byLabel) {
        return byLabel;
    }

    if (message.isEmpty()) {
        message = QObject::tr("VarSet not found: %1").arg(assignment.varSet);
    }
    return nullptr;
}

App::Property* ensureProperty(
    App::DocumentObject* varSet,
    const QString& name,
    const Base::Type& type,
    const char* group
)
{
    if (!varSet || type.isBad()) {
        return nullptr;
    }

    const std::string propName = name.toStdString();
    App::Property* prop = varSet->getPropertyByName(propName.c_str());
    // Intentionally reuse an existing property regardless of requested type.
    // Assignment semantics target the existing variable if it already exists.
    if (prop && prop->getContainer() == varSet) {
        return prop;
    }

    return varSet->addDynamicProperty(type.getName(), propName.c_str(), group);
}

bool assignExpressionToProperty(
    App::DocumentObject* varSet,
    App::Property* prop,
    const App::Expression* expression,
    QString& message
)
{
    if (!varSet || !prop || !expression) {
        message = QObject::tr("Could not assign expression to variable property.");
        return false;
    }

    try {
        if (const auto* number = freecad_cast<const App::NumberExpression*>(expression)) {
            return assignNumberLiteral(varSet, prop, number->getValue(), message);
        }

        if (const auto* stringExpr = freecad_cast<const App::StringExpression*>(expression)) {
            if (auto* stringProp = freecad_cast<App::PropertyString*>(prop)) {
                stringProp->setValue(stringExpr->getText());
                clearPropertyExpression(varSet, prop);
                return true;
            }
        }

        if (const auto* op = freecad_cast<const App::OperatorExpression*>(expression); op
            && op->getOperator() == App::OperatorExpression::Operator::UNIT
            && freecad_cast<const App::NumberExpression*>(op->getLeft())) {
            std::unique_ptr<App::Expression> result(op->eval());
            if (const auto* number = freecad_cast<App::NumberExpression*>(result.get())) {
                return assignNumberLiteral(varSet, prop, number->getValue(), message);
            }
        }

        const App::ObjectIdentifier path(*prop);
        std::shared_ptr<App::Expression> assigned(expression->copy());
        AssignmentExpressionVisitor visitor(*assigned, path);
        assigned->visit(visitor);
        const auto error = varSet->ExpressionEngine.validateExpression(path, assigned);
        if (!error.empty()) {
            message = QString::fromStdString(error);
            return false;
        }
        const auto value = assigned->getValueAsAny();
        varSet->ExpressionEngine.setValue(path, assigned);
        if (!App::isAnyEqual(value, prop->getPathValue(path))) {
            prop->setPathValue(path, value);
        }
        // Dependent parameters are updated by normal document recompute after acceptance.
        return true;
    }
    catch (const Base::Exception& e) {
        message = QString::fromUtf8(e.what());
        return false;
    }
}

QString qualifyDefaultVarSetNames(App::Document* doc, const QString& text)
{
    if (!doc || text.isEmpty()) {
        return text;
    }

    App::DocumentObject* varSet = doc->getObject(DefaultVarSetName);
    if (!varSet || !varSet->isDerivedFrom(App::VarSet::getClassTypeId())) {
        return text;
    }

    const std::string input = text.toStdString();
    std::string output;
    output.reserve(input.size() + 16);

    for (size_t i = 0; i < input.size();) {
        if (input[i] == '<' && i + 1 < input.size() && input[i + 1] == '<') {
            const size_t end = endOfObjectReferenceString(input, i);
            output.append(input, i, end - i);
            i = end;
            continue;
        }

        if (!isIdentifierStart(input[i]) || (i > 0 && isIdentifierChar(input[i - 1]))) {
            output.push_back(input[i]);
            ++i;
            continue;
        }

        size_t j = i + 1;
        while (j < input.size() && isIdentifierChar(input[j])) {
            ++j;
        }

        const std::string token = input.substr(i, j - i);
        const bool skip = hasDotBefore(input, i) || hasDotAfter(input, j) || isFunctionCall(input, j)
            || token == DefaultVarSetName || App::ExpressionParser::isTokenAUnit(token)
            || App::ExpressionParser::isTokenAConstant(token);

        if (!skip && hasPropertyInDefaultVarSet(varSet, token)) {
            output += DefaultVarSetName;
            output.push_back('.');
        }

        output += token;
        i = j;
    }

    return QString::fromStdString(output);
}

std::string makeReferenceExpression(const App::DocumentObject* varSet, const QString& name)
{
    if (!varSet || name.isEmpty()) {
        return {};
    }
    return std::string(varSet->getNameInDocument()) + "." + name.toStdString();
}

bool looksLikeExpressionInput(const QString& text)
{
    const QString normalized = text.trimmed();
    if (normalized.isEmpty()) {
        return false;
    }

    if (parseAssignment(normalized).isAssignment || normalized.contains(QStringLiteral("<<"))) {
        return true;
    }

    try {
        if (Base::Quantity::parse(normalized.toStdString()).isValid()) {
            return false;
        }
    }
    catch (...) {
    }

    for (QChar c : normalized) {
        if (c.isLetter() || c == QLatin1Char('_') || c == QLatin1Char('(') || c == QLatin1Char(')')
            || c == QLatin1Char('*') || c == QLatin1Char('/') || c == QLatin1Char('^')
            || c == QLatin1Char('+') || c == QLatin1Char('-')) {
            return true;
        }
    }

    return false;
}

}  // namespace Gui::InlineExpression
