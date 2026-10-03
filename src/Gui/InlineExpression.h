// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <memory>

#include <Base/Type.h>
#include <Base/Quantity.h>
#include <QString>

namespace App
{
class Document;
class DocumentObject;
class Expression;
class ObjectIdentifier;
class Property;
}  // namespace App

namespace Gui::InlineExpression
{

inline constexpr const char* DefaultVarSetName = "Parameters";
inline constexpr const char* DefaultVarSetGroup = "Variables";

class GuiExport AssignmentGuard
{
public:
    AssignmentGuard(App::Document* doc, const App::ObjectIdentifier* target = nullptr);
    ~AssignmentGuard();
    // Call before creating or changing the assigned parameter.
    void watch(App::DocumentObject* varSet, const QString& name);
    void recordCreatedVarSet(App::DocumentObject* varSet);
    void commit();

private:
    struct State;
    std::unique_ptr<State> state;
};

struct Assignment
{
    bool isAssignment = false;
    bool hasExplicitVarSet = false;
    bool isLabelVarSet = false;
    QString varSet;
    QString name;
    QString valueExpr;
};

QString normalizeInput(QString text);
Assignment parseAssignment(const QString& text);
bool isValidName(const QString& name, QString& message);
App::DocumentObject* resolveExpressionOwner(App::DocumentObject* boundOwner, App::Document*& doc);
bool parseNumberExpression(
    const App::DocumentObject* owner,
    const QString& source,
    std::shared_ptr<App::Expression>& expr,
    Base::Quantity& quantity,
    QString& message
);

App::DocumentObject* resolveVarSet(
    App::Document* doc,
    const Assignment& assignment,
    bool createDefault,
    QString& message,
    AssignmentGuard* guard = nullptr
);

App::Property* ensureProperty(
    App::DocumentObject* varSet,
    const QString& name,
    const Base::Type& type,
    const char* group = DefaultVarSetGroup
);

bool assignExpressionToProperty(
    App::DocumentObject* varSet,
    App::Property* prop,
    const App::Expression* expression,
    QString& message
);

QString qualifyDefaultVarSetNames(App::Document* doc, const QString& text);
std::string makeReferenceExpression(const App::DocumentObject* varSet, const QString& name);
bool looksLikeExpressionInput(const QString& text);

}  // namespace Gui::InlineExpression
