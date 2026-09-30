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
    QString& message
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
